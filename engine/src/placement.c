#include "../include/placement.h"

PlacementParams placement_default_params(void) {
    PlacementParams p;
    p.min_clearance = 1.0;
    p.channel_width = 3.0;
    return p;
}

StatusCode placement_validate(const Architecture* arch, double min_clearance, char* err_msg, size_t err_size) {
    if (!arch) {
        if (err_msg) snprintf(err_msg, err_size, "Architecture is NULL");
        return STATUS_ERROR_INVALID_PARAM;
    }

    double dw = arch->config.die_width;
    double dh = arch->config.die_height;

    // 1. Boundary check
    for (int i = 0; i < arch->component_count; ++i) {
        const ComponentInstance* c = &arch->components[i];
        if (c->bounds.x < 0.0 || c->bounds.y < 0.0 ||
            (c->bounds.x + c->bounds.width) > dw ||
            (c->bounds.y + c->bounds.height) > dh) {
            if (err_msg) {
                snprintf(err_msg, err_size,
                         "Component '%s' (ID %d) out of die bounds [0,0,%.1f,%.1f]: pos=(%.1f, %.1f), size=(%.1f, %.1f)",
                         c->name, c->id, dw, dh, c->bounds.x, c->bounds.y, c->bounds.width, c->bounds.height);
            }
            return STATUS_ERROR_OUT_OF_BOUNDS;
        }
    }

    // 2. Overlap & clearance check
    for (int i = 0; i < arch->component_count; ++i) {
        const ComponentInstance* c1 = &arch->components[i];
        for (int j = i + 1; j < arch->component_count; ++j) {
            const ComponentInstance* c2 = &arch->components[j];

            bool x_overlap = (c1->bounds.x < c2->bounds.x + c2->bounds.width + min_clearance - 1e-6) &&
                             (c1->bounds.x + c1->bounds.width + min_clearance - 1e-6 > c2->bounds.x);
            bool y_overlap = (c1->bounds.y < c2->bounds.y + c2->bounds.height + min_clearance - 1e-6) &&
                             (c1->bounds.y + c1->bounds.height + min_clearance - 1e-6 > c2->bounds.y);

            if (x_overlap && y_overlap) {
                if (err_msg) {
                    snprintf(err_msg, err_size,
                             "Overlap / clearance violation between '%s' (%.1f,%.1f,%.1fx%.1f) and '%s' (%.1f,%.1f,%.1fx%.1f)",
                             c1->name, c1->bounds.x, c1->bounds.y, c1->bounds.width, c1->bounds.height,
                             c2->name, c2->bounds.x, c2->bounds.y, c2->bounds.width, c2->bounds.height);
                }
                return STATUS_ERROR_OVERLAP;
            }
        }
    }

    return STATUS_OK;
}

StatusCode placement_check_move(const Architecture* arch, int comp_id, double new_x, double new_y, double min_clearance, char* err_msg, size_t err_size) {
    if (!arch) {
        if (err_msg) snprintf(err_msg, err_size, "Architecture is NULL");
        return STATUS_ERROR_INVALID_PARAM;
    }

    const ComponentInstance* target = architecture_find_component(arch, comp_id);
    if (!target) {
        if (err_msg) snprintf(err_msg, err_size, "Component ID %d not found", comp_id);
        return STATUS_ERROR_INVALID_PARAM;
    }

    double dw = arch->config.die_width;
    double dh = arch->config.die_height;

    // Boundary check
    if (new_x < 0.0 || new_y < 0.0 || (new_x + target->bounds.width) > dw || (new_y + target->bounds.height) > dh) {
        if (err_msg) {
            snprintf(err_msg, err_size,
                     "Requested move puts '%s' out of bounds: pos=(%.1f, %.1f), size=(%.1f, %.1f), die=(%.1f, %.1f)",
                     target->name, new_x, new_y, target->bounds.width, target->bounds.height, dw, dh);
        }
        return STATUS_ERROR_OUT_OF_BOUNDS;
    }

    // Overlap with all other components
    for (int i = 0; i < arch->component_count; ++i) {
        const ComponentInstance* other = &arch->components[i];
        if (other->id == comp_id) continue;

        bool x_overlap = (new_x < other->bounds.x + other->bounds.width + min_clearance - 1e-6) &&
                         (new_x + target->bounds.width + min_clearance - 1e-6 > other->bounds.x);
        bool y_overlap = (new_y < other->bounds.y + other->bounds.height + min_clearance - 1e-6) &&
                         (new_y + target->bounds.height + min_clearance - 1e-6 > other->bounds.y);

        if (x_overlap && y_overlap) {
            if (err_msg) {
                snprintf(err_msg, err_size,
                         "Requested move causes overlap between '%s' and '%s' (at %.1f, %.1f)",
                         target->name, other->name, other->bounds.x, other->bounds.y);
            }
            return STATUS_ERROR_OVERLAP;
        }
    }

    return STATUS_OK;
}

