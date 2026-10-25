#include "../include/component.h"

static ComponentTemplate g_templates[COMP_UNKNOWN];
static bool g_templates_initialized = false;

static void init_single_template(ComponentType type, const char* name, double w, double h, double area, double pwr) {
    ComponentTemplate* t = &g_templates[type];
    t->type = type;
    strncpy(t->type_name, name, MAX_NAME_LEN - 1);
    t->type_name[MAX_NAME_LEN - 1] = '\0';
    t->default_width = w;
    t->default_height = h;
    t->area_estimate = area;
    t->power_estimate = pwr;
    t->port_count = 0;
}

static void add_template_port(ComponentType type, const char* port_name, PortDirection dir, double rx, double ry) {
    ComponentTemplate* t = &g_templates[type];
    if (t->port_count >= MAX_PORTS_PER_COMP) return;
    Port* p = &t->ports[t->port_count++];
    strncpy(p->name, port_name, MAX_NAME_LEN - 1);
    p->name[MAX_NAME_LEN - 1] = '\0';
    p->dir = dir;
    p->rel_x = rx;
    p->rel_y = ry;
    p->abs_x = 0;
    p->abs_y = 0;
}

void component_library_init(void) {
    if (g_templates_initialized) return;

    // 1. Performance Core
    init_single_template(COMP_P_CORE, "P-Core", 10.0, 10.0, 100.0, 15.0);
    add_template_port(COMP_P_CORE, "L1I_PORT", DIR_NORTH, 0.3, 0.0);
    add_template_port(COMP_P_CORE, "L1D_PORT", DIR_NORTH, 0.7, 0.0);
    add_template_port(COMP_P_CORE, "FABRIC_PORT", DIR_SOUTH, 0.5, 1.0);

    // 2. Efficiency Core
    init_single_template(COMP_E_CORE, "E-Core", 5.0, 5.0, 25.0, 3.0);
    add_template_port(COMP_E_CORE, "L1I_PORT", DIR_NORTH, 0.3, 0.0);
    add_template_port(COMP_E_CORE, "L1D_PORT", DIR_NORTH, 0.7, 0.0);
    add_template_port(COMP_E_CORE, "CLUSTER_BUS", DIR_SOUTH, 0.5, 1.0);

    // 3. L1 Instruction Cache
    init_single_template(COMP_L1_INST, "L1-I", 4.0, 3.0, 12.0, 0.8);
    add_template_port(COMP_L1_INST, "CORE_PORT", DIR_SOUTH, 0.5, 1.0);
    add_template_port(COMP_L1_INST, "L2_PORT", DIR_NORTH, 0.5, 0.0);

    // 4. L1 Data Cache
    init_single_template(COMP_L1_DATA, "L1-D", 4.0, 3.0, 12.0, 0.8);
    add_template_port(COMP_L1_DATA, "CORE_PORT", DIR_SOUTH, 0.5, 1.0);
    add_template_port(COMP_L1_DATA, "L2_PORT", DIR_NORTH, 0.5, 0.0);

    // 5. L2 Cache
    init_single_template(COMP_L2_CACHE, "L2-Cache", 8.0, 6.0, 48.0, 3.5);
    add_template_port(COMP_L2_CACHE, "L1I_IN", DIR_SOUTH, 0.25, 1.0);
    add_template_port(COMP_L2_CACHE, "L1D_IN", DIR_SOUTH, 0.75, 1.0);
    add_template_port(COMP_L2_CACHE, "FABRIC_OUT", DIR_NORTH, 0.5, 0.0);

    // 6. Last-Level Cache Slice
    init_single_template(COMP_LLC_SLICE, "LLC-Slice", 10.0, 6.0, 60.0, 4.0);
    add_template_port(COMP_LLC_SLICE, "ROUTER_PORT", DIR_SOUTH, 0.5, 1.0);
    add_template_port(COMP_LLC_SLICE, "COHERENCE_PORT", DIR_NORTH, 0.5, 0.0);

    // 7. Memory Controller
    init_single_template(COMP_MEM_CTRL, "Mem-Ctrl", 12.0, 5.0, 60.0, 5.0);
    add_template_port(COMP_MEM_CTRL, "ROUTER_PORT", DIR_SOUTH, 0.5, 1.0);
    add_template_port(COMP_MEM_CTRL, "PHY_DRAM", DIR_NORTH, 0.5, 0.0);

    // 8. System Agent
    init_single_template(COMP_SYSTEM_AGENT, "Sys-Agent", 10.0, 6.0, 60.0, 4.5);
    add_template_port(COMP_SYSTEM_AGENT, "ROUTER_PORT", DIR_SOUTH, 0.5, 1.0);
    add_template_port(COMP_SYSTEM_AGENT, "PWR_MGMT", DIR_NORTH, 0.5, 0.0);

    // 9. PCIe/IO Controller
    init_single_template(COMP_PCIE_IO, "PCIe-IO", 12.0, 5.0, 60.0, 5.0);
    add_template_port(COMP_PCIE_IO, "ROUTER_PORT", DIR_NORTH, 0.5, 0.0);
    add_template_port(COMP_PCIE_IO, "IO_PHY", DIR_SOUTH, 0.5, 1.0);

    // 10. Interconnect Router
    init_single_template(COMP_ROUTER, "Router", 4.0, 4.0, 16.0, 1.5);
    add_template_port(COMP_ROUTER, "LOCAL_PORT", DIR_NORTH, 0.5, 0.0);
    add_template_port(COMP_ROUTER, "EAST_PORT", DIR_EAST, 1.0, 0.5);
    add_template_port(COMP_ROUTER, "WEST_PORT", DIR_WEST, 0.0, 0.5);
    add_template_port(COMP_ROUTER, "SOUTH_PORT", DIR_SOUTH, 0.5, 1.0);

    g_templates_initialized = true;
}

