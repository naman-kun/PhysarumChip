#include <assert.h>
#include "../include/common.h"
#include "../include/component.h"
#include "../include/architecture.h"
#include "../include/placement.h"
#include "../include/graph.h"
#include "../include/dijkstra.h"
#include "../include/physarum.h"
#include "../include/metrics.h"
#include "../include/json_io.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (!(cond)) { \
        printf("FAILED: %s (line %d): %s\n", __func__, __LINE__, msg); \
        return; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

// 1. Test: Component template library
static void test_component_templates(void) {
    component_library_init();
    const ComponentTemplate* tmpl = component_template_get(COMP_P_CORE);
    TEST_ASSERT(tmpl != NULL, "P-Core template should exist");
    TEST_ASSERT(tmpl->default_width > 0 && tmpl->default_height > 0, "P-Core dimensions should be positive");
    TEST_ASSERT(tmpl->port_count >= 3, "P-Core should define at least 3 ports");

    tmpl = component_template_get(COMP_E_CORE);
    TEST_ASSERT(tmpl != NULL, "E-Core template should exist");

    tmpl = component_template_get(COMP_LLC_SLICE);
    TEST_ASSERT(tmpl != NULL, "LLC slice template should exist");
}

// 2. Test: Default 8P + 8E architecture generation
static void test_default_architecture_generation(void) {
    ArchConfig cfg = architecture_default_config();
    TEST_ASSERT(cfg.num_p_cores == 8, "Default num_p_cores should be 8");
    TEST_ASSERT(cfg.num_e_cores == 8, "Default num_e_cores should be 8");
    TEST_ASSERT(cfg.num_llc_slices == 4, "Default num_llc_slices should be 4");
    TEST_ASSERT(cfg.num_mem_ctrls == 2, "Default num_mem_ctrls should be 2");

    Architecture arch;
    char err[256] = {0};
    StatusCode sc = architecture_generate(&cfg, &arch, err, sizeof(err));
    if (sc != STATUS_OK) {
        printf("DEBUG test_default_architecture_generation failed with: %s (code %d)\n", err, (int)sc);
    }
    TEST_ASSERT(sc == STATUS_OK, "Default architecture generation should succeed");
    TEST_ASSERT(arch.component_count > 40, "Component count should reflect all cores, caches, controllers, routers");
    TEST_ASSERT(arch.net_count > 30, "Net count should connect cache hierarchy and fabric");

    // Verify cache counts: 8 private L2 caches for P-cores, 2 shared L2 caches for 8 E-cores (cluster size 4)
    int p_l2_count = 0;
    int e_l2_count = 0;
    for (int i = 0; i < arch.component_count; ++i) {
        if (arch.components[i].type == COMP_L2_CACHE) {
            if (arch.components[i].cluster_id >= 100) e_l2_count++;
            else p_l2_count++;
        }
    }
    TEST_ASSERT(p_l2_count == 8, "Must have exactly 8 private P-core L2 caches");
    TEST_ASSERT(e_l2_count == 2, "Must have exactly 2 shared E-cluster L2 caches for 8 E-cores with cluster size 4");
}

// 3. Test: Custom architecture settings & validation
static void test_custom_architecture_settings(void) {
    ArchConfig cfg = architecture_default_config();
    cfg.num_p_cores = 4;
    cfg.num_e_cores = 4;
    cfg.e_cluster_size = 2; // 2 clusters
    cfg.topology = TOPOLOGY_MESH_2D;

    Architecture arch;
    char err[256];
    StatusCode sc = architecture_generate(&cfg, &arch, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_OK, "Custom 4P + 4E architecture generation should succeed");

    // Test invalid config rejection
    cfg.num_p_cores = -1;
    sc = architecture_generate(&cfg, &arch, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_ERROR_INVALID_PARAM, "Negative core count must be rejected");

    cfg.num_p_cores = 8;
    cfg.die_width = 10.0; // Too small for components
    sc = architecture_validate_config(&cfg, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_ERROR_INVALID_PARAM, "Die size too small must be rejected");
}

