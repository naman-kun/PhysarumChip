#include "../include/metrics.h"

void metrics_init_results(RoutingResults* results, const char* algo_name, int total_nets) {
    if (!results) return;
    memset(results, 0, sizeof(RoutingResults));
    strncpy(results->algorithm_name, algo_name ? algo_name : "UNKNOWN", MAX_NAME_LEN - 1);
    results->algorithm_name[MAX_NAME_LEN - 1] = '\0';
    results->total_nets = total_nets;
}

void metrics_calculate_aggregates(RoutingResults* results, const RoutingGraph* graph) {
    if (!results) return;

    results->routed_nets = 0;
    results->unrouted_nets = 0;
    results->total_wirelength = 0.0;
    results->total_bends = 0;
    results->capacity_violations = 0;

    for (int i = 0; i < results->total_nets; ++i) {
        NetRouteRecord* nr = &results->net_records[i];
        if (nr->routed && nr->path.is_valid) {
            results->routed_nets++;
            results->total_wirelength += nr->wirelength;
            results->total_bends += nr->bends;
        } else {
            results->unrouted_nets++;
        }
    }

    results->success_rate = (results->total_nets > 0) ?
        ((double)results->routed_nets / (double)results->total_nets * 100.0) : 0.0;

    results->avg_wirelength = (results->routed_nets > 0) ?
        (results->total_wirelength / (double)results->routed_nets) : 0.0;

    // Scan graph edges for congestion statistics
    results->peak_congestion = 0.0;
    results->congested_edge_count = 0;
    double cong_sum = 0.0;
    int used_edges = 0;

    if (graph) {
        for (int e = 0; e < graph->edge_count; ++e) {
            const RoutingEdge* edge = &graph->edges[e];
            if (edge->demand > 0) {
                used_edges++;
                double cr = (double)edge->demand / (double)edge->capacity;
                if (cr > results->peak_congestion) results->peak_congestion = cr;
                cong_sum += cr;
                if (edge->demand > edge->capacity) {
                    results->congested_edge_count++;
                    results->capacity_violations += (edge->demand - edge->capacity);
                }
            }
        }
    }

    results->avg_congestion = (used_edges > 0) ? (cong_sum / (double)used_edges) : 0.0;

    // Weighted abstract cost function:
    // length + 10 * congestion penalty + 0.5 * bends + 100 * unrouted
    results->abstract_total_cost = results->total_wirelength
                                 + 10.0 * (results->congested_edge_count + results->capacity_violations)
                                 + 0.5 * results->total_bends
                                 + 100.0 * results->unrouted_nets;
}

ComparisonReport metrics_compare(const RoutingResults* physarum, const RoutingResults* dijkstra) {
    ComparisonReport report;
    memset(&report, 0, sizeof(ComparisonReport));

    if (physarum) report.physarum_results = *physarum;
    if (dijkstra) report.dijkstra_results = *dijkstra;

    if (dijkstra && dijkstra->total_wirelength > 0.0 && physarum) {
        report.wirelength_diff_pct = ((physarum->total_wirelength - dijkstra->total_wirelength) / dijkstra->total_wirelength) * 100.0;
    } else {
        report.wirelength_diff_pct = 0.0;
    }

    if (physarum && dijkstra) {
        report.peak_congestion_diff = physarum->peak_congestion - dijkstra->peak_congestion;
        report.avg_congestion_diff = physarum->avg_congestion - dijkstra->avg_congestion;
        report.runtime_diff_ms = physarum->runtime_ms - dijkstra->runtime_ms;
    }

    return report;
}
