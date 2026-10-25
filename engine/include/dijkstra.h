#ifndef PHYSARUM_DIJKSTRA_H
#define PHYSARUM_DIJKSTRA_H

#include "common.h"
#include "graph.h"
#include "architecture.h"

typedef struct {
    int node;
    double dist;
} MinHeapNode;

typedef struct {
    int capacity;
    int size;
    MinHeapNode* elements;
    int* node_pos; // Array mapping node_id -> heap index for decrease_key
} MinHeap;

typedef struct {
    double bend_penalty;         // Penalty added when changing direction
    double congestion_weight;     // Multiplier for edges operating near or above capacity
    bool enforce_hard_capacity;  // If true, reject paths that exceed capacity
} RoutingConstraints;

RoutingConstraints dijkstra_default_constraints(void);

MinHeap* min_heap_create(int capacity);
void min_heap_free(MinHeap* heap);
void min_heap_insert_or_decrease(MinHeap* heap, int node, double dist);
bool min_heap_extract_min(MinHeap* heap, MinHeapNode* out_node);
bool min_heap_is_empty(const MinHeap* heap);

// Single pair shortest path on graph
bool dijkstra_find_path(const RoutingGraph* graph, int start_node, int target_node, const RoutingConstraints* constraints, int net_demand, RoutePath* out_path);

#endif /* PHYSARUM_DIJKSTRA_H */
