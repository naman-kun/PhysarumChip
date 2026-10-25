#ifndef PHYSARUM_PLACEMENT_H
#define PHYSARUM_PLACEMENT_H

#include "common.h"
#include "architecture.h"

typedef struct {
    double min_clearance;  // Minimum space between component blocks (e.g., 2.0 units)
    double channel_width;   // Central routing channel width
} PlacementParams;

PlacementParams placement_default_params(void);

// Automatic deterministic floorplanning layout heuristic
StatusCode placement_auto_layout(Architecture* arch, const PlacementParams* params, char* err_msg, size_t err_size);

// Hard-constraint validation: checks bounds and non-overlap
StatusCode placement_validate(const Architecture* arch, double min_clearance, char* err_msg, size_t err_size);

// Validate moving an individual component to (new_x, new_y)
StatusCode placement_check_move(const Architecture* arch, int comp_id, double new_x, double new_y, double min_clearance, char* err_msg, size_t err_size);

#endif /* PHYSARUM_PLACEMENT_H */
