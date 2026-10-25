#ifndef PHYSARUM_JSON_IO_H
#define PHYSARUM_JSON_IO_H

#include "common.h"
#include "architecture.h"
#include "placement.h"
#include "graph.h"
#include "dijkstra.h"
#include "physarum.h"
#include "metrics.h"
#include "../third_party/cJSON.h"

// Parse architecture configuration from JSON
bool json_parse_arch_config(const cJSON* root, ArchConfig* out_cfg, char* err_msg, size_t err_size);

// Parse full architecture (with custom component positions & netlist if user edited)
bool json_parse_architecture(const cJSON* root, Architecture* out_arch, char* err_msg, size_t err_size);

// Parse routing constraints and algorithm parameters
bool json_parse_routing_params(const cJSON* root,
                              RoutingConstraints* out_dijkstra,
                              PhysarumParams* out_physarum,
                              int* out_capacity,
                              Rect2D* out_blocked,
                              int* out_blocked_count,
                              char* err_msg,
                              size_t err_size);

// Serialization
cJSON* json_serialize_architecture(const Architecture* arch);
cJSON* json_serialize_routing_results(const RoutingResults* results, const RoutingGraph* graph);
cJSON* json_serialize_comparison(const ComparisonReport* report, const RoutingGraph* physarum_graph, const RoutingGraph* dijkstra_graph);
cJSON* json_serialize_error(StatusCode code, const char* message);
cJSON* json_serialize_success(const char* message);

#endif /* PHYSARUM_JSON_IO_H */