// 4. Test: Placement bounds and non-overlap
static void test_placement_validation(void) {
    ArchConfig cfg = architecture_default_config();
    Architecture arch;
    char err[256];
    StatusCode sc = architecture_generate(&cfg, &arch, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_OK, "Architecture generation succeeded");

    sc = placement_validate(&arch, 0.5, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_OK, "Initial placement must be inside die bounds and non-overlapping");

    // Test intentional out of bounds
    arch.components[0].bounds.x = -5.0;
    sc = placement_validate(&arch, 0.5, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_ERROR_OUT_OF_BOUNDS, "Negative coordinate must trigger out of bounds");

    // Test intentional overlap
    arch.components[0].bounds.x = arch.components[1].bounds.x;
    arch.components[0].bounds.y = arch.components[1].bounds.y;
    sc = placement_validate(&arch, 0.5, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_ERROR_OVERLAP, "Co-located blocks must trigger overlap error");
}

// 5. Test: Component move checking
static void test_placement_move_check(void) {
    ArchConfig cfg = architecture_default_config();
    Architecture arch;
    char err[256];
    architecture_generate(&cfg, &arch, err, sizeof(err));

    // Try moving component 1 to an illegal overlapping location
    StatusCode sc = placement_check_move(&arch, arch.components[0].id,
                                         arch.components[1].bounds.x, arch.components[1].bounds.y,
                                         0.5, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_ERROR_OVERLAP, "Moving to another block's position must fail");

    // Try moving component 1 off the die
    sc = placement_check_move(&arch, arch.components[0].id, 999.0, 999.0, 0.5, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_ERROR_OUT_OF_BOUNDS, "Moving off die must fail");
}

// 6. Test: MinHeap data structure
static void test_min_heap(void) {
    MinHeap* h = min_heap_create(10);
    TEST_ASSERT(h != NULL, "Heap creation");
    TEST_ASSERT(min_heap_is_empty(h), "Initial heap empty");

    min_heap_insert_or_decrease(h, 1, 10.5);
    min_heap_insert_or_decrease(h, 2, 4.2);
    min_heap_insert_or_decrease(h, 3, 7.8);

    MinHeapNode min_node;
    bool ok = min_heap_extract_min(h, &min_node);
    TEST_ASSERT(ok && min_node.node == 2, "Extract min should return node 2 (dist 4.2)");

    min_heap_insert_or_decrease(h, 1, 1.0); // Decrease key for node 1
    ok = min_heap_extract_min(h, &min_node);
    TEST_ASSERT(ok && min_node.node == 1, "Extract min after decrease-key should return node 1 (dist 1.0)");

    min_heap_free(h);
}

// 7. Test: Grid graph creation & obstacle avoidance
static void test_routing_graph_and_obstacles(void) {
    RoutingGraph* g = routing_graph_create(20.0, 20.0, 1.0, 4);
    TEST_ASSERT(g != NULL, "Graph created");
    TEST_ASSERT(g->node_count == 21 * 21, "21x21 nodes for 20x20 die with pitch 1.0");

    // Add a blocked region in middle: x in [5, 15], y in [5, 15]
    Rect2D obs = {5.0, 5.0, 10.0, 10.0};
    Architecture empty_arch;
    memset(&empty_arch, 0, sizeof(Architecture));
    empty_arch.config.die_width = 20.0;
    empty_arch.config.die_height = 20.0;
    empty_arch.config.routing_pitch = 1.0;

    routing_graph_build_from_arch(g, &empty_arch, &obs, 1);

    // Node (10, 10) must be blocked
    int mid_idx = routing_graph_node_index(g, 10, 10);
    TEST_ASSERT(g->nodes[mid_idx].is_blocked == true, "Center obstacle node must be blocked");

    // Route from (2, 10) to (18, 10) using Dijkstra
    int s_idx = routing_graph_node_index(g, 2, 10);
    int t_idx = routing_graph_node_index(g, 18, 10);
    RoutingConstraints c = dijkstra_default_constraints();
    RoutePath path;
    bool ok = dijkstra_find_path(g, s_idx, t_idx, &c, 1, &path);
    TEST_ASSERT(ok && path.is_valid, "Route around obstacle must succeed");

    // Verify none of the path nodes intersect the obstacle
    for (int i = 0; i < path.node_count; ++i) {
        int u = path.nodes[i];
        TEST_ASSERT(g->nodes[u].gx < 5 || g->nodes[u].gx > 15 || g->nodes[u].gy < 5 || g->nodes[u].gy > 15,
                    "Path must skirt around the blocked rectangle");
    }

    routing_graph_free(g);
}

