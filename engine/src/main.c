#include <time.h>
#include <unistd.h>
#include "../include/common.h"
#include "../include/component.h"
#include "../include/architecture.h"
#include "../include/placement.h"
#include "../include/graph.h"
#include "../include/dijkstra.h"
#include "../include/physarum.h"
#include "../include/metrics.h"
#include "../include/json_io.h"

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

// Read all of stdin into a dynamically allocated null-terminated string
static char* read_stdin_all(void) {
    if (isatty(STDIN_FILENO)) {
        char* empty = (char*)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t cap = 65536;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) return NULL;

    ssize_t bytes_read;
    while ((bytes_read = read(STDIN_FILENO, buf + len, cap - len - 1)) > 0) {
        len += bytes_read;
        if (len + 4096 >= cap) {
            cap *= 2;
            char* new_buf = (char*)realloc(buf, cap);
            if (!new_buf) {
                free(buf);
                return NULL;
            }
            buf = new_buf;
        }
    }
    buf[len] = '\0';
    return buf;
}

// Comparator for sorting nets by priority descending (5 down to 1)
static int net_priority_cmp(const void* a, const void* b) {
    const LogicalNet* na = (const LogicalNet*)a;
    const LogicalNet* nb = (const LogicalNet*)b;
    return nb->priority - na->priority;
}

// Execute sequential routing for Dijkstra
static void execute_dijkstra_routing(RoutingGraph* graph, const Architecture* arch, const RoutingConstraints* constraints, RoutingResults* out_results) {
    double t_start = get_time_ms();
    metrics_init_results(out_results, "Dijkstra_Baseline", arch->net_count);

    LogicalNet* sorted_nets = (LogicalNet*)malloc(sizeof(LogicalNet) * arch->net_count);
    if (!sorted_nets) return;

    for (int i = 0; i < arch->net_count; ++i) {
        sorted_nets[i] = arch->nets[i];
    }
    qsort(sorted_nets, arch->net_count, sizeof(LogicalNet), net_priority_cmp);

    for (int i = 0; i < arch->net_count; ++i) {
        const LogicalNet* net = &sorted_nets[i];
        NetRouteRecord* rec = &out_results->net_records[i];
        rec->net_id = net->id;
        strncpy(rec->net_name, net->name, MAX_NAME_LEN - 1);
        rec->demand = net->track_demand;

        const ComponentInstance* src = architecture_find_component(arch, net->source_comp_id);
        const ComponentInstance* tgt = architecture_find_component(arch, net->target_comp_id);

        if (!src || !tgt) {
            rec->routed = false;
            snprintf(rec->fail_reason, MAX_NAME_LEN, "Endpoint component not found");
            continue;
        }

        Point2D p_src, p_tgt;
        component_get_port_location(src, net->source_port, &p_src);
        component_get_port_location(tgt, net->target_port, &p_tgt);

        int start_node = -1;
        int target_node = -1;
        routing_graph_snap_point(graph, p_src.x, p_src.y, &start_node);
        routing_graph_snap_point(graph, p_tgt.x, p_tgt.y, &target_node);

        RoutePath path;
        bool ok = dijkstra_find_path(graph, start_node, target_node, constraints, net->track_demand, &path);

        if (ok && path.is_valid) {
            rec->routed = true;
            rec->path = path;
            rec->wirelength = path.total_length;
            rec->bends = path.total_bends;
            rec->max_edge_congestion = path.peak_congestion;
            routing_graph_reserve_capacity(graph, &path, net->track_demand);
        } else {
            rec->routed = false;
            strncpy(rec->fail_reason, path.fail_reason[0] ? path.fail_reason : "Routing failed", MAX_NAME_LEN - 1);
        }
    }

    free(sorted_nets);
    out_results->runtime_ms = get_time_ms() - t_start;
    out_results->total_iterations = arch->net_count;
    metrics_calculate_aggregates(out_results, graph);
}

