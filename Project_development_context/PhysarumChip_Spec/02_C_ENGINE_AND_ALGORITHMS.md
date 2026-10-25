# 02 — C Engine and Algorithms

## 1. Language boundary
All domain and algorithmic logic must be written in **C11** and compiled into a standalone executable with Clang or GCC. This includes:

- Component template instantiation and CPU architecture generation.
- Default netlist generation and logical connectivity validation.
- Floorplanning and placement validation.
- Graph and data-structure operations.
- Physarum-inspired routing and conductivity updates.
- Dijkstra baseline routing.
- Congestion, track demand, route length, budget validation, and result metrics.

JavaScript/TypeScript is allowed only for the interface and thin API orchestration. It must not duplicate or substitute for the C algorithms.

## 2. Suggested C modules
Adapt names if needed, but keep responsibilities separate:

- `component`: templates, component instances, pins/ports.
- `architecture`: configuration validation, instance generation, default netlist.
- `placement`: deterministic initial layout and hard-constraint checks.
- `graph`: routing-grid creation, adjacency representation, obstacles, capacities.
- `physarum`: pressure/flow estimation, conductivity updates, route extraction and rerouting.
- `dijkstra`: priority queue and fixed-cost baseline routing.
- `metrics`: congestion, length, success rate, budgets, runtime and optimization reports.
- `json_io`: parse requests, validate payloads, serialize results/errors.

Use C structs for components, templates, pins, nets, grid nodes, edges, budgets, routes and routing results. Keep public interfaces in headers and implementation in `.c` files.

## 3. Graph representation
Use a grid-based routing graph. Vertices represent available routing-grid positions; edges represent permissible horizontal/vertical wire segments. Component interiors and forbidden regions are obstacles. Component ports must be mapped to valid accessible grid vertices.

Each routing edge should store at least:
- Endpoints or neighbour reference.
- Length.
- Conductivity.
- Available capacity.
- Existing demand.
- Whether the edge is blocked.

Use an adjacency list or another representation that is justified for the graph size. Arrays are suitable for grid occupancy, conductivity, capacity, and congestion. Use a min-heap for Dijkstra; use a queue for BFS reachability; use a parent array for path reconstruction.

Check dimensions and allocations for overflow/failure. Validate all endpoints and avoid invalid pointers, out-of-bounds access, or unbounded iteration.

## 4. Primary routing algorithm: Physarum-inspired adaptive conductivity
Implement a genuine conductivity/flow network adaptation. Do not merely rename Dijkstra or A* as slime mould.

For one source-destination net:

1. Initialize positive conductivity for each usable edge.
2. Apply obstacles and current routing demand/capacity.
3. Treat the source and destination as pressure boundary nodes.
4. Estimate internal node pressures with a documented, numerically stable iterative method based on flow conservation.
5. Calculate edge flow using the model:

   `Q(e) = (D(e) / L(e)) * (P(u) - P(v))`

6. Update conductivity with a documented rule such as:

   `D_new(e) = max(D_min, (1 - rho) * D(e) + alpha * abs(Q(e)))`

   where `D` is conductivity, `L` is edge length, `P` is node pressure, `Q` is flow, `rho` is decay, `alpha` is reinforcement strength, and `D_min` is the minimum conductivity.
7. Stop pressure/flow iterations on convergence or at a defensible iteration cap.
8. Extract a valid loop-free source-destination path using a documented cost that combines length, adaptive conductivity, and congestion. A shortest-path routine may be used here as a subroutine over adaptive edge costs.
9. Verify obstacle avoidance, route length, and track capacity before accepting the path.
10. Reserve capacity for the accepted net, then continue with remaining nets.
11. Make additional routing passes where feasible to improve congested routes. Do not guarantee that every route can be repaired.

Use priority/criticality to define a clear net-ordering policy. Include a congestion penalty so capacity-constrained shared corridors are not attractive merely because they are short. Define how conductivities and demand are reinitialized or retained between nets and between rerouting passes.

Document all equations and parameters, explain assumptions, and make numeric parameters configurable. Use a deterministic seed if any stochastic choice is introduced.

### Failure handling
The engine must report a structured failure when a route is impossible, a connection is disconnected, all capacity-feasible paths are blocked, pressure iteration fails to converge, invalid numeric parameters are supplied, or iteration limits are reached. Never fabricate a path or silently violate hard constraints.

## 5. Dijkstra baseline
Implement a separate Dijkstra implementation in C using a min-heap priority queue and non-negative fixed edge costs. The baseline should use the same initial placement, netlist, obstacles, capacities, net ordering, and constraints as the Physarum-inspired run.

Dijkstra is the shortest-path baseline for fixed non-negative weights. The Physarum-inspired method changes conductivity based on flow and adapts its edge preferences. Neither method should be presumed better in advance.

Clarify whether the baseline also performs capacity-aware rerouting. If it does, document the policy and apply comparable routing-order rules so the comparison is interpretable.

## 6. Metrics and cost function
Calculate all metrics in C from actual routes/results:
- Successfully routed connection count and rate.
- Unrouted connection count and reasons.
- Total and average wire length.
- Maximum/peak and average congestion.
- Capacity violations (target zero for accepted hard-constrained routes).
- Route-length and budget violations.
- Abstract total routing cost.
- Algorithm runtime and iteration count.

Document the cost function, including weights for wire length, congestion, bends, criticality, and any optional terms. Normalize terms if needed so a single unit does not accidentally dominate the objective. Hard constraints must be checked separately from soft-cost scoring.

Never use hard-coded or invented result values. Save/export measured results to CSV or JSON.

## 7. Complexity analysis
Document the actual complexity of each implemented algorithm and relevant data structure. Include graph construction, placement, BFS, heap-based Dijkstra, pressure iteration, conductivity updates, path extraction, and multi-net routing.

For a graph with `V` vertices and `E` edges, and `K` nets over `T` outer optimization/routing passes, state clearly how the pressure-solver iteration count and any path-extraction searches contribute. For example, repeated heap shortest-path extraction can contribute approximately `O(T * K * (E + V) log V)`, plus the work for each pressure/conductivity iteration. Adjust this bound to match the code; do not present it as the complexity of every possible Physarum implementation.

## 8. C robustness
Use clear ownership rules for heap nodes, graphs, paths, and parsed JSON. Check allocation returns and release memory on success and error paths. Avoid global mutable state where practical. Compile with warnings enabled and fix meaningful warnings. Consider AddressSanitizer/UndefinedBehaviorSanitizer for local tests when supported.

## 9. Algorithm identity
The required default is a **Physarum-inspired conductivity/flow model** because it maps naturally to graph interconnect routing. It is not the same by definition as the population-based Slime Mould Algorithm (SMA). Keep the implementation modular so a standard SMA variant could be added as a distinct algorithm later if explicitly needed for coursework.
