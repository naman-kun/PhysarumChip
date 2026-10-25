# PhysarumChip: Software & Hardware Architecture

## 1. System Structure

```text
┌────────────────────────────────────────────────────────────────────────┐
│                   PhysarumChip Modern CAD Interface                    │
│                 (React 18 + TypeScript + Vite + SVG)                   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ HTTP JSON API (Port 3001)
┌───────────────────────────────────▼────────────────────────────────────┐
│                    Node.js + Express Bridge Server                     │
│                  - Input validation & child_process                    │
│                  - Piped stdin/stdout JSON transport                   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ CLI Executable Subprocess
┌───────────────────────────────────▼────────────────────────────────────┐
│                       PhysarumChip C11 Engine                          │
│                                                                        │
│   ┌───────────────┐  ┌───────────────┐  ┌───────────────────────────┐  │
│   │ Component &   │  │ Deterministic │  │ 2D Grid Routing Graph     │  │
│   │ Template Lib  │  │ Floorplanner  │  │ (Obstacles, Ports, Tracks)│  │
│   └───────────────┘  └───────────────┘  └─────────────┬─────────────┘  │
│                                                       │                │
│                      ┌────────────────────────────────┴───────────┐    │
│                      │                                            │    │
│            ┌─────────▼─────────┐                        ┌─────────▼──┐ │
│            │  Physarum Flow    │                        │  Dijkstra  │ │
│            │  Adaptation Model │                        │  Baseline  │ │
│            │  (Tero et al.)    │                        │  Min-Heap  │ │
│            └─────────┬─────────┘                        └─────────┬──┘ │
│                      │                                            │    │
│                      └────────────────┬───────────────────────────┘    │
│                                       │                                │
│                              ┌────────▼────────┐                       │
│                              │ Metrics, Costs  │                       │
│                              │ & cJSON Parser  │                       │
│                              └─────────────────┘                       │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Default Illustrative CPU Floorplan Architecture

The default hardware design represents an educational multi-core CPU architecture:
- **8 Performance Cores (P-Cores)**: Large compute cores ($10 \times 10$ units, 15W, 100 mm²).
- **8 Efficiency Cores (E-Cores)**: Compact high-efficiency cores ($5 \times 5$ units, 3W, 25 mm²).
- **L1 Cache Hierarchy**: Dedicated private L1 Instruction and L1 Data caches per core.
- **L2 Cache Hierarchy**:
  - Dedicated private L2 caches ($8 \times 6$ units) for each of the 8 P-Cores.
  - Shared cluster L2 caches ($8 \times 6$ units) shared by clusters of 4 E-Cores (2 clusters default).
- **4 Last-Level Cache (LLC) Slices**: Distributed coherent cache slices along the central interconnect spine.
- **2 Dual-Channel Memory Controllers**: Positioned along die peripheral edges for external DRAM interface.
- **1 System Agent / Power Control Unit**: Manages boot, power gating, and uncore coordination.
- **1 PCIe / IO Root Complex**: Peripheral hub controller.
- **Selectable Interconnect Topologies**:
  - **Ring**: Bidirectional circular bus with 14 router stops.
  - **2D Mesh**: Planar Manhattan network with XY routing hops.
  - **Crossbar**: Star backplane connecting all major subcomponents.

---

## 3. Communication & JSON Exchange Protocol

Communication between the Express bridge and C executable occurs via standard streams (`stdin` / `stdout`) with explicit CLI flags:

| Mode Flag | Input Payload (stdin) | Output Payload (stdout) |
|---|---|---|
| `--version` | None | `{"version": "1.0.0", "engine": "PhysarumChip C11 Core"}` |
| `--generate-arch` | Optional `ArchConfig` JSON | Full `Architecture` JSON (components, ports, default netlist, placement) |
| `--validate-placement` | `Architecture` JSON + `min_clearance` | `{"success": true}` or `{"success": false, "error": "..."}` |
| `--route-physarum` | `{"architecture": {...}, "routing_params": {...}}` | `RoutingResults` JSON (routes, active segments, metrics) |
| `--route-dijkstra` | `{"architecture": {...}, "routing_params": {...}}` | `RoutingResults` JSON (routes, active segments, metrics) |
| `--compare` | `{"architecture": {...}, "routing_params": {...}}` | `ComparisonReport` JSON (side-by-side results and differences) |