StatusCode placement_auto_layout(Architecture* arch, const PlacementParams* params, char* err_msg, size_t err_size) {
    if (!arch) {
        if (err_msg) snprintf(err_msg, err_size, "Architecture is NULL");
        return STATUS_ERROR_INVALID_PARAM;
    }
    double cl = params ? params->min_clearance : 1.0;
    double ch = params ? params->channel_width : 2.0;

    double dw = arch->config.die_width;
    double dh = arch->config.die_height;

    // Check minimum die size requirements against total area
    if (arch->total_estimated_area > dw * dh * 0.85) {
        if (err_msg) {
            snprintf(err_msg, err_size, "Die area (%.0f) insufficient for component area (%.0f)", dw * dh, arch->total_estimated_area);
        }
        return STATUS_ERROR_OUT_OF_BOUNDS;
    }

    // 1. Top Edge: System Agent and MC 0 (y = 1.0, height 5..6, ends at 7.0)
    double top_x = 2.0;
    for (int i = 0; i < arch->component_count; ++i) {
        ComponentInstance* c = &arch->components[i];
        if (c->type == COMP_SYSTEM_AGENT) {
            c->bounds.x = top_x;
            c->bounds.y = 1.0;
            top_x += c->bounds.width + ch;
        } else if (c->type == COMP_MEM_CTRL && (c->id % 2 == 1 || arch->config.num_mem_ctrls == 1)) {
            c->bounds.x = top_x;
            c->bounds.y = 1.0;
            top_x += c->bounds.width + ch;
        }
    }

    // 2. Bottom Edge: PCIe/IO and MC 1 (y = dh - c->bounds.height - 1.0)
    double bot_x = 2.0;
    for (int i = 0; i < arch->component_count; ++i) {
        ComponentInstance* c = &arch->components[i];
        if (c->type == COMP_PCIE_IO) {
            c->bounds.x = bot_x;
            c->bounds.y = dh - c->bounds.height - 1.0;
            bot_x += c->bounds.width + ch;
        } else if (c->type == COMP_MEM_CTRL && c->id % 2 == 0 && arch->config.num_mem_ctrls > 1) {
            c->bounds.x = bot_x;
            c->bounds.y = dh - c->bounds.height - 1.0;
            bot_x += c->bounds.width + ch;
        }
    }

    // 3. P-Core Clusters (P-Core + L1I + L1D + L2 stacked vertically per column)
    // Cluster width = 10.0, pitch = 12.0
    int p_top_idx = 0;
    int p_bot_idx = 0;
    int half_p = (arch->config.num_p_cores + 1) / 2;

    for (int i = 0; i < arch->config.num_p_cores; ++i) {
        bool is_top = (i < half_p);
        int col = is_top ? p_top_idx++ : p_bot_idx++;

        double col_x = 2.0 + col * 12.0;
        double base_y = is_top ? 8.5 : 44.0;

        for (int c_idx = 0; c_idx < arch->component_count; ++c_idx) {
            ComponentInstance* c = &arch->components[c_idx];
            if (c->cluster_id == i) {
                if (c->type == COMP_L1_INST) {
                    c->bounds.x = col_x;
                    c->bounds.y = base_y;
                } else if (c->type == COMP_L1_DATA) {
                    c->bounds.x = col_x + 5.0;
                    c->bounds.y = base_y;
                } else if (c->type == COMP_P_CORE) {
                    c->bounds.x = col_x;
                    c->bounds.y = base_y + 4.0;
                } else if (c->type == COMP_L2_CACHE) {
                    c->bounds.x = col_x + 1.0;
                    c->bounds.y = base_y + 15.0;
                }
            }
        }
    }

    // 4. Central Spine (y = 31.0 .. 42.0): LLC Slices and Routers
    double llc_x = 2.0;
    double llc_y = 31.0;
    for (int i = 0; i < arch->component_count; ++i) {
        ComponentInstance* c = &arch->components[i];
        if (c->type == COMP_LLC_SLICE) {
            c->bounds.x = llc_x;
            c->bounds.y = llc_y;
            llc_x += c->bounds.width + cl + 1.0;
        }
    }

    // Routers placed along central corridor starting after LLC slices
    double router_x = (llc_x > 2.0) ? (llc_x + 1.0) : 50.0;
    if (router_x < 50.0) router_x = 50.0;
    double router_y = 31.0;
    for (int i = 0; i < arch->component_count; ++i) {
        ComponentInstance* c = &arch->components[i];
        if (c->type == COMP_ROUTER) {
            if (router_x + c->bounds.width > dw - 2.0) {
                router_x = 50.0;
                router_y = 37.0;
            }
            c->bounds.x = router_x;
            c->bounds.y = router_y;
            router_x += c->bounds.width + cl + 0.5;
        }
    }

    // 5. East Quadrant (x = 52.0 .. dw - 2.0): E-Cores and shared L2 caches
    double east_x = (dw > 70.0) ? (dw - 46.0) : (dw * 0.5);
    int num_e_clusters = (arch->config.num_e_cores + arch->config.e_cluster_size - 1) / arch->config.e_cluster_size;

    for (int clust = 0; clust < num_e_clusters; ++clust) {
        double clust_base_y = (clust == 0) ? 8.5 : 44.0;

        // Shared L2 for this cluster
        for (int i = 0; i < arch->component_count; ++i) {
            ComponentInstance* c = &arch->components[i];
            if (c->cluster_id == (clust + 100) && c->type == COMP_L2_CACHE) {
                c->bounds.x = east_x;
                c->bounds.y = clust_base_y + 3.0;
            }
        }

        // 4 E-cores in this cluster placed in 2x2 grid to the right of shared L2
        int e_in_clust = 0;
        for (int e = 0; e < arch->config.num_e_cores; ++e) {
            if (e / arch->config.e_cluster_size == clust) {
                int row = e_in_clust / 2;
                int col = e_in_clust % 2;
                double e_x = east_x + 9.5 + col * 11.5;
                double e_y = clust_base_y + row * 10.5;

                char ecore_name[MAX_NAME_LEN];
                char l1i_name[MAX_NAME_LEN];
                char l1d_name[MAX_NAME_LEN];
                snprintf(ecore_name, sizeof(ecore_name), "E-Core-%d", e);
                snprintf(l1i_name, sizeof(l1i_name), "L1I-E%d", e);
                snprintf(l1d_name, sizeof(l1d_name), "L1D-E%d", e);

                for (int i = 0; i < arch->component_count; ++i) {
                    ComponentInstance* c = &arch->components[i];
                    if (strcmp(c->name, l1i_name) == 0) {
                        c->bounds.x = e_x;
                        c->bounds.y = e_y;
                    } else if (strcmp(c->name, l1d_name) == 0) {
                        c->bounds.x = e_x + 5.0;
                        c->bounds.y = e_y;
                    } else if (strcmp(c->name, ecore_name) == 0) {
                        c->bounds.x = e_x;
                        c->bounds.y = e_y + 4.0;
                    }
                }
                e_in_clust++;
            }
        }
    }

    // Update port absolute coordinates for all components
    for (int i = 0; i < arch->component_count; ++i) {
        component_update_port_positions(&arch->components[i]);
    }

    // Validate the resulting placement
    return placement_validate(arch, cl, err_msg, err_size);
}
