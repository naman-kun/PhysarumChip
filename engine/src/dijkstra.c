#include "../include/dijkstra.h"

RoutingConstraints dijkstra_default_constraints(void) {
    RoutingConstraints c;
    c.bend_penalty = 0.3;
    c.congestion_weight = 2.5;
    c.enforce_hard_capacity = false;
    return c;
}

MinHeap* min_heap_create(int capacity) {
    MinHeap* h = (MinHeap*)malloc(sizeof(MinHeap));
    if (!h) return NULL;
    h->capacity = capacity;
    h->size = 0;
    h->elements = (MinHeapNode*)malloc(sizeof(MinHeapNode) * capacity);
    h->node_pos = (int*)malloc(sizeof(int) * capacity);
    if (!h->elements || !h->node_pos) {
        if (h->elements) free(h->elements);
        if (h->node_pos) free(h->node_pos);
        free(h);
        return NULL;
    }
    for (int i = 0; i < capacity; ++i) {
        h->node_pos[i] = -1;
    }
    return h;
}

void min_heap_free(MinHeap* heap) {
    if (!heap) return;
    if (heap->elements) free(heap->elements);
    if (heap->node_pos) free(heap->node_pos);
    free(heap);
}

static void heap_swap(MinHeap* heap, int i, int j) {
    MinHeapNode temp = heap->elements[i];
    heap->elements[i] = heap->elements[j];
    heap->elements[j] = temp;

    heap->node_pos[heap->elements[i].node] = i;
    heap->node_pos[heap->elements[j].node] = j;
}

static void heap_sift_up(MinHeap* heap, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (heap->elements[idx].dist < heap->elements[parent].dist) {
            heap_swap(heap, idx, parent);
            idx = parent;
        } else {
            break;
        }
    }
}

static void heap_sift_down(MinHeap* heap, int idx) {
    while (2 * idx + 1 < heap->size) {
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;
        int smallest = idx;

        if (heap->elements[left].dist < heap->elements[smallest].dist) {
            smallest = left;
        }
        if (right < heap->size && heap->elements[right].dist < heap->elements[smallest].dist) {
            smallest = right;
        }
        if (smallest != idx) {
            heap_swap(heap, idx, smallest);
            idx = smallest;
        } else {
            break;
        }
    }
}

void min_heap_insert_or_decrease(MinHeap* heap, int node, double dist) {
    if (!heap || node < 0 || node >= heap->capacity) return;

    int pos = heap->node_pos[node];
    if (pos >= 0) {
        // Node already in heap: decrease key if new dist is smaller
        if (dist < heap->elements[pos].dist) {
            heap->elements[pos].dist = dist;
            heap_sift_up(heap, pos);
        }
    } else {
        // Insert new element
        if (heap->size >= heap->capacity) return;
        int idx = heap->size++;
        heap->elements[idx].node = node;
        heap->elements[idx].dist = dist;
        heap->node_pos[node] = idx;
        heap_sift_up(heap, idx);
    }
}

bool min_heap_extract_min(MinHeap* heap, MinHeapNode* out_node) {
    if (!heap || heap->size == 0) return false;
    *out_node = heap->elements[0];
    heap->node_pos[out_node->node] = -1;

    heap->size--;
    if (heap->size > 0) {
        heap->elements[0] = heap->elements[heap->size];
        heap->node_pos[heap->elements[0].node] = 0;
        heap_sift_down(heap, 0);
    }
    return true;
}

bool min_heap_is_empty(const MinHeap* heap) {
    return !heap || heap->size == 0;
}

bool dijkstra_find_path(const RoutingGraph* graph, int start_node, int target_node, const RoutingConstraints* constraints, int net_demand, RoutePath* out_path) {
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

    double bend_pen = constraints ? constraints->bend_penalty : 0.3;
    double cong_w = constraints ? constraints->congestion_weight : 2.5;
    bool hard_cap = constraints ? constraints->enforce_hard_capacity : false;

    int N = graph->node_count;
    double* dist = (double*)malloc(sizeof(double) * N);
    int* parent = (int*)malloc(sizeof(int) * N);
    int* in_edge = (int*)malloc(sizeof(int) * N);
    bool* visited = (bool*)calloc(N, sizeof(bool));

    if (!dist || !parent || !in_edge || !visited) {
        if (dist) free(dist);
        if (parent) free(parent);
        if (in_edge) free(in_edge);
        if (visited) free(visited);
        snprintf(out_path->fail_reason, MAX_NAME_LEN, "Memory allocation failed");
        return false;
    }

    for (int i = 0; i < N; ++i) {
        dist[i] = 1e12;
        parent[i] = -1;
        in_edge[i] = -1;
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

            // Check hard blocked state
            if (edge->is_blocked) continue;
            // Target can be an obstacle node only if it's the target itself
            if (graph->nodes[v].is_blocked && v != target_node) continue;

            // Check hard capacity constraint
            int new_demand = edge->demand + net_demand;
            if (hard_cap && new_demand > edge->capacity) {
                continue;
            }

            // Calculate base edge cost
            double edge_len = edge->length;
            double cong_ratio = (double)new_demand / (double)edge->capacity;
            double cong_factor = 1.0;
            if (cong_ratio > 0.8) {
                cong_factor += cong_w * (cong_ratio * cong_ratio);
            }

            double step_cost = edge_len * cong_factor;

            // Direction change (bend) penalty
            if (parent[u] != -1) {
                int prev_u = parent[u];
                int prev_dx = nu->gx - graph->nodes[prev_u].gx;
                int prev_dy = nu->gy - graph->nodes[prev_u].gy;
                int next_dx = graph->nodes[v].gx - nu->gx;
                int next_dy = graph->nodes[v].gy - nu->gy;

                if ((prev_dx != 0 && next_dy != 0) || (prev_dy != 0 && next_dx != 0)) {
                    step_cost += bend_pen;
                }
            }

            double new_dist = dist[u] + step_cost;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                parent[v] = u;
                in_edge[v] = e_idx;
                min_heap_insert_or_decrease(heap, v, new_dist);
            }
        }
    }

    min_heap_free(heap);

    if (!reached) {
        free(dist);
        free(parent);
        free(in_edge);
        free(visited);
        snprintf(out_path->fail_reason, MAX_NAME_LEN, "No feasible route found (blocked or capacity exceeded)");
        return false;
    }

    // Reconstruct path backwards from target_node to start_node
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
            int p = out_path->nodes[i - 1];
            int dx1 = graph->nodes[u].gx - graph->nodes[p].gx;
            int dy1 = graph->nodes[u].gy - graph->nodes[p].gy;
            int dx2 = graph->nodes[v].gx - graph->nodes[u].gx;
            int dy2 = graph->nodes[v].gy - graph->nodes[u].gy;
            if ((dx1 != 0 && dy2 != 0) || (dy1 != 0 && dx2 != 0)) {
                out_path->total_bends++;
            }
        }
    }

    out_path->avg_congestion = (out_path->node_count > 1) ? (cong_sum / (out_path->node_count - 1)) : 0.0;

    free(dist);
    free(parent);
    free(in_edge);
    free(visited);
    return true;
}
