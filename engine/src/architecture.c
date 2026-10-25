#include "../include/architecture.h"
#include "../include/placement.h"

ArchConfig architecture_default_config(void) {
    ArchConfig cfg;
    cfg.num_p_cores = 8;
    cfg.num_e_cores = 8;
    cfg.e_cluster_size = 4;
    cfg.num_llc_slices = 4;
    cfg.num_mem_ctrls = 2;
    cfg.has_system_agent = true;
    cfg.has_pcie_io = true;
    cfg.topology = TOPOLOGY_RING;
    cfg.die_width = 100.0;
    cfg.die_height = 80.0;
    cfg.routing_pitch = 1.0;
    return cfg;
}

const char* topology_to_string(InterconnectTopology topo) {
    switch (topo) {
        case TOPOLOGY_RING: return "RING";
        case TOPOLOGY_MESH_2D: return "MESH_2D";
        case TOPOLOGY_CROSSBAR: return "CROSSBAR";
        default: return "UNKNOWN";
    }
}

InterconnectTopology topology_from_string(const char* str) {
    if (!str) return TOPOLOGY_RING;
    if (strcmp(str, "RING") == 0 || strcmp(str, "Ring") == 0) return TOPOLOGY_RING;
    if (strcmp(str, "MESH_2D") == 0 || strcmp(str, "Mesh") == 0 || strcmp(str, "2D_Mesh") == 0) return TOPOLOGY_MESH_2D;
    if (strcmp(str, "CROSSBAR") == 0 || strcmp(str, "Crossbar") == 0) return TOPOLOGY_CROSSBAR;
    return TOPOLOGY_RING;
}

const char* net_type_to_string(NetType type) {
    switch (type) {
        case NET_TYPE_DATA: return "DATA";
        case NET_TYPE_CONTROL: return "CONTROL";
        case NET_TYPE_CLOCK: return "CLOCK";
        case NET_TYPE_POWER: return "POWER";
        default: return "UNKNOWN";
    }
}

NetType net_type_from_string(const char* str) {
    if (!str) return NET_TYPE_DATA;
    if (strcmp(str, "DATA") == 0 || strcmp(str, "data") == 0) return NET_TYPE_DATA;
    if (strcmp(str, "CONTROL") == 0 || strcmp(str, "control") == 0) return NET_TYPE_CONTROL;
    if (strcmp(str, "CLOCK") == 0 || strcmp(str, "clock") == 0) return NET_TYPE_CLOCK;
    if (strcmp(str, "POWER") == 0 || strcmp(str, "power") == 0) return NET_TYPE_POWER;
    return NET_TYPE_DATA;
}

StatusCode architecture_validate_config(const ArchConfig* cfg, char* err_msg, size_t err_size) {
    if (!cfg) {
        if (err_msg) snprintf(err_msg, err_size, "Config pointer is NULL");
        return STATUS_ERROR_INVALID_PARAM;
    }
    if (cfg->num_p_cores < 1 || cfg->num_p_cores > 32) {
        if (err_msg) snprintf(err_msg, err_size, "P-core count must be between 1 and 32 (got %d)", cfg->num_p_cores);
        return STATUS_ERROR_INVALID_PARAM;
    }
    if (cfg->num_e_cores < 0 || cfg->num_e_cores > 64) {
        if (err_msg) snprintf(err_msg, err_size, "E-core count must be between 0 and 64 (got %d)", cfg->num_e_cores);
        return STATUS_ERROR_INVALID_PARAM;
    }
    if (cfg->num_e_cores > 0 && (cfg->e_cluster_size < 1 || cfg->e_cluster_size > 8)) {
        if (err_msg) snprintf(err_msg, err_size, "E-cluster size must be between 1 and 8 (got %d)", cfg->e_cluster_size);
        return STATUS_ERROR_INVALID_PARAM;
    }
    if (cfg->num_llc_slices < 1 || cfg->num_llc_slices > 16) {
        if (err_msg) snprintf(err_msg, err_size, "LLC slice count must be between 1 and 16 (got %d)", cfg->num_llc_slices);
        return STATUS_ERROR_INVALID_PARAM;
    }
    if (cfg->num_mem_ctrls < 1 || cfg->num_mem_ctrls > 8) {
        if (err_msg) snprintf(err_msg, err_size, "Memory controller count must be between 1 and 8 (got %d)", cfg->num_mem_ctrls);
        return STATUS_ERROR_INVALID_PARAM;
    }
    if (cfg->die_width < 40.0 || cfg->die_height < 40.0) {
        if (err_msg) snprintf(err_msg, err_size, "Die dimensions must be at least 40x40 (got %.1fx%.1f)", cfg->die_width, cfg->die_height);
        return STATUS_ERROR_INVALID_PARAM;
    }
    if (cfg->routing_pitch <= 0.1 || cfg->routing_pitch > 10.0) {
        if (err_msg) snprintf(err_msg, err_size, "Routing pitch must be between 0.1 and 10.0 (got %.2f)", cfg->routing_pitch);
        return STATUS_ERROR_INVALID_PARAM;
    }
    return STATUS_OK;
}

