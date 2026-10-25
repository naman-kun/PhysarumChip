#include "../include/graph.h"

RoutingGraph* routing_graph_create(double die_w, double die_h, double pitch, int default_capacity) {
    if (die_w <= 0 || die_h <= 0 || pitch <= 0) return NULL;

    RoutingGraph* g = (RoutingGraph*)calloc(1, sizeof(RoutingGraph));
    if (!g) return NULL;

    g->die_width = die_w;
    g->die_height = die_h;
    g->pitch = pitch;
    g->default_edge_capacity = (default_capacity > 0) ? default_capacity : 4;

    g->grid_w = (int)floor(die_w / pitch) + 1;
    g->grid_h = (int)floor(die_h / pitch) + 1;
    g->node_count = g->grid_w * g->grid_h;

    g->nodes = (RoutingNode*)calloc(g->node_count, sizeof(RoutingNode));
    if (!g->nodes) {
        free(g);
        return NULL;
    }

    // Initialize nodes
    for (int gy = 0; gy < g->grid_h; ++gy) {
        for (int gx = 0; gx < g->grid_w; ++gx) {
            int idx = gy * g->grid_w + gx;
            RoutingNode* n = &g->nodes[idx];
            n->id = idx;
            n->gx = gx;
            n->gy = gy;
            n->world_x = gx * pitch;
            n->world_y = gy * pitch;
            n->is_blocked = false;
            n->port_comp_id = -1;
            n->pressure = 0.0;
            n->degree = 0;
            for (int k = 0; k < 4; ++k) {
                n->neighbor_nodes[k] = -1;
                n->incident_edges[k] = -1;
            }
        }
    }

    // Allocate edges: horizontal (grid_w - 1) * grid_h + vertical grid_w * (grid_h - 1)
    int max_edges = (g->grid_w - 1) * g->grid_h + g->grid_w * (g->grid_h - 1);
    g->edges = (RoutingEdge*)calloc(max_edges, sizeof(RoutingEdge));
    if (!g->edges) {
        free(g->nodes);
        free(g);
        return NULL;
    }
    g->edge_count = 0;

    // Connect horizontal edges
    for (int gy = 0; gy < g->grid_h; ++gy) {
        for (int gx = 0; gx < g->grid_w - 1; ++gx) {
            int u = gy * g->grid_w + gx;
            int v = gy * g->grid_w + (gx + 1);
            int e_idx = g->edge_count++;
            RoutingEdge* e = &g->edges[e_idx];
            e->id = e_idx;
            e->u = u;
            e->v = v;
            e->length = pitch;
            e->conductivity = 1.0;
            e->capacity = g->default_edge_capacity;
            e->demand = 0;
            e->is_blocked = false;
            e->flow = 0.0;
            e->effective_cost = pitch;

            RoutingNode* nu = &g->nodes[u];
            if (nu->degree < 4) {
                nu->neighbor_nodes[nu->degree] = v;
                nu->incident_edges[nu->degree++] = e_idx;
            }

            RoutingNode* nv = &g->nodes[v];
            if (nv->degree < 4) {
                nv->neighbor_nodes[nv->degree] = u;
                nv->incident_edges[nv->degree++] = e_idx;
            }
        }
    }

    // Connect vertical edges
    for (int gy = 0; gy < g->grid_h - 1; ++gy) {
        for (int gx = 0; gx < g->grid_w; ++gx) {
            int u = gy * g->grid_w + gx;
            int v = (gy + 1) * g->grid_w + gx;
            int e_idx = g->edge_count++;
            RoutingEdge* e = &g->edges[e_idx];
            e->id = e_idx;
            e->u = u;
            e->v = v;
            e->length = pitch;
            e->conductivity = 1.0;
            e->capacity = g->default_edge_capacity;
            e->demand = 0;
            e->is_blocked = false;
            e->flow = 0.0;
            e->effective_cost = pitch;

            RoutingNode* nu = &g->nodes[u];
            if (nu->degree < 4) {
                nu->neighbor_nodes[nu->degree] = v;
                nu->incident_edges[nu->degree++] = e_idx;
            }

            RoutingNode* nv = &g->nodes[v];
            if (nv->degree < 4) {
                nv->neighbor_nodes[nv->degree] = u;
                nv->incident_edges[nv->degree++] = e_idx;
            }
        }
    }

    return g;
}

void routing_graph_free(RoutingGraph* graph) {
    if (!graph) return;
    if (graph->nodes) free(graph->nodes);
    if (graph->edges) free(graph->edges);
    free(graph);
}

int routing_graph_node_index(const RoutingGraph* graph, int gx, int gy) {
    if (!graph) return -1;
    if (gx < 0 || gx >= graph->grid_w || gy < 0 || gy >= graph->grid_h) return -1;
    return gy * graph->grid_w + gx;
}

bool routing_graph_snap_point(const RoutingGraph* graph, double wx, double wy, int* out_node_idx) {
    if (!graph || !out_node_idx) return false;
    int gx = (int)round(wx / graph->pitch);
    int gy = (int)round(wy / graph->pitch);
    if (gx < 0) gx = 0;
    if (gx >= graph->grid_w) gx = graph->grid_w - 1;
    if (gy < 0) gy = 0;
    if (gy >= graph->grid_h) gy = graph->grid_h - 1;
    *out_node_idx = gy * graph->grid_w + gx;
    return true;
}

int routing_graph_find_edge(const RoutingGraph* graph, int u, int v) {
    if (!graph || u < 0 || u >= graph->node_count || v < 0 || v >= graph->node_count) return -1;
    const RoutingNode* nu = &graph->nodes[u];
    for (int i = 0; i < nu->degree; ++i) {
        if (nu->neighbor_nodes[i] == v) {
            return nu->incident_edges[i];
        }
    }
    return -1;
}

