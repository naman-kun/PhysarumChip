#ifndef PHYSARUM_ARCHITECTURE_H
#define PHYSARUM_ARCHITECTURE_H

#include "common.h"
#include "component.h"

typedef enum {
    TOPOLOGY_RING = 0,
    TOPOLOGY_MESH_2D,
    TOPOLOGY_CROSSBAR
} InterconnectTopology;

typedef enum {
    NET_TYPE_DATA = 0,
    NET_TYPE_CONTROL,
    NET_TYPE_CLOCK,
    NET_TYPE_POWER
} NetType;

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    int source_comp_id;
    char source_port[MAX_NAME_LEN];
    int target_comp_id;
    char target_port[MAX_NAME_LEN];
    NetType type;
    int priority;         // 1 (lowest) to 5 (highest / critical)
    int track_demand;     // Tracks required (e.g., 1 or 2)
    double max_length;    // 0 = unbounded, >0 = maximum wirelength budget
} LogicalNet;

typedef struct {
    int num_p_cores;       // Default 8
    int num_e_cores;       // Default 8
    int e_cluster_size;    // Default 4 (shared L2 per cluster)
    int num_llc_slices;    // Default 4
    int num_mem_ctrls;     // Default 2
    bool has_system_agent; // Default true
    bool has_pcie_io;      // Default true
    InterconnectTopology topology; // Ring, 2D Mesh, Crossbar
    double die_width;      // Default 100.0
    double die_height;     // Default 80.0
    double routing_pitch;  // Default 1.0 (grid pitch)
} ArchConfig;

typedef struct {
    ArchConfig config;
    int component_count;
    ComponentInstance components[MAX_COMPONENTS];
    int net_count;
    LogicalNet nets[MAX_NETS];
    double total_estimated_area;
    double total_estimated_power;
} Architecture;

ArchConfig architecture_default_config(void);
const char* topology_to_string(InterconnectTopology topo);
InterconnectTopology topology_from_string(const char* str);
const char* net_type_to_string(NetType type);
NetType net_type_from_string(const char* str);

StatusCode architecture_validate_config(const ArchConfig* cfg, char* err_msg, size_t err_size);
StatusCode architecture_generate(const ArchConfig* cfg, Architecture* out_arch, char* err_msg, size_t err_size);
const ComponentInstance* architecture_find_component(const Architecture* arch, int id);
const LogicalNet* architecture_find_net(const Architecture* arch, int net_id);
StatusCode architecture_validate_netlist(const Architecture* arch, char* err_msg, size_t err_size);

#endif /* PHYSARUM_ARCHITECTURE_H */
