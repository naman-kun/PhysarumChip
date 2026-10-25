#ifndef PHYSARUM_METRICS_H
#define PHYSARUM_METRICS_H

#include "common.h"
#include "graph.h"
#include "architecture.h"

typedef struct {
    int net_id;
    char net_name[MAX_NAME_LEN];
    bool routed;
    char fail_reason[MAX_NAME_LEN];
    int demand;
    double wirelength;
    int bends;
    double max_edge_congestion;
    RoutePath path;
} NetRouteRecord;

typedef struct {
    char algorithm_name[MAX_NAME_LEN];
    int total_nets;
    int routed_nets;
    int unrouted_nets;
    double success_rate;

    double total_wirelength;
    double avg_wirelength;
    double peak_congestion;
    double avg_congestion;
    int congested_edge_count;
    int capacity_violations;
    int length_violations;
    int total_bends;

    double abstract_total_cost;
    double runtime_ms;
    int total_iterations;

    NetRouteRecord net_records[MAX_NETS];
} RoutingResults;

typedef struct {
    RoutingResults physarum_results;
    RoutingResults dijkstra_results;
    double wirelength_diff_pct;     // (physarum - dijkstra) / dijkstra * 100
    double peak_congestion_diff;   // physarum - dijkstra
    double avg_congestion_diff;
    double runtime_diff_ms;
} ComparisonReport;

void metrics_init_results(RoutingResults* results, const char* algo_name, int total_nets);
void metrics_calculate_aggregates(RoutingResults* results, const RoutingGraph* graph);
ComparisonReport metrics_compare(const RoutingResults* physarum, const RoutingResults* dijkstra);

#endif /* PHYSARUM_METRICS_H */
