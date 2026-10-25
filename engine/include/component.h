#ifndef PHYSARUM_COMPONENT_H
#define PHYSARUM_COMPONENT_H

#include "common.h"

typedef enum {
    COMP_P_CORE = 0,
    COMP_E_CORE,
    COMP_L1_INST,
    COMP_L1_DATA,
    COMP_L2_CACHE,
    COMP_LLC_SLICE,
    COMP_MEM_CTRL,
    COMP_SYSTEM_AGENT,
    COMP_PCIE_IO,
    COMP_ROUTER,
    COMP_UNKNOWN
} ComponentType;

typedef enum {
    DIR_NORTH = 0,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST,
    DIR_BIDIRECTIONAL
} PortDirection;

typedef struct {
    char name[MAX_NAME_LEN];
    PortDirection dir;
    double rel_x; // Relative offset [0.0 .. 1.0] or relative coords within component
    double rel_y;
    double abs_x; // Calculated global die coordinates
    double abs_y;
} Port;

typedef struct {
    ComponentType type;
    char type_name[MAX_NAME_LEN];
    double default_width;
    double default_height;
    double area_estimate;   // Illustrative mm^2 or abstract units
    double power_estimate;  // Illustrative Watts or abstract units
    int port_count;
    Port ports[MAX_PORTS_PER_COMP];
} ComponentTemplate;

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    ComponentType type;
    int cluster_id; // -1 if unclustered, else core index or E-cluster index
    Rect2D bounds;  // x, y, width, height
    int orientation; // 0, 90, 180, 270 degrees
    double power_estimate;
    double area_estimate;
    int port_count;
    Port ports[MAX_PORTS_PER_COMP];
} ComponentInstance;

// Template library initialization and access
void component_library_init(void);
const ComponentTemplate* component_template_get(ComponentType type);
const char* component_type_to_string(ComponentType type);
ComponentType component_type_from_string(const char* str);

// Instance creation and management
ComponentInstance component_create(int id, const char* name, ComponentType type, double x, double y, int cluster_id);
void component_update_port_positions(ComponentInstance* comp);
bool component_get_port_location(const ComponentInstance* comp, const char* port_name, Point2D* out_pos);

#endif /* PHYSARUM_COMPONENT_H */
