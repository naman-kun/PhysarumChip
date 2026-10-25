#include "../include/json_io.h"

bool json_parse_arch_config(const cJSON* root, ArchConfig* out_cfg, char* err_msg, size_t err_size) {
    if (!root || !out_cfg) {
        if (err_msg) snprintf(err_msg, err_size, "Missing JSON root or config pointer");
        return false;
    }

    *out_cfg = architecture_default_config();

    const cJSON* item = cJSON_GetObjectItemCaseSensitive(root, "num_p_cores");
    if (cJSON_IsNumber(item)) out_cfg->num_p_cores = item->valueint;

    item = cJSON_GetObjectItemCaseSensitive(root, "num_e_cores");
    if (cJSON_IsNumber(item)) out_cfg->num_e_cores = item->valueint;

    item = cJSON_GetObjectItemCaseSensitive(root, "e_cluster_size");
    if (cJSON_IsNumber(item)) out_cfg->e_cluster_size = item->valueint;

    item = cJSON_GetObjectItemCaseSensitive(root, "num_llc_slices");
    if (cJSON_IsNumber(item)) out_cfg->num_llc_slices = item->valueint;

    item = cJSON_GetObjectItemCaseSensitive(root, "num_mem_ctrls");
    if (cJSON_IsNumber(item)) out_cfg->num_mem_ctrls = item->valueint;

    item = cJSON_GetObjectItemCaseSensitive(root, "has_system_agent");
    if (cJSON_IsBool(item)) out_cfg->has_system_agent = cJSON_IsTrue(item);

    item = cJSON_GetObjectItemCaseSensitive(root, "has_pcie_io");
    if (cJSON_IsBool(item)) out_cfg->has_pcie_io = cJSON_IsTrue(item);

    item = cJSON_GetObjectItemCaseSensitive(root, "topology");
    if (cJSON_IsString(item) && item->valuestring) {
        out_cfg->topology = topology_from_string(item->valuestring);
    }

    item = cJSON_GetObjectItemCaseSensitive(root, "die_width");
    if (cJSON_IsNumber(item) && item->valuedouble > 0) out_cfg->die_width = item->valuedouble;

    item = cJSON_GetObjectItemCaseSensitive(root, "die_height");
    if (cJSON_IsNumber(item) && item->valuedouble > 0) out_cfg->die_height = item->valuedouble;

    item = cJSON_GetObjectItemCaseSensitive(root, "routing_pitch");
    if (cJSON_IsNumber(item) && item->valuedouble > 0) out_cfg->routing_pitch = item->valuedouble;

    return true;
}

