# PhysarumChip Agent Specification Pack

This folder is designed for a coding agent working from an empty project directory. It separates requirements by responsibility so the agent can load context progressively rather than treating one long prompt as a single task.

## How to use it

1. Put the contents of this pack in the root of the empty project folder, or keep this pack in a sibling folder that the agent can read.
2. Give the agent the contents of `00_START_HERE.md` as the initial instruction. If the agent starts from the parent project root, use the path `PhysarumChip_Spec/00_START_HERE.md`.
3. Tell the agent the project root is the empty folder and it should read the relevant specification file when entering each phase.
4. Do not paste all specification files into one prompt unless the agent requires that workflow.

## Documents

- `00_START_HERE.md` — compact master instruction and phased reading order.
- `01_PRODUCT_REQUIREMENTS.md` — features, CPU components, connectivity, constraints and user flow.
- `02_C_ENGINE_AND_ALGORITHMS.md` — C11 engine, data structures, Physarum-inspired routing, Dijkstra and metrics.
- `03_UI_AND_INTEGRATION.md` — frontend, backend bridge, interface and persistence.
- `04_TESTS_AND_ACCEPTANCE.md` — implementation phases, automated tests, definition of done and final handoff.

## Key design decision
The default algorithm is a Physarum-inspired conductivity/flow adaptation model. It is not automatically the same as the population-based standard Slime Mould Algorithm (SMA). All core algorithms and domain computations must run in C11; the frontend and Node.js server are presentation and integration layers only.