// Execute sequential routing for Physarum
static void execute_physarum_routing(RoutingGraph* graph, const Architecture* arch, const PhysarumParams* params, RoutingResults* out_results) {
    double t_start = get_time_ms();
    metrics_init_results(out_results, "Physarum_Adaptive_Flow", arch->net_count);

    LogicalNet* sorted_nets = (LogicalNet*)malloc(sizeof(LogicalNet) * arch->net_count);
    if (!sorted_nets) return;

    for (int i = 0; i < arch->net_count; ++i) {
        sorted_nets[i] = arch->nets[i];
    }
    qsort(sorted_nets, arch->net_count, sizeof(LogicalNet), net_priority_cmp);

    int total_p_iters = 0;
    for (int i = 0; i < arch->net_count; ++i) {
        const LogicalNet* net = &sorted_nets[i];
        NetRouteRecord* rec = &out_results->net_records[i];
        rec->net_id = net->id;
        strncpy(rec->net_name, net->name, MAX_NAME_LEN - 1);
        rec->demand = net->track_demand;

        const ComponentInstance* src = architecture_find_component(arch, net->source_comp_id);
        const ComponentInstance* tgt = architecture_find_component(arch, net->target_comp_id);

        if (!src || !tgt) {
            rec->routed = false;
            snprintf(rec->fail_reason, MAX_NAME_LEN, "Endpoint component not found");
            continue;
        }

        Point2D p_src, p_tgt;
        component_get_port_location(src, net->source_port, &p_src);
        component_get_port_location(tgt, net->target_port, &p_tgt);

        int start_node = -1;
        int target_node = -1;
        routing_graph_snap_point(graph, p_src.x, p_src.y, &start_node);
        routing_graph_snap_point(graph, p_tgt.x, p_tgt.y, &target_node);

        RoutePath path;
        int p_iters = 0;
        int a_iters = 0;
        bool ok = physarum_route_single_net(graph, start_node, target_node, params, net->track_demand, &path, &p_iters, &a_iters);
        total_p_iters += p_iters;

        if (ok && path.is_valid) {
            rec->routed = true;
            rec->path = path;
            rec->wirelength = path.total_length;
            rec->bends = path.total_bends;
            rec->max_edge_congestion = path.peak_congestion;
            routing_graph_reserve_capacity(graph, &path, net->track_demand);
        } else {
            rec->routed = false;
            strncpy(rec->fail_reason, path.fail_reason[0] ? path.fail_reason : "Physarum adaptation failed", MAX_NAME_LEN - 1);
        }
    }

    free(sorted_nets);
    out_results->runtime_ms = get_time_ms() - t_start;
    out_results->total_iterations = total_p_iters;
    metrics_calculate_aggregates(out_results, graph);
}