bool json_parse_architecture(const cJSON* root, Architecture* out_arch, char* err_msg, size_t err_size) {
    if (!root || !out_arch) {
        if (err_msg) snprintf(err_msg, err_size, "Missing JSON root or arch pointer");
        return false;
    }

    memset(out_arch, 0, sizeof(Architecture));

    // 1. Config
    const cJSON* cfg_json = cJSON_GetObjectItemCaseSensitive(root, "config");
    if (cfg_json) {
        if (!json_parse_arch_config(cfg_json, &out_arch->config, err_msg, err_size)) return false;
    } else {
        out_arch->config = architecture_default_config();
    }

    // 2. Components
    const cJSON* comps_arr = cJSON_GetObjectItemCaseSensitive(root, "components");
    if (!comps_arr || !cJSON_IsArray(comps_arr) || cJSON_GetArraySize(comps_arr) == 0) {
        if (err_msg) snprintf(err_msg, err_size, "Architecture payload must contain a valid components array");
        return false;
    }

    int count = cJSON_GetArraySize(comps_arr);
    if (count > MAX_COMPONENTS) count = MAX_COMPONENTS;

    for (int i = 0; i < count; ++i) {
            const cJSON* cj = cJSON_GetArrayItem(comps_arr, i);
            ComponentInstance* c = &out_arch->components[out_arch->component_count++];

            const cJSON* it = cJSON_GetObjectItemCaseSensitive(cj, "id");
            if (cJSON_IsNumber(it)) c->id = it->valueint;

            it = cJSON_GetObjectItemCaseSensitive(cj, "name");
            if (cJSON_IsString(it) && it->valuestring) {
                strncpy(c->name, it->valuestring, MAX_NAME_LEN - 1);
            }

            it = cJSON_GetObjectItemCaseSensitive(cj, "type");
            if (cJSON_IsString(it) && it->valuestring) {
                c->type = component_type_from_string(it->valuestring);
            }

            it = cJSON_GetObjectItemCaseSensitive(cj, "cluster_id");
            if (cJSON_IsNumber(it)) c->cluster_id = it->valueint;

            const cJSON* bj = cJSON_GetObjectItemCaseSensitive(cj, "bounds");
            if (bj) {
                const cJSON* coord = cJSON_GetObjectItemCaseSensitive(bj, "x");
                if (cJSON_IsNumber(coord)) c->bounds.x = coord->valuedouble;
                coord = cJSON_GetObjectItemCaseSensitive(bj, "y");
                if (cJSON_IsNumber(coord)) c->bounds.y = coord->valuedouble;
                coord = cJSON_GetObjectItemCaseSensitive(bj, "width");
                if (cJSON_IsNumber(coord)) c->bounds.width = coord->valuedouble;
                coord = cJSON_GetObjectItemCaseSensitive(bj, "height");
                if (cJSON_IsNumber(coord)) c->bounds.height = coord->valuedouble;
            }

            it = cJSON_GetObjectItemCaseSensitive(cj, "power_estimate");
            if (cJSON_IsNumber(it)) c->power_estimate = it->valuedouble;

            it = cJSON_GetObjectItemCaseSensitive(cj, "area_estimate");
            if (cJSON_IsNumber(it)) c->area_estimate = it->valuedouble;

            // Ports
            const cJSON* ports_arr = cJSON_GetObjectItemCaseSensitive(cj, "ports");
            if (cJSON_IsArray(ports_arr)) {
                int p_count = cJSON_GetArraySize(ports_arr);
                if (p_count > MAX_PORTS_PER_COMP) p_count = MAX_PORTS_PER_COMP;
                c->port_count = p_count;
                for (int p = 0; p < p_count; ++p) {
                    const cJSON* pj = cJSON_GetArrayItem(ports_arr, p);
                    Port* port = &c->ports[p];
                    const cJSON* pit = cJSON_GetObjectItemCaseSensitive(pj, "name");
                    if (cJSON_IsString(pit) && pit->valuestring) {
                        strncpy(port->name, pit->valuestring, MAX_NAME_LEN - 1);
                    }
                    pit = cJSON_GetObjectItemCaseSensitive(pj, "rel_x");
                    if (cJSON_IsNumber(pit)) port->rel_x = pit->valuedouble;
                    pit = cJSON_GetObjectItemCaseSensitive(pj, "rel_y");
                    if (cJSON_IsNumber(pit)) port->rel_y = pit->valuedouble;
                }
            } else {
                // If ports array not explicitly given, copy from template
                const ComponentTemplate* tmpl = component_template_get(c->type);
                if (tmpl) {
                    c->port_count = tmpl->port_count;
                    for (int p = 0; p < tmpl->port_count; ++p) {
                        c->ports[p] = tmpl->ports[p];
                    }
                }
            }

            component_update_port_positions(c);
        }

    // 3. Nets
    const cJSON* nets_arr = cJSON_GetObjectItemCaseSensitive(root, "nets");
    if (cJSON_IsArray(nets_arr)) {
        int count = cJSON_GetArraySize(nets_arr);
        if (count > MAX_NETS) count = MAX_NETS;

        for (int i = 0; i < count; ++i) {
            const cJSON* nj = cJSON_GetArrayItem(nets_arr, i);
            LogicalNet* n = &out_arch->nets[out_arch->net_count++];

            const cJSON* it = cJSON_GetObjectItemCaseSensitive(nj, "id");
            if (cJSON_IsNumber(it)) n->id = it->valueint;

            it = cJSON_GetObjectItemCaseSensitive(nj, "name");
            if (cJSON_IsString(it) && it->valuestring) {
                strncpy(n->name, it->valuestring, MAX_NAME_LEN - 1);
            }

            it = cJSON_GetObjectItemCaseSensitive(nj, "source_comp_id");
            if (cJSON_IsNumber(it)) n->source_comp_id = it->valueint;

            it = cJSON_GetObjectItemCaseSensitive(nj, "source_port");
            if (cJSON_IsString(it) && it->valuestring) {
                strncpy(n->source_port, it->valuestring, MAX_NAME_LEN - 1);
            }

            it = cJSON_GetObjectItemCaseSensitive(nj, "target_comp_id");
            if (cJSON_IsNumber(it)) n->target_comp_id = it->valueint;

            it = cJSON_GetObjectItemCaseSensitive(nj, "target_port");
            if (cJSON_IsString(it) && it->valuestring) {
                strncpy(n->target_port, it->valuestring, MAX_NAME_LEN - 1);
            }

            it = cJSON_GetObjectItemCaseSensitive(nj, "type");
            if (cJSON_IsString(it) && it->valuestring) {
                n->type = net_type_from_string(it->valuestring);
            }

            it = cJSON_GetObjectItemCaseSensitive(nj, "priority");
            if (cJSON_IsNumber(it)) n->priority = it->valueint;
            else n->priority = 3;

            it = cJSON_GetObjectItemCaseSensitive(nj, "track_demand");
            if (cJSON_IsNumber(it)) n->track_demand = it->valueint;
            else n->track_demand = 1;

            it = cJSON_GetObjectItemCaseSensitive(nj, "max_length");
            if (cJSON_IsNumber(it)) n->max_length = it->valuedouble;
        }
    }

    return true;
}

