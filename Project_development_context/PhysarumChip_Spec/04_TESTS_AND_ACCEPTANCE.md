# 04 — Build Phases, Tests, and Acceptance

## 1. Build from the empty folder
Create all source, config, dependency, testing, and documentation files in the current folder. Do not assume any files already exist and do not create an unnecessary nested project directory.

Suggested structure:

```text
engine/include/
engine/src/
engine/tests/
engine/third_party/
server/src/
frontend/src/
docs/
Makefile
README.md
.gitignore
```

Adjust the structure if needed while preserving separation between the C engine, thin server, frontend, tests, and documentation.

## 2. Implementation phases

### Phase 1 — Bootstrap
- Initialize React/TypeScript/Vite and Node/Express.
- Create the standalone C11 build system.
- Add lint/test scripts and `.gitignore`.
- Write initial README with prerequisites and expected commands.
- Verify that the minimal C engine compiles and the basic app launches.

### Phase 2 — C domain model
- Define components, templates, ports, nets, constraints, graph edges, paths and results.
- Generate the default 8P + 8E architecture and counts.
- Generate a plausible default netlist.
- Implement deterministic placement and hard-constraint validation.
- Add C unit tests before routing.

### Phase 3 — Routing engine
- Build grid graph, obstacle handling and endpoint mapping.
- Implement BFS connectivity checks and heap-based Dijkstra.
- Implement Physarum-inspired pressure/flow and conductivity updates.
- Add path extraction, track capacity reservation, congestion costs, failure reporting and metrics.
- Test on small deterministic scenarios before trying the full CPU design.

### Phase 4 — JSON/API integration
- Define request/response structures.
- Implement parser/serializer and C executable entrypoint.
- Add thin Express endpoints to invoke the C engine safely.
- Test valid and invalid requests end-to-end.

### Phase 5 — UI
- Build architecture controls, chip canvas, netlist editor, constraints, algorithm controls, results and inspector.
- Ensure the default design appears at startup.
- Connect every action to real backend/C behavior.

### Phase 6 — End-to-end verification
- Run the default architecture workflow.
- Move a component and validate placement.
- Run both routing methods.
- Insert a blocked region or change capacity and rerun.
- Confirm that failed routes and constraints are reported honestly.
- Fix errors discovered during testing.

### Phase 7 — Finalization
- Complete usage/build instructions.
- Document assumptions, limitations, algorithms, data structures and complexity.
- Record actual benchmark results from reproducible test runs.

## 3. Minimum automated tests
1. Default 8P + 8E architecture generates correctly.
2. Cache counts and cluster counts are correct.
3. Changing architecture settings regenerates correct components/netlist.
4. Placements are in bounds and do not overlap.
5. Impossible placement is rejected with a useful error.
6. Valid default netlist passes structural validation.
7. A simple unobstructed connection routes successfully.
8. Routes avoid component blocks and forbidden regions.
9. Disconnected endpoints are reported as unroutable.
10. Accepted routes respect hard track-capacity constraints.
11. Invalid budgets and numeric settings are handled safely.
12. Conductivity updates and path reconstruction are tested.
13. Pressure iteration handles convergence and iteration limits.
14. Both routing methods run and return measured metrics.
15. Save/load restores architecture, positions, netlist and settings.
16. Backend handles invalid inputs and C-engine failures.
17. Frontend integration displays actual result/failure responses.
18. Changing an obstacle/capacity produces a new run with updated results.

Test more than one architecture size and include deterministic routing test graphs.

## 4. Definition of done
The project is complete when:

- The C11 engine compiles from a clean setup.
- The app runs locally on macOS with documented commands.
- The default 8P + 8E architecture appears at startup.
- Architecture settings regenerate components and netlist.
- The user can inspect and move component blocks.
- Hard placement constraints are enforced.
- The user can edit connections and routing constraints.
- Physarum-inspired routing and Dijkstra both execute from the UI.
- Wires drawn on the canvas correspond to actual C-engine output.
- Obstacles/capacity changes can be tested in a new run.
- Metrics derive from actual results; failure cases are not concealed.
- Core tests pass and failing tests are explained rather than hidden.
- README includes prerequisites, install/build/run/test instructions.
- Algorithm identity, assumptions, complexity, and known limitations are documented.

## 5. Required final handoff
When implementation is complete, report:
- What was built and which features work.
- Exact commands to install, build, run and test.
- Test results, including any tests that could not be run.
- Measured example results from an actual reproducible run.
- Known limitations or incomplete requirements.

Do not claim completion for features that remain placeholders or have not been tested.