const ComponentInstance* architecture_find_component(const Architecture* arch, int id) {
    if (!arch) return NULL;
    for (int i = 0; i < arch->component_count; ++i) {
        if (arch->components[i].id == id) {
            return &arch->components[i];
        }
    }
    return NULL;
}

const LogicalNet* architecture_find_net(const Architecture* arch, int net_id) {
    if (!arch) return NULL;
    for (int i = 0; i < arch->net_count; ++i) {
        if (arch->nets[i].id == net_id) {
            return &arch->nets[i];
        }
    }
    return NULL;
}

static void add_net(Architecture* arch, const char* name, int src_id, const char* src_port,
                    int tgt_id, const char* tgt_port, NetType type, int priority, int demand) {
    if (arch->net_count >= MAX_NETS) return;
    LogicalNet* n = &arch->nets[arch->net_count];
    n->id = arch->net_count + 1;
    strncpy(n->name, name, MAX_NAME_LEN - 1);
    n->name[MAX_NAME_LEN - 1] = '\0';
    n->source_comp_id = src_id;
    strncpy(n->source_port, src_port, MAX_NAME_LEN - 1);
    n->source_port[MAX_NAME_LEN - 1] = '\0';
    n->target_comp_id = tgt_id;
    strncpy(n->target_port, tgt_port, MAX_NAME_LEN - 1);
    n->target_port[MAX_NAME_LEN - 1] = '\0';
    n->type = type;
    n->priority = priority;
    n->track_demand = demand;
    n->max_length = 0.0;
    arch->net_count++;
}

StatusCode architecture_validate_netlist(const Architecture* arch, char* err_msg, size_t err_size) {
    if (!arch) {
        if (err_msg) snprintf(err_msg, err_size, "Architecture pointer is NULL");
        return STATUS_ERROR_INVALID_PARAM;
    }
    for (int i = 0; i < arch->net_count; ++i) {
        const LogicalNet* n = &arch->nets[i];
        const ComponentInstance* src = architecture_find_component(arch, n->source_comp_id);
        if (!src) {
            if (err_msg) snprintf(err_msg, err_size, "Net %s (ID %d): Source component ID %d not found", n->name, n->id, n->source_comp_id);
            return STATUS_ERROR_INVALID_PARAM;
        }
        const ComponentInstance* tgt = architecture_find_component(arch, n->target_comp_id);
        if (!tgt) {
            if (err_msg) snprintf(err_msg, err_size, "Net %s (ID %d): Target component ID %d not found", n->name, n->id, n->target_comp_id);
            return STATUS_ERROR_INVALID_PARAM;
        }
        Point2D p_src, p_tgt;
        if (!component_get_port_location(src, n->source_port, &p_src)) {
            if (err_msg) snprintf(err_msg, err_size, "Net %s: Port '%s' on source '%s' invalid", n->name, n->source_port, src->name);
            return STATUS_ERROR_INVALID_PARAM;
        }
        if (!component_get_port_location(tgt, n->target_port, &p_tgt)) {
            if (err_msg) snprintf(err_msg, err_size, "Net %s: Port '%s' on target '%s' invalid", n->name, n->target_port, tgt->name);
            return STATUS_ERROR_INVALID_PARAM;
        }
    }
    return STATUS_OK;
}