bool json_parse_routing_params(const cJSON* root,
                              RoutingConstraints* out_dijkstra,
                              PhysarumParams* out_physarum,
                              int* out_capacity,
                              Rect2D* out_blocked,
                              int* out_blocked_count,
                              char* err_msg,
                              size_t err_size) {
    (void)err_msg; (void)err_size;
    if (out_dijkstra) *out_dijkstra = dijkstra_default_constraints();
    if (out_physarum) *out_physarum = physarum_default_params();
    if (out_capacity) *out_capacity = 4;
    if (out_blocked_count) *out_blocked_count = 0;

    if (!root) return true;

    // Capacity
    const cJSON* cap_item = cJSON_GetObjectItemCaseSensitive(root, "default_edge_capacity");
    if (cJSON_IsNumber(cap_item) && out_capacity) {
        *out_capacity = cap_item->valueint;
    }

    // Dijkstra params
    const cJSON* dj_item = cJSON_GetObjectItemCaseSensitive(root, "dijkstra_constraints");
    if (dj_item && out_dijkstra) {
        const cJSON* it = cJSON_GetObjectItemCaseSensitive(dj_item, "bend_penalty");
        if (cJSON_IsNumber(it)) out_dijkstra->bend_penalty = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(dj_item, "congestion_weight");
        if (cJSON_IsNumber(it)) out_dijkstra->congestion_weight = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(dj_item, "enforce_hard_capacity");
        if (cJSON_IsBool(it)) out_dijkstra->enforce_hard_capacity = cJSON_IsTrue(it);
    }

    // Physarum params
    const cJSON* ph_item = cJSON_GetObjectItemCaseSensitive(root, "physarum_params");
    if (ph_item && out_physarum) {
        const cJSON* it = cJSON_GetObjectItemCaseSensitive(ph_item, "rho");
        if (cJSON_IsNumber(it)) out_physarum->rho = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "alpha");
        if (cJSON_IsNumber(it)) out_physarum->alpha = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "d_min");
        if (cJSON_IsNumber(it)) out_physarum->d_min = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "d_max");
        if (cJSON_IsNumber(it)) out_physarum->d_max = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "source_current");
        if (cJSON_IsNumber(it)) out_physarum->source_current = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "max_pressure_iters");
        if (cJSON_IsNumber(it)) out_physarum->max_pressure_iters = it->valueint;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "adaptation_steps");
        if (cJSON_IsNumber(it)) out_physarum->adaptation_steps = it->valueint;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "congestion_weight");
        if (cJSON_IsNumber(it)) out_physarum->congestion_weight = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "bend_penalty");
        if (cJSON_IsNumber(it)) out_physarum->bend_penalty = it->valuedouble;

        it = cJSON_GetObjectItemCaseSensitive(ph_item, "enforce_hard_capacity");
        if (cJSON_IsBool(it)) out_physarum->enforce_hard_capacity = cJSON_IsTrue(it);
    }

    // Blocked regions
    const cJSON* blk_arr = cJSON_GetObjectItemCaseSensitive(root, "blocked_regions");
    if (cJSON_IsArray(blk_arr) && out_blocked && out_blocked_count) {
        int cnt = cJSON_GetArraySize(blk_arr);
        if (cnt > MAX_OBSTACLES) cnt = MAX_OBSTACLES;
        *out_blocked_count = cnt;
        for (int i = 0; i < cnt; ++i) {
            const cJSON* bj = cJSON_GetArrayItem(blk_arr, i);
            Rect2D* r = &out_blocked[i];
            const cJSON* it = cJSON_GetObjectItemCaseSensitive(bj, "x");
            if (cJSON_IsNumber(it)) r->x = it->valuedouble;
            it = cJSON_GetObjectItemCaseSensitive(bj, "y");
            if (cJSON_IsNumber(it)) r->y = it->valuedouble;
            it = cJSON_GetObjectItemCaseSensitive(bj, "width");
            if (cJSON_IsNumber(it)) r->width = it->valuedouble;
            it = cJSON_GetObjectItemCaseSensitive(bj, "height");
            if (cJSON_IsNumber(it)) r->height = it->valuedouble;
        }
    }

    return true;
}

