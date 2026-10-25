# PhysarumChip: Bio-Inspired CPU Floorplanning & Routing Simulator

[![C11 Engine](https://img.shields.io/badge/Engine-C11-blue.svg)](file:///Users/apple/Downloads/DSA_Project/engine)
[![Frontend](https://img.shields.io/badge/Frontend-React%20%2B%20TypeScript%20%2B%20Vite-61dafb.svg)](file:///Users/apple/Downloads/DSA_Project/frontend)
[![Backend](https://img.shields.io/badge/Server-Node.js%20%2B%20Express-green.svg)](file:///Users/apple/Downloads/DSA_Project/server)
[![Tests](https://img.shields.io/badge/Acceptance%20Tests-18%2F18%20Passed%20(100%25)-success.svg)](file:///Users/apple/Downloads/DSA_Project/tests/e2e_verification.js)

**PhysarumChip** is an interactive, educational Computer-Aided Design (CAD) simulator that models chip floorplanning and physical interconnect routing on modern multi-core CPU architectures. 

It implements a biologically inspired **Physarum polycephalum conductivity/flow adaptation model** (Tero et al., 2007) in clean, standalone **C11**, benchmarking it against a classic **Dijkstra shortest-path priority-queue baseline**. The engine is connected via a thin Node.js + Express JSON API bridge to a responsive React + TypeScript CAD canvas with real-time drag-and-drop placement, live C-based design rule checking, interactive wire routing, and congestion heatmaps.

---

## 1. Algorithmic Identity: Flow Adaptation vs. SMA

> [!IMPORTANT]
> **PhysarumChip implements the biological fluid flow adaptation model, NOT the Slime Mould Algorithm (SMA).**
> - **Physarum Flow Adaptation (Tero et al. 2007)**: Models physical protoplasmic tube adaptation where fluid flux obeys Poiseuille's law ($Q = \frac{D}{L} \Delta P$), Kirchhoff's node flow conservation holds, and pipe conductivities dynamically dilate with flux ($D \leftarrow (1-\rho)D + \alpha |Q|$) and atrophy with disuse. This physically simulates electrical and fluidic transport networks on 2D grid graphs.
> - **Slime Mould Algorithm (SMA, Li et al. 2020)**: A population-based metaheuristic optimization algorithm for unconstrained numerical optimization using stochastic position-update formulas. It has no physical relation to floorplanning or graph-based circuit routing.

For complete mathematical derivations, Laplace-Beltrami linear systems, and convergence criteria, see [docs/ALGORITHMS.md](file:///Users/apple/Downloads/DSA_Project/docs/ALGORITHMS.md).

---

## 2. Hardware Architecture & Default Floorplan

PhysarumChip models an illustrative modern heterogeneous multi-core CPU floorplan ($100 \times 80$ grid units):

- **Compute Cores**:
  - **8 Performance Cores (P-Cores)**: Large compute blocks ($10 \times 10$ units, 15W, 100 mm²).
  - **8 Efficiency Cores (E-Cores)**: Compact power-efficient blocks ($5 \times 5$ units, 3W, 25 mm²).
- **Cache Hierarchy**:
  - Dedicated private L1 Instruction ($2 \times 3$) and L1 Data ($2 \times 3$) caches per core.
  - Dedicated private L2 caches ($8 \times 6$) for each of the 8 P-Cores.
  - Shared cluster L2 caches ($8 \times 6$) shared by clusters of 4 E-Cores (2 clusters default).
- **Uncore & Memory Subsystem**:
  - **4 Last-Level Cache (LLC) Slices** ($10 \times 6$): Distributed coherent cache slices along the central interconnect spine.
  - **2 Dual-Channel Memory Controllers (MC)** ($12 \times 5$): Positioned along top and bottom die edges for external DRAM pinout.
  - **1 System Agent / PCU** ($12 \times 5$): Power management, boot sequencing, and uncore coordination.
  - **1 PCIe / IO Root Complex** ($12 \times 5$): High-speed peripheral interface controller.
- **Selectable Interconnect Topologies**:
  - **Ring**: Dual counter-rotating circular bus with 14 router stops connecting all P-cores, E-clusters, LLC slices, and controllers.
  - **2D Mesh**: Planar Manhattan network with XY routing hops.
  - **Crossbar**: Full crossbar star backplane connecting all major functional units.
- **Default Netlist**: 97 point-to-point and multi-drop interconnect nets spanning Core-to-L1, L1-to-L2, L2-to-Fabric, LLC-to-Fabric, Memory-to-LLC, and inter-router hops.

For complete architecture details and JSON schemas, see [docs/ARCHITECTURE.md](file:///Users/apple/Downloads/DSA_Project/docs/ARCHITECTURE.md).

---

## 3. Measured Benchmark Results

Authentic measurements from running the standalone C11 engine on the 97-net default architecture:

| Metric | Dijkstra Baseline | Physarum Flow Adaptation | Delta |
| :--- | :--- | :--- | :--- |
| **Routed Nets** | 97 / 97 (100.0%) | 97 / 97 (100.0%) | 0 |
| **Total Wirelength** | 2,069 grid units | 2,035 grid units | **-34 units (-1.64%)** |
| **Average Wirelength** | 21.33 units | 20.98 units | -0.35 units |
| **Peak Congestion** | 1.00 (4/4 tracks) | 1.25 (5/4 tracks) | +0.25 |
| **Average Congestion** | 0.4497 | 0.4507 | +0.0010 (+0.22%) |
| **Capacity Violations** | 0 | 2 | +2 |
| **Total Route Bends** | 188 bends | 216 bends | +28 bends (+14.9%) |
| **Execution Runtime** | **7.59 ms** | **6,477.28 ms** | **+6,469.68 ms (~853x)** |

**Key Takeaways**:
- **Wirelength Advantage**: Physarum's continuous Laplace potential relaxation allows it to find diagonal bypass corridors across parallel channels, saving **1.64% total wirelength**.
- **Runtime Trade-off**: Dijkstra's min-heap is ~850x faster ($0.078\text{ ms/net}$ vs $66.8\text{ ms/net}$), making Dijkstra ideal for real-time interactive previews and Physarum ideal for high-quality global routing optimization.

For detailed analysis, see [docs/BENCHMARKS.md](file:///Users/apple/Downloads/DSA_Project/docs/BENCHMARKS.md).

---

## 4. Project Layout

```text
PhysarumChip/
├── engine/                       # Standalone C11 Routing Engine
│   ├── include/                  # Public headers
│   │   ├── common.h              # Data structures, error codes, limits
│   │   ├── component.h           # Component library definitions
│   │   ├── architecture.h        # CPU architecture generator
│   │   ├── placement.h           # Floorplanning & overlap validation
│   │   ├── graph.h               # 2D routing grid & edge capacities
│   │   ├── dijkstra.h            # Binary min-heap & shortest path
│   │   ├── physarum.h            # Poiseuille flow, Gauss-Seidel & adaptation
│   │   ├── metrics.h             # Wirelength, congestion & bend calculation
│   │   └── json_io.h             # JSON serialization & deserialization
│   ├── src/                      # C11 implementation sources
│   ├── tests/                    # C unit test suite (77 tests)
│   ├── third_party/              # cJSON library (MIT)
│   └── Makefile                  # Engine build configuration
├── server/                       # Node.js + Express Bridge Server
│   ├── src/index.js              # Express app & C engine process invoker
│   ├── test_bridge.js            # Server API integration test
│   └── package.json
├── frontend/                     # React 18 + TypeScript + Vite UI
│   ├── src/
│   │   ├── components/
│   │   │   ├── Header.tsx        # Action bar (Route, Compare, Export, Reset)
│   │   │   ├── ChipCanvas.tsx    # Interactive SVG CAD canvas with zoom/pan
│   │   │   ├── ArchitecturePanel.tsx # Core counts, caches, topology settings
│   │   │   ├── NetlistPanel.tsx  # Searchable netlist & custom net adder
│   │   │   ├── ConstraintsPanel.tsx  # Hard capacities & soft weights
│   │   │   ├── ComparisonPanel.tsx   # Side-by-side metric tables
│   │   │   ├── InspectorPanel.tsx    # Block & Net property inspector
│   │   │   └── ExportModal.tsx   # JSON / CSV export and import
│   │   ├── types/chip.ts         # TypeScript schema definitions
│   │   ├── App.tsx               # Main workspace coordinator
│   │   └── index.css             # Tailored dark-mode CAD styling
│   ├── vite.config.ts
│   └── package.json
├── docs/                         # Technical documentation
│   ├── ALGORITHMS.md             # Mathematical derivations & SMA distinction
│   ├── ARCHITECTURE.md           # Software & hardware architecture
│   ├── BENCHMARKS.md             # Measured performance comparison
│   └── benchmark_results.json    # Machine-readable benchmark artifact
├── tests/                        # Acceptance verification
│   └── e2e_verification.js       # 18 automated criteria test runner
├── Makefile                      # Top-level workspace build orchestration
└── README.md
```

---

## 5. Quickstart & Usage

### Prerequisites
- C11 compliant compiler (`clang` or `gcc`)
- `make`
- Node.js (v18+) and `npm`

### Building the Project
From the repository root:

```bash
# Build engine, server, and frontend
make build
```

### Running the Application
```bash
# Start server and launch the interactive CAD interface
make run
```
Then navigate your browser to `http://localhost:3001`.

### Running Tests
```bash
# Run both the C unit test suite (77 tests) and End-to-End Acceptance suite (18 tests)
make test
```

### CLI Engine Direct Invocations
The C engine operates as a standalone CLI tool without requiring the web server:

```bash
# Check version
./engine/bin/physarum_chip --version

# Generate default 8P+8E architecture to JSON
./engine/bin/physarum_chip --generate-arch > arch.json

# Validate component placement
./engine/bin/physarum_chip --validate-placement < arch.json

# Route with Dijkstra baseline
./engine/bin/physarum_chip --route-dijkstra < arch.json > dijkstra_result.json

# Route with Physarum adaptive flow model
./engine/bin/physarum_chip --route-physarum < arch.json > physarum_result.json

# Execute direct comparison
./engine/bin/physarum_chip --compare < arch.json > comparison.json
```

---

## 6. REST API Endpoints

The Express server exposes the following JSON endpoints on port 3001:

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Healthcheck and C engine binary presence check |
| `POST` | `/api/architecture/generate` | Generates a parameterized CPU architecture & netlist |
| `POST` | `/api/placement/validate` | Validates die boundaries and detects block overlaps |
| `POST` | `/api/route/physarum` | Executes Physarum adaptive flow routing |
| `POST` | `/api/route/dijkstra` | Executes Dijkstra min-heap shortest path routing |
| `POST` | `/api/compare` | Runs both routers on the identical design and returns side-by-side diffs |

---

## 7. Interactive CAD Features

- **Zoom & Pan Canvas**: Smooth mouse-wheel zooming and drag-panning across the chip canvas with a 1-click "Reset View" button.
- **Drag-and-Drop Floorplanning**: Move any CPU block directly on the canvas. The engine validates overlaps in real-time, displaying green guides for legal placements and red outlines for invalid configurations.
- **Snap-to-Port Routing**: Terminal ports snap deterministically to perimeter centers, ensuring clean orthogonal wire takeoffs.
- **Multi-Layer Visualization**:
  - Toggle between routed wire overlays, block geometry, pinout dots, and grid lines.
  - **Congestion Heatmap**: Overlay color-coded edge demand / capacity ratios across all routing channels (green $\to$ yellow $\to$ red).
- **User-Drawn Keepouts**: Click and drag to create routing obstacle zones that immediately force routers to find alternate channels.
- **Dynamic Netlist Management**: Inspect individual nets, filter by component name, and add or delete user-defined nets.
- **Export & Import**: Export full designs to JSON, export results to CSV or JSON, and reload saved designs.

---

## 8. Definition of Done & Verification

All 18 automated criteria specified in `PhysarumChip_Spec/04_TESTS_AND_ACCEPTANCE.md` have been verified:

```text
=== PhysarumChip End-to-End Acceptance Verification ===
[PASS]  1. C Binary Exists and is Executable
[PASS]  2. CLI Version and Help Output
[PASS]  3. Default Architecture Generation via C Engine
[PASS]  4. Floorplan Placement Validity
[PASS]  5. Overlap Detection in Placement Validator
[PASS]  6. Netlist Generation Integrity
[PASS]  7. Dijkstra Routing Correctness
[PASS]  8. Physarum Routing Correctness
[PASS]  9. Engine Comparison Mode Execution
[PASS] 10. Express API Server Health Endpoint
[PASS] 11. API Architecture Generation
[PASS] 12. API Placement Validation
[PASS] 13. API Dijkstra Routing Bridge
[PASS] 14. API Physarum Routing Bridge
[PASS] 15. API Routing Comparison Bridge
[PASS] 16. Server Error Handling for Malformed Requests
[PASS] 17. Frontend Production Bundle Built
[PASS] 18. Frontend Serves Cleanly from Express Static Route
================================================================
Verification Summary: 18 / 18 Tests Passed (100%)
Status: ACCEPTANCE SUITE FULLY SATISFIED
```

---

## 9. Educational Scope & Limitations

PhysarumChip is designed for educational exploration of bio-inspired algorithms and VLSI principles. The following intentional simplifications are in place:
1. **Planar 2D Routing Grid**: The simulator routes on a single virtual 2D metal layer with orthogonal tracks and capacity constraints, rather than a full 14-layer industrial foundry stack.
2. **Simplified Timing & Parasitics**: Delay is represented as abstract wirelength and via bend penalties rather than full SPICE-level distributed RC delay equations (Elmore delay).
3. **Discrete Die Grid**: Component dimensions and coordinates snap to integer grid units ($1.0 \times 1.0$), reflecting coarse global routing rather than sub-nanometer standard cell placement.

---

## 10. License

PhysarumChip is released under the **MIT License**. Third-party `cJSON` library is included under its original permissive MIT license.
