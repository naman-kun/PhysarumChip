# 01 — Product Requirements

## 1. Scope
Build an educational CPU-chip floorplanning and physical interconnect-routing simulator. The user configures a CPU architecture, creates a component layout and logical netlist, sets constraints, routes connections, and compares routing strategies. Keep the first release focused on two-terminal routing and core DSA concepts.

This is not a transistor-level design tool and does not perform sign-off timing, detailed signal-integrity analysis, real power-grid design, fabrication checks, or production-quality place-and-route.

## 2. Default CPU configuration
The initial example must be generated from editable parameters:

- 8 performance CPU cores.
- 8 efficiency CPU cores.
- One L1 instruction cache and one L1 data cache per core.
- One private L2 cache per performance core.
- One shared L2 cache per group of four efficiency cores (two efficiency clusters by default).
- Four last-level cache slices.
- Two memory controllers.
- One system agent.
- One PCIe/I/O controller.
- One selectable on-chip logical interconnect: Ring, 2D Mesh, or Crossbar abstraction.

These are educational example specifications, not the specifications of an actual commercial CPU. Core count, cache count, efficiency-cluster size, controller count, LLC slice count, and interconnect selection must be configurable.

When the configuration changes, regenerate the component instances and default netlist consistently. Validate incompatible or nonsensical inputs before generation.

## 3. Component template library
Create reusable templates for:

- Performance core.
- Efficiency core.
- L1 instruction cache.
- L1 data cache.
- L2 cache.
- Last-level cache slice.
- Memory controller.
- System agent.
- PCIe/I/O controller.
- Interconnect router/fabric block.

Each template defines component type/name, normalized width and height, illustrative area and power estimates, ports and their boundary locations, supported connection types, default connectivity rules, and permitted rotations/orientations.

A component instance has a unique ID, template/type, position, dimensions, orientation, and configurable parameters. Component template values are illustrative and editable; do not label them as measured specifications for a real processor.

## 4. Automatic floorplanning
Implement a deterministic, explainable first-pass placement heuristic in C. It should group cores with their local caches, place shared resources sensibly, leave routing channels, keep all blocks inside the die boundary, and prevent overlap.

Required behavior:
- User can inspect and manually reposition blocks on the canvas.
- Validate a new position before committing it.
- Keep components inside die bounds and prevent overlap.
- Report when the current die cannot fit the design or required routing clearance.
- Do not silently overlap blocks or shrink them beyond their configured dimensions.

Focus the Physarum-inspired algorithm on routing. Do not claim that it optimizes placement unless a separate placement optimizer is actually implemented and evaluated.

## 5. Logical connectivity / netlist
Keep logical connectivity distinct from physical wire paths.

Generate sensible default two-terminal connections for:
- Each core to its L1 caches.
- Performance-core L2 caches and efficiency-cluster L2 caches to the logical fabric.
- LLC slices to the fabric and/or selected cache hierarchy endpoints.
- Fabric to memory controllers, system agent, and I/O controller.

Document the chosen default topology and connectivity assumptions. Do not invent random connections to make the diagram look connected.

Allow users to inspect, add, edit, and delete logical connections. Each net contains source component/port, destination component/port, net type, priority/criticality, track demand or bandwidth proxy, and optional maximum route length.

Support simplified `data`, `control`, `clock`, and `power` net labels. Clock and power nets are only illustrative; do not claim production clock-tree synthesis or power-distribution routing.

The three interconnect choices (Ring, 2D Mesh, Crossbar) describe the logical/architectural topology. Do not confuse them with the physical grid paths created by the routing engine. If a chosen topology restricts valid connections, enforce those restrictions and explain them in the UI.

Support two-terminal nets in the first release. If the UI offers a logical multi-destination relation, decompose it into clearly identified two-terminal branches and document the simplification.

## 6. Configuration and budgets
Provide editable settings for:

### Physical
- Die width and height in normalized layout units.
- Maximum die area.
- Routing-grid resolution or pitch.
- Minimum clearance between blocks and available routing regions.
- Forbidden/blocked regions.

### Routing
- Edge/segment capacity in abstract available tracks.
- Required track demand per net.
- Maximum congestion threshold.
- Maximum route length per net or overall.
- Optional bend penalty.
- Optional via/layer-change penalty only if multi-layer routing is actually implemented.

### Budgets
- Maximum illustrative component-area budget.
- Maximum estimated power budget.
- Maximum total wire length.
- Maximum abstract design cost.
- Maximum acceptable routing congestion.

Label area, power, and cost as simplified estimates or abstract units unless backed by provided technology-library data. Do not imply that the simulator predicts real fabrication costs, power, temperature, or silicon feasibility.

Separate hard constraints from soft optimization objectives. A hard constraint must be enforced or explicitly reported as failed. Soft objectives are optimization preferences.

## 7. User workflow
The application should support this complete journey:

1. Open the default 8P + 8E example.
2. Change architecture counts or cache/cluster settings.
3. Generate or regenerate components and the default netlist.
4. Inspect the chip floorplan and move blocks where needed.
5. Edit logical connections and routing demands.
6. Set die, routing, and budget constraints.
7. Run Physarum-inspired routing.
8. Run Dijkstra baseline routing.
9. Compare measured results.
10. Add a blocked region or change a segment capacity and run again.
11. Save/load the design and export results when implemented.

The default architecture must be visible immediately after startup; the user should not need to configure everything from scratch.
