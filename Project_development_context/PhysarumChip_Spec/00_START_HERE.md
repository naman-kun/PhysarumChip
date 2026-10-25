# PhysarumChip — Agent Start Here

## Your task
Build a complete local application from the **empty project folder**. Do not assume any code, framework, assets, or configuration already exists. Create the files, install dependencies, implement the software, compile the C engine, test it, and document how to run it.

**Do not stop after planning or creating a scaffold.** Work through the phases below until the acceptance criteria in `04_TESTS_AND_ACCEPTANCE.md` pass as far as the environment permits.

## Product in one sentence
PhysarumChip is an educational CPU-chip floorplanning and interconnect-routing simulator: users configure a CPU architecture, generate a block layout and netlist, set constraints, then route the connections using a Physarum-inspired adaptive algorithm implemented in C and compare the result with Dijkstra routing.

## Non-negotiable decisions
1. **All core algorithmic/domain logic is C11.** This includes architecture generation, graph operations, placement, route optimization, conductivity updates, Dijkstra, congestion, constraint validation, budgets, and metrics.
2. Use React + TypeScript + Vite for the UI and Node.js + Express only as a thin bridge to the C executable.
3. The C engine is a standalone executable. Exchange structured JSON through standard input/output or temporary JSON files; choose one approach and document it.
4. The current folder is empty. Initialize the whole project here; do not create an unnecessary nested project directory.
5. The primary algorithm is a **Physarum-inspired conductivity/flow adaptation model**, not the separately defined population-based standard Slime Mould Algorithm (SMA). Do not mislabel one as the other.
6. Dijkstra is a separate baseline and may be used for route extraction over adaptive edge costs, but it must not replace the conductivity/flow adaptation.
7. The software is an educational global-routing simulator, not production EDA, transistor-level design, fabrication sign-off, or a reliable predictor of real silicon power/cost.
8. No fake metrics, hard-coded successful routing results, nonfunctional buttons, or frontend reimplementations of core algorithms.
9. Make practical engineering decisions without asking for approval at every phase. Record assumptions and known limitations in the README.

## Read the specification progressively
Use these files as modular requirements. Do not repeatedly load the entire specification into working context.

- `01_PRODUCT_REQUIREMENTS.md` — CPU architecture, templates, netlist, budgets, placement, and user-visible behavior.
- `02_C_ENGINE_AND_ALGORITHMS.md` — C architecture, data structures, Physarum-inspired routing, Dijkstra, validation, and metrics.
- `03_UI_AND_INTEGRATION.md` — frontend, backend bridge, interface, state and visual design.
- `04_TESTS_AND_ACCEPTANCE.md` — build phases, tests, acceptance checklist, deliverables.

Read `01` before implementing the domain model, `02` before coding the C engine, `03` before integration/UI work, and `04` before and during verification. Consult another file when a task crosses those boundaries.

## Implementation sequence
1. Bootstrap the C11 build, React/Vite frontend, Express bridge, test setup, and README.
2. Implement and test C data models, templates, architecture generation, graph representation, placement, and validation.
3. Implement and test Physarum-inspired routing and Dijkstra on small deterministic graphs.
4. Add the JSON interface and connect the C executable to the backend.
5. Build architecture controls, floorplan canvas, netlist editor, constraints panel, and results view.
6. Run end-to-end tests, fix integration defects, and verify the default architecture workflow.
7. Finish documentation with reproducible commands and measured results from actual runs.

## Working rules
- Keep modules small and interfaces explicit.
- Build incrementally and run tests after each phase.
- Prefer a working simplified feature to a placeholder for an ambitious feature.
- Reject invalid or infeasible configurations with useful errors; do not silently violate hard constraints.
- Use deterministic defaults and a configurable random seed if randomness is introduced.
- Do not claim that the Physarum-inspired algorithm always outperforms Dijkstra. Report measured outcomes.
- At completion, provide a short summary of what works, how to launch it, commands used for tests, and any remaining limitations.
