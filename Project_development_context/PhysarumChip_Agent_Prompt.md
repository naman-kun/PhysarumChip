# Initial Prompt for the Coding Agent

Build **PhysarumChip** from scratch in the current empty project folder. First read `PhysarumChip_Spec/00_START_HERE.md`, then follow its phased reading order and consult the relevant specification file before implementing each phase. Do not ingest every specification into one giant prompt unnecessarily.

All core domain logic and algorithms must be implemented in C11. Use React + TypeScript + Vite for the UI and a thin Node.js + Express bridge to invoke the standalone C engine. The primary routing method is a Physarum-inspired conductivity/flow adaptation model; Dijkstra is a separate baseline. Do not conflate this with the distinct population-based standard Slime Mould Algorithm (SMA).

Do not stop after planning or scaffolding. Build, test, integrate, and document the complete local application, starting from an empty folder. The required default design is an illustrative CPU architecture with 8 performance cores, 8 efficiency cores, corresponding L1 caches, private performance-core L2 caches, shared efficiency-cluster L2 caches, four last-level cache slices, two memory controllers, one system agent, one PCIe/I/O controller, and a selectable Ring/Mesh/Crossbar logical interconnect.

Use `PhysarumChip_Spec/04_TESTS_AND_ACCEPTANCE.md` as the definition of done. Report actual implementation and test results honestly; never present placeholders or fabricated metrics as real output.
