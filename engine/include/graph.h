#ifndef PHYSARUM_GRAPH_H
#define PHYSARUM_GRAPH_H

#include "common.h"
#include "architecture.h"

typedef struct {
    int id;
    int gx;
    int gy;
    double world_x;
    double world_y;
    bool is_blocked;
    int port_comp_id;    // -1 if regular node, or comp_id if port contact point
    char port_name[MAX_NAME_LEN];
    double pressure;     // Node pressure for Physarum flow equation
    int degree;
    int neighbor_nodes[4];
    int incident_edges[4]; // Indices into graph->edges
} RoutingNode;

typedef struct {
    int id;
    int u;               // Node index 1
    int v;               // Node index 2
    double length;       // Distance between nodes (typically 1.0 * pitch)
    double conductivity; // D(e)
    int capacity;        // Available track capacity
    int demand;          // Current allocated track demand
    bool is_blocked;     // True if edge traverses obstacle or cut
    double flow;         // Q(e) flow magnitude/direction
    double effective_cost; // Dynamic cost for path extraction
} RoutingEdge;

typedef struct {
    int grid_w;
    int grid_h;
    double pitch;
    double die_width;
    double die_height;
    int default_edge_capacity;

    int node_count;
    RoutingNode* nodes;

    int edge_count;
    RoutingEdge* edges;

    int blocked_region_count;
    Rect2D blocked_regions[MAX_OBSTACLES];
} RoutingGraph;

typedef struct {
    int node_count;
    int nodes[MAX_PATH_NODES];
    double total_length;
    int total_bends;
    double peak_congestion;
    double avg_congestion;
    bool is_valid;
    char fail_reason[MAX_NAME_LEN];
} RoutePath;

RoutingGraph* routing_graph_create(double die_w, double die_h, double pitch, int default_capacity);
void routing_graph_free(RoutingGraph* graph);

StatusCode routing_graph_build_from_arch(RoutingGraph* graph, const Architecture* arch, const Rect2D* extra_obstacles, int extra_obs_count);
int routing_graph_node_index(const RoutingGraph* graph, int gx, int gy);
bool routing_graph_snap_point(const RoutingGraph* graph, double wx, double wy, int* out_node_idx);

void routing_graph_reset_state(RoutingGraph* graph, double default_conductivity);
bool routing_graph_reserve_capacity(RoutingGraph* graph, const RoutePath* path, int demand);
void routing_graph_release_capacity(RoutingGraph* graph, const RoutePath* path, int demand);

int routing_graph_find_edge(const RoutingGraph* graph, int u, int v);

#endif /* PHYSARUM_GRAPH_H */