cJSON* json_serialize_architecture(const Architecture* arch) {
    if (!arch) return NULL;
    cJSON* root = cJSON_CreateObject();

    // Config
    cJSON* cfg = cJSON_CreateObject();
    cJSON_AddNumberToObject(cfg, "num_p_cores", arch->config.num_p_cores);
    cJSON_AddNumberToObject(cfg, "num_e_cores", arch->config.num_e_cores);
    cJSON_AddNumberToObject(cfg, "e_cluster_size", arch->config.e_cluster_size);
    cJSON_AddNumberToObject(cfg, "num_llc_slices", arch->config.num_llc_slices);
    cJSON_AddNumberToObject(cfg, "num_mem_ctrls", arch->config.num_mem_ctrls);
    cJSON_AddBoolToObject(cfg, "has_system_agent", arch->config.has_system_agent);
    cJSON_AddBoolToObject(cfg, "has_pcie_io", arch->config.has_pcie_io);
    cJSON_AddStringToObject(cfg, "topology", topology_to_string(arch->config.topology));
    cJSON_AddNumberToObject(cfg, "die_width", arch->config.die_width);
    cJSON_AddNumberToObject(cfg, "die_height", arch->config.die_height);
    cJSON_AddNumberToObject(cfg, "routing_pitch", arch->config.routing_pitch);
    cJSON_AddItemToObject(root, "config", cfg);

    cJSON_AddNumberToObject(root, "total_estimated_area", arch->total_estimated_area);
    cJSON_AddNumberToObject(root, "total_estimated_power", arch->total_estimated_power);

    // Components
    cJSON* comps_arr = cJSON_CreateArray();
    for (int i = 0; i < arch->component_count; ++i) {
        const ComponentInstance* c = &arch->components[i];
        cJSON* cj = cJSON_CreateObject();
        cJSON_AddNumberToObject(cj, "id", c->id);
        cJSON_AddStringToObject(cj, "name", c->name);
        cJSON_AddStringToObject(cj, "type", component_type_to_string(c->type));
        cJSON_AddNumberToObject(cj, "cluster_id", c->cluster_id);
        cJSON_AddNumberToObject(cj, "orientation", c->orientation);
        cJSON_AddNumberToObject(cj, "power_estimate", c->power_estimate);
        cJSON_AddNumberToObject(cj, "area_estimate", c->area_estimate);

        cJSON* b = cJSON_CreateObject();
        cJSON_AddNumberToObject(b, "x", c->bounds.x);
        cJSON_AddNumberToObject(b, "y", c->bounds.y);
        cJSON_AddNumberToObject(b, "width", c->bounds.width);
        cJSON_AddNumberToObject(b, "height", c->bounds.height);
        cJSON_AddItemToObject(cj, "bounds", b);

        cJSON* ports_arr = cJSON_CreateArray();
        for (int p = 0; p < c->port_count; ++p) {
            const Port* port = &c->ports[p];
            cJSON* pj = cJSON_CreateObject();
            cJSON_AddStringToObject(pj, "name", port->name);
            cJSON_AddNumberToObject(pj, "rel_x", port->rel_x);
            cJSON_AddNumberToObject(pj, "rel_y", port->rel_y);
            cJSON_AddNumberToObject(pj, "abs_x", port->abs_x);
            cJSON_AddNumberToObject(pj, "abs_y", port->abs_y);
            cJSON_AddItemToArray(ports_arr, pj);
        }
        cJSON_AddItemToObject(cj, "ports", ports_arr);

        cJSON_AddItemToArray(comps_arr, cj);
    }
    cJSON_AddItemToObject(root, "components", comps_arr);

    // Nets
    cJSON* nets_arr = cJSON_CreateArray();
    for (int i = 0; i < arch->net_count; ++i) {
        const LogicalNet* n = &arch->nets[i];
        cJSON* nj = cJSON_CreateObject();
        cJSON_AddNumberToObject(nj, "id", n->id);
        cJSON_AddStringToObject(nj, "name", n->name);
        cJSON_AddNumberToObject(nj, "source_comp_id", n->source_comp_id);
        cJSON_AddStringToObject(nj, "source_port", n->source_port);
        cJSON_AddNumberToObject(nj, "target_comp_id", n->target_comp_id);
        cJSON_AddStringToObject(nj, "target_port", n->target_port);
        cJSON_AddStringToObject(nj, "type", net_type_to_string(n->type));
        cJSON_AddNumberToObject(nj, "priority", n->priority);
        cJSON_AddNumberToObject(nj, "track_demand", n->track_demand);
        cJSON_AddNumberToObject(nj, "max_length", n->max_length);
        cJSON_AddItemToArray(nets_arr, nj);
    }
    cJSON_AddItemToObject(root, "nets", nets_arr);

    return root;
}