// 8. Test: Disconnected endpoints reported as unroutable
static void test_disconnected_endpoints(void) {
    RoutingGraph* g = routing_graph_create(10.0, 10.0, 1.0, 4);
    // Block a complete vertical cut at x = 5 (all y from 0 to 10)
    Rect2D cut = {4.8, 0.0, 0.4, 10.0};
    Architecture empty_arch;
    memset(&empty_arch, 0, sizeof(Architecture));
    routing_graph_build_from_arch(g, &empty_arch, &cut, 1);

    int s_idx = routing_graph_node_index(g, 1, 5);
    int t_idx = routing_graph_node_index(g, 8, 5);

    RoutingConstraints c = dijkstra_default_constraints();
    RoutePath path;
    bool ok = dijkstra_find_path(g, s_idx, t_idx, &c, 1, &path);
    TEST_ASSERT(!ok || !path.is_valid, "Disconnected path across barrier must fail");
    TEST_ASSERT(strlen(path.fail_reason) > 0, "Must give descriptive failure reason");

    routing_graph_free(g);
}

// 9. Test: Hard capacity enforcement
static void test_capacity_enforcement(void) {
    RoutingGraph* g = routing_graph_create(5.0, 5.0, 1.0, 2); // capacity = 2 tracks
    int s_idx = routing_graph_node_index(g, 0, 0);
    int t_idx = routing_graph_node_index(g, 1, 0);

    RoutingConstraints c = dijkstra_default_constraints();
    c.enforce_hard_capacity = true;

    // Fill capacity to maximum
    int edge_idx = routing_graph_find_edge(g, s_idx, t_idx);
    g->edges[edge_idx].demand = 2; // full capacity

    RoutePath path;
    // Trying to route demand = 1 with hard capacity on single edge graph segment
    // It should choose an alternate path if possible or fail if blocked
    // Block all alternate nodes so only (0,0)->(1,0) was possible
    g->nodes[routing_graph_node_index(g, 0, 1)].is_blocked = true;
    bool ok = dijkstra_find_path(g, s_idx, t_idx, &c, 1, &path);
    TEST_ASSERT(!ok || !path.is_valid, "Exceeding hard capacity when no alternative exists must fail");

    routing_graph_free(g);
}

// 10. Test: Physarum adaptive routing on small graph
static void test_physarum_routing_adaptation(void) {
    RoutingGraph* g = routing_graph_create(15.0, 15.0, 1.0, 4);
    int s_idx = routing_graph_node_index(g, 2, 7);
    int t_idx = routing_graph_node_index(g, 12, 7);

    PhysarumParams params = physarum_default_params();
    params.adaptation_steps = 10;
    RoutePath path;
    int p_iters = 0;
    int a_iters = 0;

    bool ok = physarum_route_single_net(g, s_idx, t_idx, &params, 1, &path, &p_iters, &a_iters);
    TEST_ASSERT(ok && path.is_valid, "Physarum routing must successfully find path on unobstructed grid");
    TEST_ASSERT(path.node_count >= 11, "Path node count should span grid distance");
    TEST_ASSERT(p_iters > 0, "Pressure relaxation iterations must be recorded");

    routing_graph_free(g);
}

