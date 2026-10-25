# 03 — UI and Integration

## 1. Stack
- Frontend: React, TypeScript, Vite.
- Backend: Node.js + Express as a thin API bridge only.
- Core: standalone C11 executable.
- Communication: structured JSON requests and responses.
- Rendering: SVG or HTML Canvas for the chip canvas and wires.

Create all files and configuration from the empty folder. Use minimal, maintained dependencies and keep the app runnable locally on macOS.

## 2. Integration contract
The backend receives validated UI requests, invokes the C executable safely, and returns the executable's output. Do not interpolate untrusted values into a shell command. Use a child-process API with explicit arguments and controlled input/output.

The C engine owns all component/graph calculations, placement validation, connectivity/routing logic, constraint evaluation, costs, and metrics. The backend may validate transport-level shape and report errors, but must not reproduce algorithmic logic.

Use structured JSON input/output with explicit schema/version if practical. Return useful errors for malformed requests, executable failures, timeouts, and invalid engine output. Do not report success until a successful result is returned by the C engine.

## 3. Main interface
Use a compact technical/CAD-inspired style with clear labels, restrained colors, flat surfaces, and limited decoration. Avoid generic gradients and inflated dashboard cards. Prioritize useful canvas space and legibility.

### Architecture panel
- Editable counts for P-cores, E-cores, cache/cluster settings, LLC slices and controllers.
- Interconnect selection: Ring, 2D Mesh, Crossbar.
- Generate/regenerate architecture action and validation messages.

### Chip canvas
- Die boundary and grid.
- Labeled component blocks and ports.
- Physical wires for actual routed paths.
- Visually distinct blocked areas, unrouted nets, and congested segments.
- Component selection, drag-and-drop movement and zoom/pan if practical.
- Revalidate placement when a block moves; do not commit illegal moves silently.

### Netlist panel
A searchable/editable list of connections showing source, destination, type, priority, demand, route status and failure reason. Support add/edit/delete operations.

### Constraints panel
Editable die dimensions, budgets, routing-grid parameters, segment capacities, per-net requirements, and algorithm parameters. Mark hard constraints separately from soft objectives.

### Algorithm controls
- Run Physarum-inspired routing.
- Run Dijkstra baseline.
- Compare results.
- Reset/clear routes.
- Introduce or edit a blocked region or segment capacity and rerun.
- Show actual iteration/convergence information if provided by the engine. Only provide pause/resume if genuinely supported.

### Results/inspector panel
Show actual C-engine outputs: successful and failed net counts, total/average wire length, peak/average congestion, budget validation, execution time, iterations and per-net details. Selecting a net should highlight its route and show its details.

## 4. Persistence and export
Implement save/load for a design, including architecture configuration, component positions, netlist, constraints and algorithm parameters. Use a simple local JSON file format; do not add a database unless a real requirement emerges.

Export measured results as CSV or JSON. Ensure exports use actual engine results and include units/assumptions.

## 5. Frontend quality rules
- No mock metrics displayed as real output.
- No decorative buttons without working behavior.
- Display loading/error/success states that reflect actual API responses.
- Keep component IDs and net IDs stable during edits where practical.
- Make validation errors specific and actionable.
- Default to the example CPU design at startup.
- Keep the frontend thin: it renders and edits data, but it does not calculate physical routes or optimization results.