cJSON* json_serialize_routing_results(const RoutingResults* results, const RoutingGraph* graph) {
    if (!results) return NULL;
    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "algorithm", results->algorithm_name);

    // Summary
    cJSON* sum = cJSON_CreateObject();
    cJSON_AddNumberToObject(sum, "total_nets", results->total_nets);
    cJSON_AddNumberToObject(sum, "routed_nets", results->routed_nets);
    cJSON_AddNumberToObject(sum, "unrouted_nets", results->unrouted_nets);
    cJSON_AddNumberToObject(sum, "success_rate", results->success_rate);
    cJSON_AddNumberToObject(sum, "total_wirelength", results->total_wirelength);
    cJSON_AddNumberToObject(sum, "avg_wirelength", results->avg_wirelength);
    cJSON_AddNumberToObject(sum, "peak_congestion", results->peak_congestion);
    cJSON_AddNumberToObject(sum, "avg_congestion", results->avg_congestion);
    cJSON_AddNumberToObject(sum, "congested_edge_count", results->congested_edge_count);
    cJSON_AddNumberToObject(sum, "capacity_violations", results->capacity_violations);
    cJSON_AddNumberToObject(sum, "length_violations", results->length_violations);
    cJSON_AddNumberToObject(sum, "total_bends", results->total_bends);
    cJSON_AddNumberToObject(sum, "abstract_total_cost", results->abstract_total_cost);
    cJSON_AddNumberToObject(sum, "runtime_ms", results->runtime_ms);
    cJSON_AddNumberToObject(sum, "total_iterations", results->total_iterations);
    cJSON_AddItemToObject(root, "summary", sum);

    // Routes
    cJSON* routes_arr = cJSON_CreateArray();
    for (int i = 0; i < results->total_nets; ++i) {
        const NetRouteRecord* nr = &results->net_records[i];
        cJSON* rj = cJSON_CreateObject();
        cJSON_AddNumberToObject(rj, "net_id", nr->net_id);
        cJSON_AddStringToObject(rj, "net_name", nr->net_name);
        cJSON_AddBoolToObject(rj, "routed", nr->routed);
        cJSON_AddNumberToObject(rj, "demand", nr->demand);
        cJSON_AddNumberToObject(rj, "wirelength", nr->wirelength);
        cJSON_AddNumberToObject(rj, "bends", nr->bends);
        cJSON_AddNumberToObject(rj, "max_edge_congestion", nr->max_edge_congestion);

        if (!nr->routed) {
            cJSON_AddStringToObject(rj, "fail_reason", nr->fail_reason);
        }

        if (nr->routed && graph) {
            cJSON* path_arr = cJSON_CreateArray();
            for (int k = 0; k < nr->path.node_count; ++k) {
                int n_idx = nr->path.nodes[k];
                if (n_idx >= 0 && n_idx < graph->node_count) {
                    const RoutingNode* n = &graph->nodes[n_idx];
                    cJSON* pt = cJSON_CreateObject();
                    cJSON_AddNumberToObject(pt, "gx", n->gx);
                    cJSON_AddNumberToObject(pt, "gy", n->gy);
                    cJSON_AddNumberToObject(pt, "x", n->world_x);
                    cJSON_AddNumberToObject(pt, "y", n->world_y);
                    cJSON_AddItemToArray(path_arr, pt);
                }
            }
            cJSON_AddItemToObject(rj, "path", path_arr);
        }

        cJSON_AddItemToArray(routes_arr, rj);
    }
    cJSON_AddItemToObject(root, "routes", routes_arr);

    // Congested segments for visual heatmap in canvas
    if (graph) {
        cJSON* cong_arr = cJSON_CreateArray();
        for (int e = 0; e < graph->edge_count; ++e) {
            const RoutingEdge* edge = &graph->edges[e];
            if (edge->demand > 0) {
                cJSON* ej = cJSON_CreateObject();
                const RoutingNode* nu = &graph->nodes[edge->u];
                const RoutingNode* nv = &graph->nodes[edge->v];
                cJSON_AddNumberToObject(ej, "x1", nu->world_x);
                cJSON_AddNumberToObject(ej, "y1", nu->world_y);
                cJSON_AddNumberToObject(ej, "x2", nv->world_x);
                cJSON_AddNumberToObject(ej, "y2", nv->world_y);
                cJSON_AddNumberToObject(ej, "demand", edge->demand);
                cJSON_AddNumberToObject(ej, "capacity", edge->capacity);
                cJSON_AddNumberToObject(ej, "ratio", (double)edge->demand / (double)edge->capacity);
                cJSON_AddItemToArray(cong_arr, ej);
            }
        }
        cJSON_AddItemToObject(root, "active_segments", cong_arr);
    }

    return root;
}

