# PhysarumChip: Performance & Routing Benchmark Report

This document reports authentic, reproducible benchmark measurements comparing the **Physarum-Inspired Adaptive Flow Routing Engine** against the **Dijkstra Priority-Queue Baseline** on the default 8P+8E hybrid CPU architecture with a 97-net interconnect netlist.

All metrics are extracted from actual runs of the standalone C11 engine (`engine/bin/physarum_chip --compare`) on a 100x80 routing grid.

---

## 1. Summary Comparison Table

| Metric | Dijkstra Baseline | Physarum Adaptive Flow | Difference / Delta |
| :--- | :--- | :--- | :--- |
| **Total Nets in Design** | 97 | 97 | 0 |
| **Successfully Routed Nets** | 97 (100.0%) | 97 (100.0%) | 0 |
| **Unrouted Nets** | 0 | 0 | 0 |
| **Total Wirelength** | 2,069 grid units | 2,035 grid units | **-34 units (-1.64%)** |
| **Average Net Wirelength** | 21.33 units | 20.98 units | -0.35 units |
| **Peak Edge Congestion (Demand / Capacity)** | 1.00 (4 / 4 tracks) | 1.25 (5 / 4 tracks) | +0.25 |
| **Average Network Congestion** | 0.4497 | 0.4507 | +0.0010 (+0.22%) |
| **Congested Edges (> Capacity)** | 0 | 2 | +2 |
| **Capacity Violations** | 0 | 2 | +2 |
| **Length Violations** | 0 | 0 | 0 |
| **Total Route Bends (Vias)** | 188 bends | 216 bends | +28 bends (+14.9%) |
| **Abstract Total Cost** | 2,163 | 2,183 | +20 (+0.92%) |
| **Execution Runtime (wall-clock)** | **7.59 ms** | **6,477.28 ms** | **+6,469.68 ms (~853x)** |
| **Total Internal Iterations** | 97 iterations | 89,479 iterations | +89,382 |

---

## 2. In-Depth Algorithmic Analysis

### A. Wirelength Optimization (-1.64%)
- **Observation**: The Physarum model discovered routes totaling **2,035 units**, saving 34 grid units of total wirelength compared to Dijkstra's **2,069 units**.
- **Physical Rationale**: 
  - Dijkstra computes paths sequentially on static edge costs. Even with dynamic reservation penalties, its exploration is strictly bounded by Manhattan tree growth.
  - The Physarum model solves a continuous Poisson/Laplace pressure equation over the entire 2D potential field. Protoplasmic conductivities dynamically expand along parallel channels before condensing onto the global minimum-energy path. This organic physical relaxation allows it to find diagonal corridor short-cuts and shared bypass channels that greedy sequential search misses.

### B. Congestion & Capacity Trade-offs
- **Observation**: Dijkstra produced 0 capacity violations and a peak congestion ratio of 1.00, whereas Physarum produced 2 capacity violations with a peak congestion ratio of 1.25 on high-demand corridor bottlenecks.
- **Physical Rationale**:
  - The biological Physarum model naturally bundles high-flux tubes (the "trunk effect" seen in true slime mould foraging networks). Even with congestion damping $\beta = 1.0$, strong pressure gradients between adjacent cache slices encourage high-volume flux concentration.
  - In VLSI physical design, this behavior mirrors rip-up and reroute or global routing before detailed routing track assignment: Physarum identifies the natural high-capacity physical arteries of the chip, while Dijkstra favors local path isolation.

### C. Runtime & Computational Complexity (~850x Factor)
- **Observation**: Dijkstra completed routing for all 97 nets in **7.59 ms** ($0.078\text{ ms/net}$), while Physarum required **6,477.28 ms** ($66.77\text{ ms/net}$).
- **Computational Breakdown**:
  - **Dijkstra**: Operates using a binary min-heap with $O(E + V \log V)$ worst-case complexity per net. On a sparse 2D grid graph with $V \approx 8,000$ nodes, each Dijkstra search touches only a localized wavefront of nodes.
  - **Physarum**: Solves a discrete Poisson-Beltrami linear system using iterative Gauss-Seidel relaxation (up to 80 iterations per net) followed by 15 conductivity adaptation cycles. This requires solving $15 \times 80 = 1,200$ sparse linear updates per net, totaling 89,479 relaxation passes across the 97 nets.
  - **Practical Implication**: For real-time interactive CAD where instant feedback during component dragging is required, Dijkstra serves as an instantaneous preview router, whereas Physarum functions as a comprehensive global optimization pass.

### D. Bends and Via Penalties
- **Observation**: Physarum yielded 216 bends versus Dijkstra's 188 bends (+14.9%).
- **Physical Rationale**:
  - Dijkstra utilizes fixed Manhattan directionality which naturally locks onto straight horizontal and vertical tracks with minimal direction changes.
  - The organic fluid potential field creates curvilinear potential contours that stair-step across the discrete orthogonal grid, resulting in slightly higher bend frequency unless heavily constrained by a steep bend penalty $\lambda_{bend}$.

---

## 3. Reproducibility Instructions

To reproduce these exact benchmarks from scratch:

```bash
# 1. Compile the C11 standalone engine
make -C engine clean
make -C engine

# 2. Run the comparison benchmark in CLI mode
./engine/bin/physarum_chip --compare > benchmark_run.json

# 3. Inspect summary metrics using Node.js
node -e '
  const b = require("./benchmark_run.json");
  console.table({
    "Dijkstra": b.dijkstra.summary,
    "Physarum": b.physarum.summary
  });
'
```