StatusCode routing_graph_build_from_arch(RoutingGraph* graph, const Architecture* arch, const Rect2D* extra_obstacles, int extra_obs_count) {
    if (!graph || !arch) return STATUS_ERROR_INVALID_PARAM;

    // 1. Reset node blocks
    for (int i = 0; i < graph->node_count; ++i) {
        graph->nodes[i].is_blocked = false;
        graph->nodes[i].port_comp_id = -1;
        graph->nodes[i].port_name[0] = '\0';
    }

    // 2. Mark component interiors as blocked
    for (int i = 0; i < arch->component_count; ++i) {
        const ComponentInstance* c = &arch->components[i];
        // Calculate grid range covered by component interior
        // Shrink slightly so perimeter grid lines can still serve as routing channels
        double margin = 0.2 * graph->pitch;
        int min_gx = (int)ceil((c->bounds.x + margin) / graph->pitch);
        int max_gx = (int)floor((c->bounds.x + c->bounds.width - margin) / graph->pitch);
        int min_gy = (int)ceil((c->bounds.y + margin) / graph->pitch);
        int max_gy = (int)floor((c->bounds.y + c->bounds.height - margin) / graph->pitch);

        for (int gy = min_gy; gy <= max_gy; ++gy) {
            for (int gx = min_gx; gx <= max_gx; ++gx) {
                int idx = routing_graph_node_index(graph, gx, gy);
                if (idx >= 0 && idx < graph->node_count) {
                    graph->nodes[idx].is_blocked = true;
                }
            }
        }
    }

    // 3. Mark extra obstacles / keepouts as blocked
    graph->blocked_region_count = 0;
    if (extra_obstacles && extra_obs_count > 0) {
        for (int k = 0; k < extra_obs_count && k < MAX_OBSTACLES; ++k) {
            graph->blocked_regions[graph->blocked_region_count++] = extra_obstacles[k];
            const Rect2D* obs = &extra_obstacles[k];
            int min_gx = (int)floor(obs->x / graph->pitch);
            int max_gx = (int)ceil((obs->x + obs->width) / graph->pitch);
            int min_gy = (int)floor(obs->y / graph->pitch);
            int max_gy = (int)ceil((obs->y + obs->height) / graph->pitch);

            for (int gy = min_gy; gy <= max_gy; ++gy) {
                for (int gx = min_gx; gx <= max_gx; ++gx) {
                    int idx = routing_graph_node_index(graph, gx, gy);
                    if (idx >= 0 && idx < graph->node_count) {
                        graph->nodes[idx].is_blocked = true;
                    }
                }
            }
        }
    }

    // 4. Unblock port contact points and register them
    for (int i = 0; i < arch->component_count; ++i) {
        const ComponentInstance* c = &arch->components[i];
        for (int p = 0; p < c->port_count; ++p) {
            const Port* port = &c->ports[p];
            int p_idx = -1;
            if (routing_graph_snap_point(graph, port->abs_x, port->abs_y, &p_idx)) {
                RoutingNode* pn = &graph->nodes[p_idx];
                pn->is_blocked = false; // Port contact MUST be routable!
                pn->port_comp_id = c->id;
                strncpy(pn->port_name, port->name, MAX_NAME_LEN - 1);
                pn->port_name[MAX_NAME_LEN - 1] = '\0';
            }
        }
    }

    // 5. Update edge blocked status
    for (int e = 0; e < graph->edge_count; ++e) {
        RoutingEdge* edge = &graph->edges[e];
        const RoutingNode* nu = &graph->nodes[edge->u];
        const RoutingNode* nv = &graph->nodes[edge->v];

        // An edge is blocked if both endpoints are blocked, or if an endpoint is blocked without being a port
        if (nu->is_blocked || nv->is_blocked) {
            edge->is_blocked = true;
            edge->conductivity = 1e-6; // virtually zero conductivity
        } else {
            edge->is_blocked = false;
            edge->conductivity = 1.0;
        }
    }

    return STATUS_OK;
}

void routing_graph_reset_state(RoutingGraph* graph, double default_conductivity) {
    if (!graph) return;
    for (int i = 0; i < graph->node_count; ++i) {
        graph->nodes[i].pressure = 0.0;
    }
    for (int e = 0; e < graph->edge_count; ++e) {
        RoutingEdge* edge = &graph->edges[e];
        edge->demand = 0;
        edge->flow = 0.0;
        if (!edge->is_blocked) {
            edge->conductivity = default_conductivity;
            edge->effective_cost = edge->length;
        } else {
            edge->conductivity = 1e-6;
            edge->effective_cost = 1e9;
        }
    }
}

bool routing_graph_reserve_capacity(RoutingGraph* graph, const RoutePath* path, int demand) {
    if (!graph || !path || !path->is_valid) return false;
    for (int i = 0; i < path->node_count - 1; ++i) {
        int u = path->nodes[i];
        int v = path->nodes[i + 1];
        int e_idx = routing_graph_find_edge(graph, u, v);
        if (e_idx >= 0) {
            graph->edges[e_idx].demand += demand;
        }
    }
    return true;
}

void routing_graph_release_capacity(RoutingGraph* graph, const RoutePath* path, int demand) {
    if (!graph || !path || !path->is_valid) return;
    for (int i = 0; i < path->node_count - 1; ++i) {
        int u = path->nodes[i];
        int v = path->nodes[i + 1];
        int e_idx = routing_graph_find_edge(graph, u, v);
        if (e_idx >= 0) {
            graph->edges[e_idx].demand -= demand;
            if (graph->edges[e_idx].demand < 0) graph->edges[e_idx].demand = 0;
        }
    }
}