int main(int argc, char** argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("PhysarumChip C11 Routing Engine\n");
        printf("Usage: %s [mode]\n", argv[0]);
        printf("Modes:\n");
        printf("  --generate-arch       Generate architecture and initial placement from config JSON\n");
        printf("  --validate-placement  Validate component placement inside die and check overlap\n");
        printf("  --route-physarum      Run Physarum-inspired adaptive flow routing\n");
        printf("  --route-dijkstra      Run Dijkstra baseline routing\n");
        printf("  --compare             Run both algorithms and produce side-by-side comparison\n");
        printf("  --version             Print version\n");
        fflush(stdout);
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("{\"version\": \"1.0.0\", \"engine\": \"PhysarumChip C11 Core\"}\n");
        fflush(stdout);
        return 0;
    }

    char* stdin_str = read_stdin_all();
    cJSON* input_json = NULL;
    if (stdin_str && strlen(stdin_str) > 0) {
        input_json = cJSON_Parse(stdin_str);
    }

    char err_buf[512] = {0};

    // Mode: Generate Architecture
    if (strcmp(argv[1], "--generate-arch") == 0) {
        ArchConfig cfg = architecture_default_config();
        if (input_json) {
            json_parse_arch_config(input_json, &cfg, err_buf, sizeof(err_buf));
        }

        Architecture* arch = (Architecture*)calloc(1, sizeof(Architecture));
        StatusCode sc = architecture_generate(&cfg, arch, err_buf, sizeof(err_buf));
        if (sc != STATUS_OK) {
            cJSON* resp = json_serialize_error(sc, err_buf);
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);
            free(arch);
            if (input_json) cJSON_Delete(input_json);
            if (stdin_str) free(stdin_str);
            return 1;
        }

        cJSON* resp = json_serialize_architecture(arch);
        char* out = cJSON_PrintUnformatted(resp);
        printf("%s\n", out);
        fflush(stdout);
        free(out);
        cJSON_Delete(resp);
        free(arch);
    }
    // Mode: Validate Placement
    else if (strcmp(argv[1], "--validate-placement") == 0) {
        if (!input_json) {
            cJSON* resp = json_serialize_error(STATUS_ERROR_JSON, "Missing JSON input");
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);
            if (stdin_str) free(stdin_str);
            return 1;
        }

        Architecture* arch = (Architecture*)calloc(1, sizeof(Architecture));
        if (!json_parse_architecture(input_json, arch, err_buf, sizeof(err_buf))) {
            cJSON* resp = json_serialize_error(STATUS_ERROR_JSON, err_buf);
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);
            cJSON_Delete(input_json);
            free(arch);
            if (stdin_str) free(stdin_str);
            return 1;
        }

        double clearance = 1.0;
        const cJSON* cl_item = cJSON_GetObjectItemCaseSensitive(input_json, "min_clearance");
        if (cJSON_IsNumber(cl_item)) clearance = cl_item->valuedouble;

        StatusCode sc = placement_validate(arch, clearance, err_buf, sizeof(err_buf));
        cJSON* resp = (sc == STATUS_OK) ? json_serialize_success("Placement is valid") : json_serialize_error(sc, err_buf);
        char* out = cJSON_PrintUnformatted(resp);
        printf("%s\n", out);
        fflush(stdout);
        free(out);
        cJSON_Delete(resp);
        free(arch);
    }
    // Mode: Route Physarum or Dijkstra or Compare
    else if (strcmp(argv[1], "--route-physarum") == 0 ||
             strcmp(argv[1], "--route-dijkstra") == 0 ||
             strcmp(argv[1], "--compare") == 0) {

        if (!input_json) {
            cJSON* resp = json_serialize_error(STATUS_ERROR_JSON, "Missing JSON input payload");
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);
            if (stdin_str) free(stdin_str);
            return 1;
        }

        Architecture* arch = (Architecture*)calloc(1, sizeof(Architecture));
        const cJSON* arch_json = cJSON_GetObjectItemCaseSensitive(input_json, "architecture");
        const cJSON* target_arch_json = arch_json ? arch_json : input_json;

        if (!json_parse_architecture(target_arch_json, arch, err_buf, sizeof(err_buf))) {
            cJSON* resp = json_serialize_error(STATUS_ERROR_JSON, err_buf);
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);
            cJSON_Delete(input_json);
            free(arch);
            if (stdin_str) free(stdin_str);
            return 1;
        }

        RoutingConstraints d_constraints = dijkstra_default_constraints();
        PhysarumParams p_params = physarum_default_params();
        int edge_cap = 4;
        Rect2D blocked[MAX_OBSTACLES];
        int blocked_count = 0;

        const cJSON* params_json = cJSON_GetObjectItemCaseSensitive(input_json, "routing_params");
        json_parse_routing_params(params_json ? params_json : input_json,
                                  &d_constraints, &p_params, &edge_cap,
                                  blocked, &blocked_count, err_buf, sizeof(err_buf));

        // Create routing graph
        RoutingGraph* graph = routing_graph_create(arch->config.die_width, arch->config.die_height, arch->config.routing_pitch, edge_cap);
        if (!graph) {
            cJSON* resp = json_serialize_error(STATUS_ERROR_ALLOCATION_FAILED, "Failed to create routing grid graph");
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);
            cJSON_Delete(input_json);
            free(arch);
            if (stdin_str) free(stdin_str);
            return 1;
        }

        routing_graph_build_from_arch(graph, arch, blocked, blocked_count);

        if (strcmp(argv[1], "--route-physarum") == 0) {
            RoutingResults* res = (RoutingResults*)calloc(1, sizeof(RoutingResults));
            execute_physarum_routing(graph, arch, &p_params, res);
            cJSON* resp = json_serialize_routing_results(res, graph);
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);
            free(res);
        } else if (strcmp(argv[1], "--route-dijkstra") == 0) {
            RoutingResults* res = (RoutingResults*)calloc(1, sizeof(RoutingResults));
            execute_dijkstra_routing(graph, arch, &d_constraints, res);
            cJSON* resp = json_serialize_routing_results(res, graph);
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);
            free(res);
        } else { // --compare
            RoutingResults* p_res = (RoutingResults*)calloc(1, sizeof(RoutingResults));
            execute_physarum_routing(graph, arch, &p_params, p_res);

            RoutingGraph* d_graph = routing_graph_create(arch->config.die_width, arch->config.die_height, arch->config.routing_pitch, edge_cap);
            routing_graph_build_from_arch(d_graph, arch, blocked, blocked_count);

            RoutingResults* d_res = (RoutingResults*)calloc(1, sizeof(RoutingResults));
            execute_dijkstra_routing(d_graph, arch, &d_constraints, d_res);

            ComparisonReport comp = metrics_compare(p_res, d_res);
            cJSON* resp = json_serialize_comparison(&comp, graph, d_graph);
            char* out = cJSON_PrintUnformatted(resp);
            printf("%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(resp);

            routing_graph_free(d_graph);
            free(p_res);
            free(d_res);
        }

        routing_graph_free(graph);
        free(arch);
    } else {
        cJSON* resp = json_serialize_error(STATUS_ERROR_INVALID_PARAM, "Unrecognized mode");
        char* out = cJSON_PrintUnformatted(resp);
        printf("%s\n", out);
        fflush(stdout);
        free(out);
        cJSON_Delete(resp);
    }

    if (input_json) cJSON_Delete(input_json);
    if (stdin_str) free(stdin_str);
    return 0;
}
