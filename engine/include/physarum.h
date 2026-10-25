#ifndef PHYSARUM_ALGORITHM_H
#define PHYSARUM_ALGORITHM_H

#include "common.h"
#include "graph.h"
#include "architecture.h"
#include "dijkstra.h"

typedef struct {
    double rho;                  // Conductivity decay factor (e.g., 0.1)
    double alpha;                // Conductivity reinforcement rate (e.g., 1.0)
    double d_min;                // Minimum conductivity floor (e.g., 0.01)
    double d_max;                // Maximum conductivity ceiling (e.g., 100.0)
    double source_current;       // Boundary flow injection I_0 (e.g., 1.0)
    int max_pressure_iters;      // Max Gauss-Seidel pressure relaxation iterations (e.g., 100)
    double pressure_tolerance;   // Epsilon for pressure convergence (e.g., 1e-4)
    int adaptation_steps;        // Number of conductivity adaptation cycles (e.g., 20)
    double congestion_weight;    // Resistance penalty for high demand edges (e.g., 3.0)
    double bend_penalty;         // Direction change penalty (e.g., 0.5)
    bool enforce_hard_capacity;  // Fail route if track demand cannot be satisfied
    int reroute_passes;          // Rip-up and reroute passes for congested nets (e.g., 2)
} PhysarumParams;

PhysarumParams physarum_default_params(void);

// Solve flow network for a single net and extract adapted tube path
bool physarum_route_single_net(RoutingGraph* graph,
                               int start_node,
                               int target_node,
                               const PhysarumParams* params,
                               int net_demand,
                               RoutePath* out_path,
                               int* out_pressure_iters,
                               int* out_adapt_iters);

#endif /* PHYSARUM_ALGORITHM_H */