const ComponentTemplate* component_template_get(ComponentType type) {
    if (!g_templates_initialized) component_library_init();
    if (type >= COMP_P_CORE && type < COMP_UNKNOWN) {
        return &g_templates[type];
    }
    return NULL;
}

const char* component_type_to_string(ComponentType type) {
    switch (type) {
        case COMP_P_CORE: return "P_CORE";
        case COMP_E_CORE: return "E_CORE";
        case COMP_L1_INST: return "L1_INST";
        case COMP_L1_DATA: return "L1_DATA";
        case COMP_L2_CACHE: return "L2_CACHE";
        case COMP_LLC_SLICE: return "LLC_SLICE";
        case COMP_MEM_CTRL: return "MEM_CTRL";
        case COMP_SYSTEM_AGENT: return "SYSTEM_AGENT";
        case COMP_PCIE_IO: return "PCIE_IO";
        case COMP_ROUTER: return "ROUTER";
        default: return "UNKNOWN";
    }
}

ComponentType component_type_from_string(const char* str) {
    if (!str) return COMP_UNKNOWN;
    if (strcmp(str, "P_CORE") == 0 || strcmp(str, "P-Core") == 0) return COMP_P_CORE;
    if (strcmp(str, "E_CORE") == 0 || strcmp(str, "E-Core") == 0) return COMP_E_CORE;
    if (strcmp(str, "L1_INST") == 0 || strcmp(str, "L1-I") == 0) return COMP_L1_INST;
    if (strcmp(str, "L1_DATA") == 0 || strcmp(str, "L1-D") == 0) return COMP_L1_DATA;
    if (strcmp(str, "L2_CACHE") == 0 || strcmp(str, "L2-Cache") == 0) return COMP_L2_CACHE;
    if (strcmp(str, "LLC_SLICE") == 0 || strcmp(str, "LLC-Slice") == 0) return COMP_LLC_SLICE;
    if (strcmp(str, "MEM_CTRL") == 0 || strcmp(str, "Mem-Ctrl") == 0) return COMP_MEM_CTRL;
    if (strcmp(str, "SYSTEM_AGENT") == 0 || strcmp(str, "Sys-Agent") == 0) return COMP_SYSTEM_AGENT;
    if (strcmp(str, "PCIE_IO") == 0 || strcmp(str, "PCIe-IO") == 0) return COMP_PCIE_IO;
    if (strcmp(str, "ROUTER") == 0 || strcmp(str, "Router") == 0) return COMP_ROUTER;
    return COMP_UNKNOWN;
}

void component_update_port_positions(ComponentInstance* comp) {
    if (!comp) return;
    for (int i = 0; i < comp->port_count; ++i) {
        Port* p = &comp->ports[i];
        p->abs_x = comp->bounds.x + p->rel_x * comp->bounds.width;
        p->abs_y = comp->bounds.y + p->rel_y * comp->bounds.height;
    }
}

ComponentInstance component_create(int id, const char* name, ComponentType type, double x, double y, int cluster_id) {
    if (!g_templates_initialized) component_library_init();
    ComponentInstance inst;
    memset(&inst, 0, sizeof(inst));

    inst.id = id;
    if (name) {
        strncpy(inst.name, name, MAX_NAME_LEN - 1);
        inst.name[MAX_NAME_LEN - 1] = '\0';
    }
    inst.type = type;
    inst.cluster_id = cluster_id;
    inst.orientation = 0;

    const ComponentTemplate* tmpl = component_template_get(type);
    if (tmpl) {
        inst.bounds.x = x;
        inst.bounds.y = y;
        inst.bounds.width = tmpl->default_width;
        inst.bounds.height = tmpl->default_height;
        inst.area_estimate = tmpl->area_estimate;
        inst.power_estimate = tmpl->power_estimate;
        inst.port_count = tmpl->port_count;
        for (int i = 0; i < tmpl->port_count; ++i) {
            inst.ports[i] = tmpl->ports[i];
        }
    } else {
        inst.bounds.x = x;
        inst.bounds.y = y;
        inst.bounds.width = 8.0;
        inst.bounds.height = 8.0;
        inst.area_estimate = 64.0;
        inst.power_estimate = 5.0;
        inst.port_count = 0;
    }

    component_update_port_positions(&inst);
    return inst;
}

bool component_get_port_location(const ComponentInstance* comp, const char* port_name, Point2D* out_pos) {
    if (!comp || !port_name || !out_pos) return false;
    for (int i = 0; i < comp->port_count; ++i) {
        if (strcmp(comp->ports[i].name, port_name) == 0) {
            out_pos->x = comp->ports[i].abs_x;
            out_pos->y = comp->ports[i].abs_y;
            return true;
        }
    }
    // Fallback: if port_name is not found or empty, return block center
    out_pos->x = comp->bounds.x + comp->bounds.width * 0.5;
    out_pos->y = comp->bounds.y + comp->bounds.height * 0.5;
    return true;
}