StatusCode architecture_generate(const ArchConfig* cfg, Architecture* out_arch, char* err_msg, size_t err_size) {
    StatusCode sc = architecture_validate_config(cfg, err_msg, err_size);
    if (sc != STATUS_OK) return sc;

    component_library_init();
    memset(out_arch, 0, sizeof(Architecture));
    out_arch->config = *cfg;

    int comp_id = 1;
    char name_buf[MAX_NAME_LEN];

    // Arrays to store component IDs for netlist linking
    int pcore_ids[32];
    int l1i_p_ids[32];
    int l1d_p_ids[32];
    int l2_p_ids[32];

    int ecore_ids[64];
    int l1i_e_ids[64];
    int l1d_e_ids[64];
    int l2_eclust_ids[16];

    int llc_ids[16];
    int mc_ids[8];
    int sys_agent_id = -1;
    int pcie_io_id = -1;

    // 1. Generate P-Cores with private L1-I, L1-D, L2
    for (int i = 0; i < cfg->num_p_cores; ++i) {
        snprintf(name_buf, sizeof(name_buf), "P-Core-%d", i);
        pcore_ids[i] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_P_CORE, 0, 0, i);

        snprintf(name_buf, sizeof(name_buf), "L1I-P%d", i);
        l1i_p_ids[i] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_L1_INST, 0, 0, i);

        snprintf(name_buf, sizeof(name_buf), "L1D-P%d", i);
        l1d_p_ids[i] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_L1_DATA, 0, 0, i);

        snprintf(name_buf, sizeof(name_buf), "L2-P%d", i);
        l2_p_ids[i] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_L2_CACHE, 0, 0, i);
    }

    // 2. Generate E-Clusters and E-Cores
    int num_e_clusters = (cfg->num_e_cores + cfg->e_cluster_size - 1) / cfg->e_cluster_size;
    for (int c = 0; c < num_e_clusters; ++c) {
        snprintf(name_buf, sizeof(name_buf), "L2-EClust-%d", c);
        l2_eclust_ids[c] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_L2_CACHE, 0, 0, c + 100);
    }

    for (int j = 0; j < cfg->num_e_cores; ++j) {
        int cluster_idx = j / cfg->e_cluster_size;
        snprintf(name_buf, sizeof(name_buf), "E-Core-%d", j);
        ecore_ids[j] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_E_CORE, 0, 0, cluster_idx + 100);

        snprintf(name_buf, sizeof(name_buf), "L1I-E%d", j);
        l1i_e_ids[j] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_L1_INST, 0, 0, cluster_idx + 100);

        snprintf(name_buf, sizeof(name_buf), "L1D-E%d", j);
        l1d_e_ids[j] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_L1_DATA, 0, 0, cluster_idx + 100);
    }

    // 3. Generate LLC Slices
    for (int k = 0; k < cfg->num_llc_slices; ++k) {
        snprintf(name_buf, sizeof(name_buf), "LLC-Slice-%d", k);
        llc_ids[k] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_LLC_SLICE, 0, 0, -1);
    }

    // 4. Memory Controllers
    for (int m = 0; m < cfg->num_mem_ctrls; ++m) {
        snprintf(name_buf, sizeof(name_buf), "MemCtrl-%d", m);
        mc_ids[m] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_MEM_CTRL, 0, 0, -1);
    }

    // 5. System Agent & PCIe/IO
    if (cfg->has_system_agent) {
        sys_agent_id = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, "SysAgent", COMP_SYSTEM_AGENT, 0, 0, -1);
    }
    if (cfg->has_pcie_io) {
        pcie_io_id = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, "PCIe-IO", COMP_PCIE_IO, 0, 0, -1);
    }

    // 6. Interconnect Routers
    // We instantiate a set of fabric routers corresponding to major endpoints
    int router_ids[32];
    int num_routers = cfg->num_p_cores + num_e_clusters + cfg->num_llc_slices;
    if (num_routers > 32) num_routers = 32;

    for (int r = 0; r < num_routers; ++r) {
        snprintf(name_buf, sizeof(name_buf), "Router-%d", r);
        router_ids[r] = comp_id;
        out_arch->components[out_arch->component_count++] = component_create(comp_id++, name_buf, COMP_ROUTER, 0, 0, -1);
    }

    // --- Generate Default Netlist ---

    // A. P-core local cache hierarchy
    for (int i = 0; i < cfg->num_p_cores; ++i) {
        char nname[MAX_NAME_LEN];
        snprintf(nname, sizeof(nname), "P%d_Core_L1I", i);
        add_net(out_arch, nname, pcore_ids[i], "L1I_PORT", l1i_p_ids[i], "CORE_PORT", NET_TYPE_DATA, 5, 1);

        snprintf(nname, sizeof(nname), "P%d_Core_L1D", i);
        add_net(out_arch, nname, pcore_ids[i], "L1D_PORT", l1d_p_ids[i], "CORE_PORT", NET_TYPE_DATA, 5, 1);

        snprintf(nname, sizeof(nname), "P%d_L1I_L2", i);
        add_net(out_arch, nname, l1i_p_ids[i], "L2_PORT", l2_p_ids[i], "L1I_IN", NET_TYPE_DATA, 4, 1);

        snprintf(nname, sizeof(nname), "P%d_L1D_L2", i);
        add_net(out_arch, nname, l1d_p_ids[i], "L2_PORT", l2_p_ids[i], "L1D_IN", NET_TYPE_DATA, 4, 1);

        // L2 to its assigned Router
        int r_idx = i % num_routers;
        snprintf(nname, sizeof(nname), "P%d_L2_Fabric", i);
        add_net(out_arch, nname, l2_p_ids[i], "FABRIC_OUT", router_ids[r_idx], "LOCAL_PORT", NET_TYPE_DATA, 4, 2);
    }

    // B. E-core cluster hierarchy
    for (int j = 0; j < cfg->num_e_cores; ++j) {
        int c = j / cfg->e_cluster_size;
        char nname[MAX_NAME_LEN];
        snprintf(nname, sizeof(nname), "E%d_Core_L1I", j);
        add_net(out_arch, nname, ecore_ids[j], "L1I_PORT", l1i_e_ids[j], "CORE_PORT", NET_TYPE_DATA, 5, 1);

        snprintf(nname, sizeof(nname), "E%d_Core_L1D", j);
        add_net(out_arch, nname, ecore_ids[j], "L1D_PORT", l1d_e_ids[j], "CORE_PORT", NET_TYPE_DATA, 5, 1);

        snprintf(nname, sizeof(nname), "E%d_L1I_L2Clust%d", j, c);
        add_net(out_arch, nname, l1i_e_ids[j], "L2_PORT", l2_eclust_ids[c], "L1I_IN", NET_TYPE_DATA, 4, 1);

        snprintf(nname, sizeof(nname), "E%d_L1D_L2Clust%d", j, c);
        add_net(out_arch, nname, l1d_e_ids[j], "L2_PORT", l2_eclust_ids[c], "L1D_IN", NET_TYPE_DATA, 4, 1);
    }

    // E-Cluster L2 to Fabric
    for (int c = 0; c < num_e_clusters; ++c) {
        int r_idx = (cfg->num_p_cores + c) % num_routers;
        char nname[MAX_NAME_LEN];
        snprintf(nname, sizeof(nname), "EClust%d_L2_Fabric", c);
        add_net(out_arch, nname, l2_eclust_ids[c], "FABRIC_OUT", router_ids[r_idx], "LOCAL_PORT", NET_TYPE_DATA, 4, 2);
    }

    // C. LLC Slices to Fabric
    for (int k = 0; k < cfg->num_llc_slices; ++k) {
        int r_idx = (cfg->num_p_cores + num_e_clusters + k) % num_routers;
        char nname[MAX_NAME_LEN];
        snprintf(nname, sizeof(nname), "LLC%d_Fabric", k);
        add_net(out_arch, nname, llc_ids[k], "ROUTER_PORT", router_ids[r_idx], "SOUTH_PORT", NET_TYPE_DATA, 5, 2);
    }

    // D. Interconnect topology links between Routers
    if (cfg->topology == TOPOLOGY_RING) {
        for (int r = 0; r < num_routers; ++r) {
            int next_r = (r + 1) % num_routers;
            char nname[MAX_NAME_LEN];
            snprintf(nname, sizeof(nname), "Ring_Hop_%d_to_%d", r, next_r);
            add_net(out_arch, nname, router_ids[r], "EAST_PORT", router_ids[next_r], "WEST_PORT", NET_TYPE_DATA, 4, 2);
        }
    } else if (cfg->topology == TOPOLOGY_MESH_2D) {
        int cols = 4;
        for (int r = 0; r < num_routers; ++r) {
            int col = r % cols;
            (void)(r / cols); // row
            if (col + 1 < cols && r + 1 < num_routers) {
                char nname[MAX_NAME_LEN];
                snprintf(nname, sizeof(nname), "Mesh_H_%d_%d", r, r + 1);
                add_net(out_arch, nname, router_ids[r], "EAST_PORT", router_ids[r + 1], "WEST_PORT", NET_TYPE_DATA, 4, 2);
            }
            int below = r + cols;
            if (below < num_routers) {
                char nname[MAX_NAME_LEN];
                snprintf(nname, sizeof(nname), "Mesh_V_%d_%d", r, below);
                add_net(out_arch, nname, router_ids[r], "SOUTH_PORT", router_ids[below], "LOCAL_PORT", NET_TYPE_DATA, 4, 2);
            }
        }
    } else { // CROSSBAR
        int center_r = num_routers / 2;
        for (int r = 0; r < num_routers; ++r) {
            if (r != center_r) {
                char nname[MAX_NAME_LEN];
                snprintf(nname, sizeof(nname), "XBar_Spoke_%d", r);
                add_net(out_arch, nname, router_ids[r], "EAST_PORT", router_ids[center_r], "WEST_PORT", NET_TYPE_DATA, 4, 2);
            }
        }
    }

    // E. Memory controllers to LLC / Routers
    for (int m = 0; m < cfg->num_mem_ctrls; ++m) {
        int target_llc = m % cfg->num_llc_slices;
        char nname[MAX_NAME_LEN];
        snprintf(nname, sizeof(nname), "MemCtrl%d_LLC%d", m, target_llc);
        add_net(out_arch, nname, mc_ids[m], "ROUTER_PORT", llc_ids[target_llc], "COHERENCE_PORT", NET_TYPE_DATA, 5, 2);
    }

    // F. System Agent & PCIe/IO
    if (sys_agent_id != -1 && num_routers > 0) {
        add_net(out_arch, "SysAgent_Fabric", sys_agent_id, "ROUTER_PORT", router_ids[0], "LOCAL_PORT", NET_TYPE_CONTROL, 3, 1);
    }
    if (pcie_io_id != -1 && num_routers > 1) {
        add_net(out_arch, "PCIeIO_Fabric", pcie_io_id, "ROUTER_PORT", router_ids[1], "LOCAL_PORT", NET_TYPE_DATA, 3, 2);
    }

    // Illustrative Clock Net (System Agent to P-Core-0 and E-Cluster-0)
    if (sys_agent_id != -1 && cfg->num_p_cores > 0) {
        add_net(out_arch, "Clock_Root_P0", sys_agent_id, "PWR_MGMT", pcore_ids[0], "FABRIC_PORT", NET_TYPE_CLOCK, 2, 1);
    }

    // Calculate total area and power
    out_arch->total_estimated_area = 0;
    out_arch->total_estimated_power = 0;
    for (int i = 0; i < out_arch->component_count; ++i) {
        out_arch->total_estimated_area += out_arch->components[i].area_estimate;
        out_arch->total_estimated_power += out_arch->components[i].power_estimate;
    }

    // Automatic initial floorplan placement
    PlacementParams pparams = placement_default_params();
    sc = placement_auto_layout(out_arch, &pparams, err_msg, err_size);
    if (sc != STATUS_OK) return sc;

    return architecture_validate_netlist(out_arch, err_msg, err_size);
}
