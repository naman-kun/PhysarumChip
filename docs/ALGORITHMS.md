# PhysarumChip: Mathematical & Algorithmic Foundations

## 1. Algorithmic Identity: Flow Adaptation vs. Slime Mould Algorithm (SMA)

A critical distinction in this project:
- **Physarum-Inspired Conductivity/Flow Adaptation Model (Implemented Here)**:
  Based on the biological fluid mechanics model discovered by Tero, Kobayashi, and Nakagaki (2007). It models the acellular slime mould *Physarum polycephalum* as a continuous resistive network where protoplasmic tube conductivities dynamically expand with flux and atrophy with disuse according to Poiseuille's law and Kirchoff's flow conservation.
- **Slime Mould Algorithm (SMA)**:
  A distinct population-based metaheuristic optimization algorithm proposed by Li et al. (2020) for general numerical function optimization using stochastic particle search formulas.
  
**PhysarumChip implements the genuine fluid/flow adaptation model**, which maps directly to physical graph-based routing, congestion mitigation, and network design.

---

## 2. Mathematical Model of Physarum Flow Adaptation

Let $G = (V, E)$ be a 2D routing grid graph representing available on-chip wire channels.

### A. Flux and Poiseuille Flow Equation
For each edge $e = (u, v) \in E$, let:
- $L(e)$ be the physical segment length (grid pitch).
- $D(e)$ be the hydraulic/electrical conductivity of the edge ($D(e) \ge D_{min} > 0$).
- $P(u)$ and $P(v)$ be the node pressures (potentials).

The flow (flux) $Q(e)$ through edge $e$ from node $u$ to node $v$ follows Ohm's / Poiseuille's law:
$$Q(e) = \frac{D(e)}{L(e)} (P(u) - P(v))$$

### B. Flow Conservation (Kirchhoff's Current Law)
At every internal node $u \in V \setminus \{s, t\}$, mass conservation requires zero net divergence:
$$\sum_{v \in N(u)} Q(u, v) = S(u)$$

Where boundary injection currents are:
- $S(s) = +I_0$ at the source node $s$.
- $S(t) = -I_0$ at the target node $t$.
- $S(u) = 0$ for all internal nodes $u \neq s, t$.

Substituting the flow equation yields the discrete Laplace-Beltrami linear system:
$$\sum_{v \in N(u)} \frac{D(u, v)}{L(u, v)} (P(u) - P(v)) = S(u)$$

### C. Pressure Field Relaxation (Gauss-Seidel / Successive Over-Relaxation)
Fixing the target node as reference ground ($P(t) = 0.0$), the system is strictly diagonally dominant:
$$P^{(k+1)}(u) = \frac{S(u) + \sum_{v \in N(u)} G(u, v) P(v)}{\sum_{v \in N(u)} G(u, v)}$$
where conductance $G(u, v) = \frac{D(u, v)}{L(u, v)}$.

The iteration converges when:
$$\max_{u \in V} |P^{(k+1)}(u) - P^{(k)}(u)| < \epsilon$$
with an enforced iteration ceiling $K_{max} = 80$ to guarantee bounded runtime.

### D. Dynamic Tube Adaptation Rule
After calculating edge flow magnitudes $|Q(e)|$, conductivities are updated via biological reinforcement and exponential decay:
$$D^{(m+1)}(e) = \max\left(D_{min}, \min\left(D_{max}, (1 - \rho) D^{(m)}(e) + \alpha |Q(e)|\right)\right)$$
where:
- $\rho \in (0, 1)$ is the decay rate (atrophy of idle routes).
- $\alpha > 0$ is the reinforcement strength (expansion of active conduits).
- $D_{min} > 0$ ensures connectivity is never permanently severed.
- $D_{max}$ prevents numerical divergence.

### E. Congestion Feedback & Soft Objective Penalty
To prevent multiple nets from crowding into a single routing corridor, edge conductance is damped by existing traffic demand:
$$G_{eff}(e) = \frac{D(e)}{L(e)} \cdot \frac{1.0}{1.0 + \beta \left(\frac{\text{demand}(e) + \text{net\_demand}}{\text{capacity}(e)}\right)^2}$$
where $\beta$ is the congestion resistance factor. High-demand channels present high fluid resistance, driving the adaptive flow into uncongested parallel bypass corridors.

### F. Tube Path Extraction
Once the tubular network stabilizes over $M$ adaptation cycles, the optimal loop-free path is extracted by finding the path of minimum resistance:
$$\text{Cost}(e) = \frac{L(e)}{D(e)} \cdot \left(1.0 + \beta \left(\frac{\text{demand}(e) + \text{net\_demand}}{\text{capacity}(e)}\right)^2\right) + \text{Bend\_Penalty}$$

---

## 3. Dijkstra Baseline Algorithm

The Dijkstra baseline evaluates standard shortest-path routing over static edge weights:
$$\text{Cost}_{Dijkstra}(e) = L(e) \cdot \left(1.0 + \beta \left(\frac{\text{demand}(e) + \text{net\_demand}}{\text{capacity}(e)}\right)^2\right) + \text{Bend\_Penalty}$$

Implemented using a binary min-heap with $O(\log V)$ `decrease_key` through a reverse node index map. Nets are processed in descending priority order ($5 \to 1$).

---

## 4. Complexity Analysis

| Component | Time Complexity | Space Complexity |
|---|---|---|
| Routing Graph Construction | $O(V + E)$ where $V = W \times H$, $E \approx 4V$ | $O(V + E)$ |
| Floorplan Placement Heuristic | $O(C^2)$ where $C$ is component count ($\le 256$) | $O(C)$ |
| Dijkstra Path Search (Single Net) | $O((V + E) \log V)$ | $O(V)$ |
| Physarum Pressure Relaxation (1 Step) | $O(K_{iter} \cdot E)$ where $K_{iter} \le 80$ | $O(V)$ |
| Physarum Full Adaptation (1 Net) | $O(M \cdot K_{iter} \cdot E + (V + E) \log V)$ where $M \le 20$ | $O(V + E)$ |
| Full Architecture Routing ($K$ nets) | $O(K \cdot [M \cdot K_{iter} \cdot E + (V + E) \log V])$ | $O(V + E + K)$ |

For the default 8P+8E CPU architecture ($101 \times 81$ grid, $V \approx 8,181$ nodes, $E \approx 16,180$ edges, $K = 97$ nets):
- Dijkstra completes in **$\sim 8 - 15$ milliseconds**.
- Physarum completes in **$\sim 1.1 - 1.5$ seconds**, performing thorough fluid flow optimization across thousands of grid edges.
