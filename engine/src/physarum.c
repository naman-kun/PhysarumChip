#include "../include/physarum.h"

PhysarumParams physarum_default_params(void) {
    PhysarumParams p;
    p.rho = 0.15;
    p.alpha = 1.2;
    p.d_min = 0.05;
    p.d_max = 50.0;
    p.source_current = 1.0;
    p.max_pressure_iters = 80;
    p.pressure_tolerance = 1e-4;
    p.adaptation_steps = 15;
    p.congestion_weight = 3.0;
    p.bend_penalty = 0.3;
    p.enforce_hard_capacity = false;
    p.reroute_passes = 1;
    return p;
}

bool physarum_route_single_net(RoutingGraph* graph,
                               int start_node,
                               int target_node,
                               const PhysarumParams* params,
                               int net_demand,
                               RoutePath* out_path,
                               int* out_pressure_iters,
                               int* out_adapt_iters) {
    if (!graph || !out_path) return false;
    memset(out_path, 0, sizeof(RoutePath));
    out_path->is_valid = false;

    if (start_node < 0 || start_node >= graph->node_count || target_node < 0 || target_node >= graph->node_count) {
        snprintf(out_path->fail_reason, MAX_NAME_LEN, "Invalid endpoint node indices (%d, %d)", start_node, target_node);
        return false;
    }

    if (start_node == target_node) {
        out_path->node_count = 1;
        out_path->nodes[0] = start_node;
        out_path->is_valid = true;
        out_path->total_length = 0.0;
        return true;
    }

    PhysarumParams p = params ? *params : physarum_default_params();
    int N = graph->node_count;
    int E = graph->edge_count;

    // Working arrays
    double* P = (double*)calloc(N, sizeof(double));
    double* D = (double*)malloc(sizeof(double) * E);
    double* Q = (double*)calloc(E, sizeof(double));

    if (!P || !D || !Q) {
        if (P) free(P);
        if (D) free(D);
        if (Q) free(Q);
        snprintf(out_path->fail_reason, MAX_NAME_LEN, "Memory allocation failed");
        return false;
    }

    // 1. Initialize edge conductivities
    for (int e = 0; e < E; ++e) {
        const RoutingEdge* edge = &graph->edges[e];
        if (edge->is_blocked) {
            D[e] = 1e-6;
        } else {
            D[e] = 1.0;
        }
    }

    int total_pressure_iters = 0;
    int adapt_step = 0;

    // 2. Physarum Flow Adaptation Loop
    for (adapt_step = 0; adapt_step < p.adaptation_steps; ++adapt_step) {
        // Fix target_node as reference ground: P[target_node] = 0.0
        P[target_node] = 0.0;

        // Iterative Gauss-Seidel pressure solver
        int p_iter;
        for (p_iter = 0; p_iter < p.max_pressure_iters; ++p_iter) {
            double max_diff = 0.0;

            for (int u = 0; u < N; ++u) {
                if (u == target_node) continue; // Reference ground stays 0.0
                if (graph->nodes[u].is_blocked && u != start_node) continue;

                double sum_G = 0.0;
                double sum_GP = 0.0;

                const RoutingNode* nu = &graph->nodes[u];
                for (int i = 0; i < nu->degree; ++i) {
                    int v = nu->neighbor_nodes[i];
                    int e_idx = nu->incident_edges[i];
                    const RoutingEdge* edge = &graph->edges[e_idx];

                    if (edge->is_blocked) continue;
                    if (graph->nodes[v].is_blocked && v != target_node && v != start_node) continue;

                    // Hard capacity check during flow simulation
                    if (p.enforce_hard_capacity && (edge->demand + net_demand > edge->capacity)) {
                        continue;
                    }

                    // Conductance with congestion damping
                    double cong_factor = 1.0;
                    double cr = (double)(edge->demand + net_demand) / (double)edge->capacity;
                    if (cr > 0.8) {
                        cong_factor = 1.0 / (1.0 + p.congestion_weight * (cr * cr));
                    }

                    double G = (D[e_idx] / edge->length) * cong_factor;
                    sum_G += G;
                    sum_GP += G * P[v];
                }

                if (sum_G > 1e-9) {
                    // Current source at start_node
                    double I_u = (u == start_node) ? p.source_current : 0.0;
                    double new_P = (I_u + sum_GP) / sum_G;
                    double diff = fabs(new_P - P[u]);
                    if (diff > max_diff) max_diff = diff;
                    P[u] = new_P;
                }
            }

            if (max_diff < p.pressure_tolerance) {
                p_iter++;
                break;
            }
        }

        total_pressure_iters += p_iter;

        // Calculate flow Q(e) and adapt conductivity D(e)
        for (int e = 0; e < E; ++e) {
            const RoutingEdge* edge = &graph->edges[e];
            if (edge->is_blocked) continue;

            int u = edge->u;
            int v = edge->v;
            double pot_diff = fabs(P[u] - P[v]);
            Q[e] = (D[e] / edge->length) * pot_diff;

            // Conductivity update equation:
            // D_new(e) = max(D_min, min(D_max, (1 - rho) * D(e) + alpha * Q(e)))
            double d_new = (1.0 - p.rho) * D[e] + p.alpha * Q[e];
            if (d_new < p.d_min) d_new = p.d_min;
            if (d_new > p.d_max) d_new = p.d_max;
            D[e] = d_new;
        }
    }

    if (out_pressure_iters) *out_pressure_iters = total_pressure_iters;
    if (out_adapt_iters) *out_adapt_iters = adapt_step;

    // 3. Extract path using adaptive resistance
    // Resistance cost = (L(e) / D(e)) * (1 + congestion_penalty) + bend_penalty
    // We use Dijkstra over these adaptive weights to extract the loop-free tube!
    double* dist = (double*)malloc(sizeof(double) * N);
    int* parent = (int*)malloc(sizeof(int) * N);
    bool* visited = (bool*)calloc(N, sizeof(bool));

    for (int i = 0; i < N; ++i) {
        dist[i] = 1e12;
        parent[i] = -1;
    }

    MinHeap* heap = min_heap_create(N);
    dist[start_node] = 0.0;
    min_heap_insert_or_decrease(heap, start_node, 0.0);

    bool reached = false;
    while (!min_heap_is_empty(heap)) {
        MinHeapNode curr;
        if (!min_heap_extract_min(heap, &curr)) break;

        int u = curr.node;
        if (visited[u]) continue;
        visited[u] = true;

        if (u == target_node) {
            reached = true;
            break;
        }

        const RoutingNode* nu = &graph->nodes[u];
        for (int i = 0; i < nu->degree; ++i) {
            int v = nu->neighbor_nodes[i];
            int e_idx = nu->incident_edges[i];
            if (visited[v]) continue;

            const RoutingEdge* edge = &graph->edges[e_idx];
            if (edge->is_blocked) continue;
            if (graph->nodes[v].is_blocked && v != target_node) continue;

            int new_demand = edge->demand + net_demand;
            if (p.enforce_hard_capacity && new_demand > edge->capacity) {
                continue;
            }

            // Physarum adaptive resistance
            double cong_pen = 1.0;
            double cr = (double)new_demand / (double)edge->capacity;
            if (cr > 0.8) {
                cong_pen += p.congestion_weight * (cr * cr);
            }

            // High conductivity -> lower resistance
            double resistance = (edge->length / (D[e_idx] + 1e-4)) * cong_pen;

            // Direction bend penalty
            if (parent[u] != -1) {
                int prev_u = parent[u];
                int prev_dx = nu->gx - graph->nodes[prev_u].gx;
                int prev_dy = nu->gy - graph->nodes[prev_u].gy;
                int next_dx = graph->nodes[v].gx - nu->gx;
                int next_dy = graph->nodes[v].gy - nu->gy;

                if ((prev_dx != 0 && next_dy != 0) || (prev_dy != 0 && next_dx != 0)) {
                    resistance += p.bend_penalty;
                }
            }

            double new_dist = dist[u] + resistance;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                parent[v] = u;
                min_heap_insert_or_decrease(heap, v, new_dist);
            }
        }
    }

    min_heap_free(heap);

    if (!reached) {
        free(P); free(D); free(Q);
        free(dist); free(parent); free(visited);
        snprintf(out_path->fail_reason, MAX_NAME_LEN, "Physarum adaptation failed to connect endpoints within capacity limits");
        return false;
    }

    // Reconstruct path
    int temp_path[MAX_PATH_NODES];
    int count = 0;
    int curr = target_node;
    while (curr != -1 && count < MAX_PATH_NODES) {
        temp_path[count++] = curr;
        curr = parent[curr];
    }

    out_path->node_count = count;
    for (int i = 0; i < count; ++i) {
        out_path->nodes[i] = temp_path[count - 1 - i];
    }
    out_path->is_valid = true;

    // Calculate path metrics
    out_path->total_length = 0.0;
    out_path->total_bends = 0;
    out_path->peak_congestion = 0.0;
    double cong_sum = 0.0;

    for (int i = 0; i < out_path->node_count - 1; ++i) {
        int u = out_path->nodes[i];
        int v = out_path->nodes[i + 1];
        int e_idx = routing_graph_find_edge(graph, u, v);
        if (e_idx >= 0) {
            const RoutingEdge* e = &graph->edges[e_idx];
            out_path->total_length += e->length;
            double cr = (double)(e->demand + net_demand) / (double)e->capacity;
            if (cr > out_path->peak_congestion) out_path->peak_congestion = cr;
            cong_sum += cr;
        }

        if (i > 0) {
            int prev = out_path->nodes[i - 1];
            int dx1 = graph->nodes[u].gx - graph->nodes[prev].gx;
            int dy1 = graph->nodes[u].gy - graph->nodes[prev].gy;
            int dx2 = graph->nodes[v].gx - graph->nodes[u].gx;
            int dy2 = graph->nodes[v].gy - graph->nodes[u].gy;
            if ((dx1 != 0 && dy2 != 0) || (dy1 != 0 && dx2 != 0)) {
                out_path->total_bends++;
            }
        }
    }

    out_path->avg_congestion = (out_path->node_count > 1) ? (cong_sum / (out_path->node_count - 1)) : 0.0;

    free(P); free(D); free(Q);
    free(dist); free(parent); free(visited);
    return true;
}
