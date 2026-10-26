# Line-by-Line Code Explanation: `engine/src/physarum.c`

This document provides a beginner-friendly, line-by-line explanation of the core routing algorithm implemented in [`engine/src/physarum.c`](file:///Users/apple/Downloads/DSA_Project/engine/src/physarum.c).

---

## 1. High-Level Intuition (The Water Pipe Analogy)

Imagine you have a grid representing all the possible streets and alleyways on a silicon chip. You want to connect a **Start Pin** (e.g., CPU Core 0) to a **Target Pin** (e.g., L2 Cache).

Instead of guessing roads one by one, the Physarum algorithm treats the entire grid as a **network of garden water hoses**:
1. You turn on a high-pressure pump at the **Start Pin**.
2. You open a zero-pressure drain at the **Target Pin**.
3. Water pushes through every possible route across the chip.
4. **The Biological Rule**:
   - Any pipe carrying lots of rushing water **expands and gets wider**.
   - Any pipe with little or no water **shrinks, withers, and decays**.
5. After 15 rounds of fluid flow and growth, the chaotic web collapses into **one thick, low-resistance artery** — the optimal wire route!

---

## 2. Line-by-Line Code Breakdown

---

### Part 1: Setting Default Parameters (`physarum_default_params`)

```c
1: #include "../include/physarum.h"
```
* **Line 1:** Includes the header file containing definitions for structs like `RoutingGraph`, `RoutingEdge`, `RoutePath`, and `PhysarumParams`.

```c
3: PhysarumParams physarum_default_params(void) {
4:     PhysarumParams p;
```
* **Lines 3–4:** A helper function that bundles the default physics simulation knobs into a struct `p` and returns it.

```c
5:     p.rho = 0.15;
```
* **Line 5:** **Decay Rate ($\rho = 15\%$)**: Idle pipes that carry little water shrink by 15% each round.

```c
6:     p.alpha = 1.2;
```
* **Line 6:** **Growth Rate ($\alpha = 1.2$)**: How aggressively a pipe widens when water rushes through it.

```c
7:     p.d_min = 0.05;
8:     p.d_max = 50.0;
```
* **Lines 7–8:** Safety limits:
  * `d_min`: Pipes never shrink below width `0.05` (so paths are never permanently dead-ended).
  * `d_max`: Pipes cannot grow wider than `50.0` (prevents numerical overflow).

```c
9:     p.source_current = 1.0;
```
* **Line 9:** The flow rate injected by the "pump" at the starting pin.

```c
10:     p.max_pressure_iters = 80;
11:     p.pressure_tolerance = 1e-4;
```
* **Lines 10–11:** Pressure solver limits:
  * We calculate water pressure across the chip for up to `80` iterations.
  * If the pressure change between iterations drops below `0.0001` (`1e-4`), fluid flow has stabilized, and we stop early.

```c
12:     p.adaptation_steps = 15;
```
* **Line 12:** The number of biological evolution cycles (15 rounds of flowing water, expanding busy pipes, and shrinking idle pipes).

```c
13:     p.congestion_weight = 3.0;
14:     p.bend_penalty = 0.3;
```
* **Lines 13–14:** Circuit design penalties:
  * `congestion_weight`: Adds resistance to roads that are already crowded with other wires.
  * `bend_penalty`: Adds extra cost to 90-degree corners, encouraging straight wire segments.

```c
15:     p.enforce_hard_capacity = false;
16:     p.reroute_passes = 1;
17:     return p;
18: }
```
* **Lines 15–18:** Default capacity flags and returning the completed `p` struct.

---

### Part 2: Function Signature & Input Validation

```c
20: bool physarum_route_single_net(RoutingGraph* graph,
21:                                int start_node,
22:                                int target_node,
23:                                const PhysarumParams* params,
24:                                int net_demand,
25:                                RoutePath* out_path,
26:                                int* out_pressure_iters,
27:                                int* out_adapt_iters) {
```
* **Lines 20–27:** The main routing function.
  * `graph`: The grid of wire channels.
  * `start_node`, `target_node`: The integer IDs of the start and target pins.
  * `net_demand`: Number of wire tracks required (typically 1 or 2).
  * `out_path`: Pointer where the final list of coordinates and stats will be written.
  * `out_pressure_iters`, `out_adapt_iters`: Pointers to store diagnostic statistics.

```c
28:     if (!graph || !out_path) return false;
29:     memset(out_path, 0, sizeof(RoutePath));
30:     out_path->is_valid = false;
```
* **Lines 28–30:** Defensive check. If input pointers are NULL, exit immediately. Zeroes out the output structure.

```c
32:     if (start_node < 0 || start_node >= graph->node_count || target_node < 0 || target_node >= graph->node_count) {
33:         snprintf(out_path->fail_reason, MAX_NAME_LEN, "Invalid endpoint node indices (%d, %d)", start_node, target_node);
34:         return false;
35:     }
```
* **Lines 32–35:** Boundary check. Ensures the start and target node IDs actually exist inside the grid.

```c
37:     if (start_node == target_node) {
38:         out_path->node_count = 1;
39:         out_path->nodes[0] = start_node;
40:         out_path->is_valid = true;
41:         out_path->total_length = 0.0;
42:         return true;
43:     }
```
* **Lines 37–43:** Edge case. If the start and target are the same pin, wirelength is `0.0`, and routing succeeds immediately.

---

### Part 3: Memory Allocation for Simulation Arrays

```c
45:     PhysarumParams p = params ? *params : physarum_default_params();
46:     int N = graph->node_count;
47:     int E = graph->edge_count;
```
* **Lines 45–47:** Uses passed parameters (or defaults), and gets the total number of nodes ($N$) and edges ($E$) in the graph.

```c
50:     double* P = (double*)calloc(N, sizeof(double));
51:     double* D = (double*)malloc(sizeof(double) * E);
52:     double* Q = (double*)calloc(E, sizeof(double));
```
* **Lines 50–52:** Allocates dynamic arrays on the heap:
  * `P`: Pressure potential at each node ($N$ doubles, initialized to 0.0).
  * `D`: Conductivity / diameter of each edge ($E$ doubles).
  * `Q`: Fluid flux / flow through each edge ($E$ doubles, initialized to 0.0).

```c
54:     if (!P || !D || !Q) {
55:         if (P) free(P);
56:         if (D) free(D);
57:         if (Q) free(Q);
58:         snprintf(out_path->fail_reason, MAX_NAME_LEN, "Memory allocation failed");
59:         return false;
60:     }
```
* **Lines 54–60:** Memory safety check. If allocation fails, clean up and return `false`.

---

### Part 4: Initializing Edge Conductivities

```c
62:     // 1. Initialize edge conductivities
63:     for (int e = 0; e < E; ++e) {
64:         const RoutingEdge* edge = &graph->edges[e];
65:         if (edge->is_blocked) {
66:             D[e] = 1e-6;
67:         } else {
68:             D[e] = 1.0;
69:         }
70:     }
```
* **Lines 63–70:** Loops through all roads $e$:
  * If the road is inside an obstacle or CPU block (`is_blocked`), set its diameter to microscopic $10^{-6}$ (virtually no water can pass).
  * Otherwise, initialize diameter to `1.0` (all open roads start with equal potential).

```c
72:     int total_pressure_iters = 0;
73:     int adapt_step = 0;
```
* **Lines 72–73:** Counters for tracking total relaxation passes.

---

### Part 5: The Adaptation Loop (Fluid Physics & Biological Growth)

```c
76:     for (adapt_step = 0; adapt_step < p.adaptation_steps; ++adapt_step) {
```
* **Line 76:** Main evolution loop running for 15 rounds.

```c
78:         P[target_node] = 0.0;
```
* **Line 78:** Fixes the target pin as **ground reference** ($P(\text{target}) = 0.0$).

#### Solving Node Pressures (Gauss-Seidel Iteration)
```c
81:         int p_iter;
82:         for (p_iter = 0; p_iter < p.max_pressure_iters; ++p_iter) {
83:             double max_diff = 0.0;
```
* **Lines 81–83:** Pressure relaxation loop (up to 80 passes). `max_diff` tracks the largest change in pressure.

```c
85:             for (int u = 0; u < N; ++u) {
86:                 if (u == target_node) continue; // Reference ground stays 0.0
87:                 if (graph->nodes[u].is_blocked && u != start_node) continue;
```
* **Lines 85–87:** Loops over every node $u$. Skips the ground node and blocked nodes.

```c
89:                 double sum_G = 0.0;
90:                 double sum_GP = 0.0;
```
* **Lines 89–90:** Accumulators for total conductance ($\sum G$) and neighbor pressure-flux ($\sum G \cdot P$).

```c
92:                 const RoutingNode* nu = &graph->nodes[u];
93:                 for (int i = 0; i < nu->degree; ++i) {
94:                     int v = nu->neighbor_nodes[i];
95:                     int e_idx = nu->incident_edges[i];
96:                     const RoutingEdge* edge = &graph->edges[e_idx];
```
* **Lines 92–96:** Examines every neighbor $v$ connected to node $u$ through road `edge`.

```c
98:                     if (edge->is_blocked) continue;
99:                     if (graph->nodes[v].is_blocked && v != target_node && v != start_node) continue;
```
* **Lines 98–99:** Skips blocked roads or blocked neighbor nodes.

```c
102:                     if (p.enforce_hard_capacity && (edge->demand + net_demand > edge->capacity)) {
103:                         continue;
104:                     }
```
* **Lines 102–104:** If hard capacity is enforced and the road is full, skip it.

```c
107:                     double cong_factor = 1.0;
108:                     double cr = (double)(edge->demand + net_demand) / (double)edge->capacity;
109:                     if (cr > 0.8) {
110:                         cong_factor = 1.0 / (1.0 + p.congestion_weight * (cr * cr));
111:                     }
```
* **Lines 107–111:** **Congestion Damping**: If the road is over 80% full, choke its conductance down so water naturally diverts around crowded highways.

```c
113:                     double G = (D[e_idx] / edge->length) * cong_factor;
114:                     sum_G += G;
115:                     sum_GP += G * P[v];
116:                 }
```
* **Lines 113–116:** Computes conductance $G = \frac{D}{L} \times \text{cong\_factor}$.

```c
118:                 if (sum_G > 1e-9) {
120:                     double I_u = (u == start_node) ? p.source_current : 0.0;
121:                     double new_P = (I_u + sum_GP) / sum_G;
122:                     double diff = fabs(new_P - P[u]);
123:                     if (diff > max_diff) max_diff = diff;
124:                     P[u] = new_P;
125:                 }
126:             }
```
* **Lines 118–126:** **Kirchhoff's Flow Conservation**: The pressure at node $u$ is the weighted average of its neighbors' pressures plus any injected current $I_u$. Updates $P[u]$.

```c
128:             if (max_diff < p.pressure_tolerance) {
129:                 p_iter++;
130:                 break;
131:             }
132:         }
```
* **Lines 128–132:** Convergence check. If the largest change was less than `0.0001`, pressure has stabilized; stop early.

---

### Part 6: Calculating Flow & Updating Tube Conductivities

```c
137:         for (int e = 0; e < E; ++e) {
138:             const RoutingEdge* edge = &graph->edges[e];
139:             if (edge->is_blocked) continue;
```
* **Lines 137–139:** Loops over all edges to update their widths.

```c
141:             int u = edge->u;
142:             int v = edge->v;
143:             double pot_diff = fabs(P[u] - P[v]);
144:             Q[e] = (D[e] / edge->length) * pot_diff;
```
* **Lines 141–144:** **Poiseuille's Law**: Flow $Q(e) = \frac{D(e)}{L(e)} \cdot |P(u) - P(v)|$.

```c
148:             double d_new = (1.0 - p.rho) * D[e] + p.alpha * Q[e];
149:             if (d_new < p.d_min) d_new = p.d_min;
150:             if (d_new > p.d_max) d_new = p.d_max;
151:             D[e] = d_new;
152:         }
153:     }
```
* **Lines 148–152:** **Conductivity Update Equation**:
  $$D_{\text{new}}(e) = (1 - \rho) D(e) + \alpha Q(e)$$
  - Unused roads lose 15% diameter.
  - Active roads expand by $1.2 \times Q$.
  - Clamped between `d_min` (`0.05`) and `d_max` (`50.0`).

---

### Part 7: Tracing the Winning Wire Path

```c
161:     double* dist = (double*)malloc(sizeof(double) * N);
162:     int* parent = (int*)malloc(sizeof(int) * N);
163:     bool* visited = (bool*)calloc(N, sizeof(bool));
```
* **Lines 161–163:** Arrays for running Dijkstra over the adapted network.

```c
165:     for (int i = 0; i < N; ++i) {
166:         dist[i] = 1e12;
167:         parent[i] = -1;
168:     }
170:     MinHeap* heap = min_heap_create(N);
171:     dist[start_node] = 0.0;
172:     min_heap_insert_or_decrease(heap, start_node, 0.0);
```
* **Lines 165–172:** Initializes distances to infinity, starts from `start_node` at distance `0.0`.

```c
175:     while (!min_heap_is_empty(heap)) {
176:         MinHeapNode curr;
177:         if (!min_heap_extract_min(heap, &curr)) break;
178:         int u = curr.node;
183:         if (u == target_node) { reached = true; break; }
```
* **Lines 175–186:** Standard priority queue search: visits the node with lowest resistance.

```c
211:             double resistance = (edge->length / (D[e_idx] + 1e-4)) * cong_pen;
```
* **Line 211:** **Resistance Cost**: $\text{Resistance} = \frac{\text{Length}}{\text{Diameter}}$. Fat pipes have nearly zero resistance!

```c
214:             if (parent[u] != -1) {
215:                 int prev_u = parent[u];
216:                 int prev_dx = nu->gx - graph->nodes[prev_u].gx;
217:                 int prev_dy = nu->gy - graph->nodes[prev_u].gy;
218:                 int next_dx = graph->nodes[v].gx - nu->gx;
219:                 int next_dy = graph->nodes[v].gy - nu->gy;
221:                 if ((prev_dx != 0 && next_dy != 0) || (prev_dy != 0 && next_dx != 0)) {
222:                     resistance += p.bend_penalty;
223:                 }
224:             }
```
* **Lines 214–224:** If taking edge $(u, v)$ creates a 90-degree corner relative to where we came from, add `bend_penalty` (`0.3`).

```c
226:             double new_dist = dist[u] + resistance;
227:             if (new_dist < dist[v]) {
228:                 dist[v] = new_dist;
229:                 parent[v] = u;
230:                 min_heap_insert_or_decrease(heap, v, new_dist);
231:             }
```
* **Lines 226–231:** Updates distance and parent pointer if a cheaper path to $v$ is discovered.

---

### Part 8: Path Reconstruction & Memory Cleanup

```c
244:     // Reconstruct path
245:     int temp_path[MAX_PATH_NODES];
246:     int count = 0;
247:     int curr = target_node;
248:     while (curr != -1 && count < MAX_PATH_NODES) {
249:         temp_path[count++] = curr;
250:         curr = parent[curr];
251:     }
```
* **Lines 244–251:** Traces parent pointers backward from `target_node` to `start_node`.

```c
253:     out_path->node_count = count;
254:     for (int i = 0; i < count; ++i) {
255:         out_path->nodes[i] = temp_path[count - 1 - i];
256:     }
257:     out_path->is_valid = true;
```
* **Lines 253–257:** Reverses the array so the wire path runs forward from Start to Target.

```c
266:     for (int i = 0; i < out_path->node_count - 1; ++i) {
267:         int u = out_path->nodes[i];
268:         int v = out_path->nodes[i + 1];
272:         out_path->total_length += e->length;
...
284:         out_path->total_bends++;
```
* **Lines 266–287:** Computes total physical wirelength, peak congestion ratio, and counts 90-degree corner bends.

```c
291:     free(P); free(D); free(Q);
292:     free(dist); free(parent); free(visited);
293:     return true;
294: }
```
* **Lines 291–294:** **RAM Cleanup**: Frees all allocated working arrays, preventing memory leaks, and returns `true`.