// 11. Test: Full comparison pipeline
static void test_full_comparison_pipeline(void) {
    ArchConfig cfg = architecture_default_config();
    cfg.num_p_cores = 2; // Small config for fast unit testing
    cfg.num_e_cores = 2;
    cfg.num_llc_slices = 2;
    cfg.die_width = 80.0;
    cfg.die_height = 80.0;

    Architecture arch;
    char err[256];
    StatusCode sc = architecture_generate(&cfg, &arch, err, sizeof(err));
    TEST_ASSERT(sc == STATUS_OK, "Architecture generated");

    RoutingGraph* g = routing_graph_create(arch.config.die_width, arch.config.die_height, arch.config.routing_pitch, 4);
    routing_graph_build_from_arch(g, &arch, NULL, 0);

    RoutingConstraints dc = dijkstra_default_constraints();
    PhysarumParams pp = physarum_default_params();
    pp.adaptation_steps = 5;

    RoutingResults d_res;
    RoutingResults p_res;
    metrics_init_results(&d_res, "Dijkstra", arch.net_count);
    metrics_init_results(&p_res, "Physarum", arch.net_count);

    // Test first 3 nets
    for (int i = 0; i < 3 && i < arch.net_count; ++i) {
        const LogicalNet* net = &arch.nets[i];
        const ComponentInstance* src = architecture_find_component(&arch, net->source_comp_id);
        const ComponentInstance* tgt = architecture_find_component(&arch, net->target_comp_id);
        Point2D ps, pt;
        component_get_port_location(src, net->source_port, &ps);
        component_get_port_location(tgt, net->target_port, &pt);
        int sn, tn;
        routing_graph_snap_point(g, ps.x, ps.y, &sn);
        routing_graph_snap_point(g, pt.x, pt.y, &tn);

        RoutePath d_path, p_path;
        dijkstra_find_path(g, sn, tn, &dc, net->track_demand, &d_path);
        physarum_route_single_net(g, sn, tn, &pp, net->track_demand, &p_path, NULL, NULL);

        TEST_ASSERT(d_path.is_valid, "Dijkstra path valid");
        TEST_ASSERT(p_path.is_valid, "Physarum path valid");
    }

    routing_graph_free(g);
}

// 12. Test: JSON round-trip
static void test_json_roundtrip(void) {
    ArchConfig cfg = architecture_default_config();
    Architecture arch;
    char err[256];
    architecture_generate(&cfg, &arch, err, sizeof(err));

    cJSON* json = json_serialize_architecture(&arch);
    TEST_ASSERT(json != NULL, "Serialization succeeded");

    Architecture parsed;
    bool ok = json_parse_architecture(json, &parsed, err, sizeof(err));
    TEST_ASSERT(ok, "Parsing succeeded");
    TEST_ASSERT(parsed.component_count == arch.component_count, "Component counts match");
    TEST_ASSERT(parsed.net_count == arch.net_count, "Net counts match");

    cJSON_Delete(json);
}

int main(void) {
    printf("=== Starting PhysarumChip C11 Test Suite ===\n");

    test_component_templates();
    test_default_architecture_generation();
    test_custom_architecture_settings();
    test_placement_validation();
    test_placement_move_check();
    test_min_heap();
    test_routing_graph_and_obstacles();
    test_disconnected_endpoints();
    test_capacity_enforcement();
    test_physarum_routing_adaptation();
    test_full_comparison_pipeline();
    test_json_roundtrip();

    printf("\nTest Summary: %d / %d assertions PASSED\n", g_tests_passed, g_tests_run);
    if (g_tests_passed == g_tests_run) {
        printf("ALL TESTS PASSED SUCCESSFULLY!\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED!\n");
        return 1;
    }
}