cJSON* json_serialize_comparison(const ComparisonReport* report, const RoutingGraph* physarum_graph, const RoutingGraph* dijkstra_graph) {
    if (!report) return NULL;
    cJSON* root = cJSON_CreateObject();

    cJSON* ph_json = json_serialize_routing_results(&report->physarum_results, physarum_graph);
    cJSON* dj_json = json_serialize_routing_results(&report->dijkstra_results, dijkstra_graph);
    cJSON_AddItemToObject(root, "physarum", ph_json);
    cJSON_AddItemToObject(root, "dijkstra", dj_json);

    cJSON* diff = cJSON_CreateObject();
    cJSON_AddNumberToObject(diff, "wirelength_diff_pct", report->wirelength_diff_pct);
    cJSON_AddNumberToObject(diff, "peak_congestion_diff", report->peak_congestion_diff);
    cJSON_AddNumberToObject(diff, "avg_congestion_diff", report->avg_congestion_diff);
    cJSON_AddNumberToObject(diff, "runtime_diff_ms", report->runtime_diff_ms);
    cJSON_AddItemToObject(root, "differences", diff);

    return root;
}

cJSON* json_serialize_error(StatusCode code, const char* message) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", false);
    cJSON_AddNumberToObject(root, "status_code", (int)code);
    cJSON_AddStringToObject(root, "error", message ? message : "Unknown error");
    return root;
}

cJSON* json_serialize_success(const char* message) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddNumberToObject(root, "status_code", 0);
    cJSON_AddStringToObject(root, "message", message ? message : "Operation succeeded");
    return root;
}
