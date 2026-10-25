export type ComponentType =
  | 'P_CORE'
  | 'E_CORE'
  | 'L1_INST'
  | 'L1_DATA'
  | 'L2_CACHE'
  | 'LLC_SLICE'
  | 'MEM_CTRL'
  | 'SYSTEM_AGENT'
  | 'PCIE_IO'
  | 'ROUTER'
  | 'UNKNOWN';

export type InterconnectTopology = 'RING' | 'MESH_2D' | 'CROSSBAR';
export type NetType = 'DATA' | 'CONTROL' | 'CLOCK' | 'POWER';

export interface Port {
  name: string;
  rel_x: number;
  rel_y: number;
  abs_x: number;
  abs_y: number;
}

export interface RectBounds {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface ComponentInstance {
  id: number;
  name: string;
  type: ComponentType;
  cluster_id: number;
  orientation: number;
  power_estimate: number;
  area_estimate: number;
  bounds: RectBounds;
  ports: Port[];
}

export interface LogicalNet {
  id: number;
  name: string;
  source_comp_id: number;
  source_port: string;
  target_comp_id: number;
  target_port: string;
  type: NetType;
  priority: number;
  track_demand: number;
  max_length: number;
}

export interface ArchConfig {
  num_p_cores: number;
  num_e_cores: number;
  e_cluster_size: number;
  num_llc_slices: number;
  num_mem_ctrls: number;
  has_system_agent: boolean;
  has_pcie_io: boolean;
  topology: InterconnectTopology;
  die_width: number;
  die_height: number;
  routing_pitch: number;
}

export interface Architecture {
  config: ArchConfig;
  components: ComponentInstance[];
  nets: LogicalNet[];
  total_estimated_area: number;
  total_estimated_power: number;
}

export interface DijkstraConstraints {
  bend_penalty: number;
  congestion_weight: number;
  enforce_hard_capacity: boolean;
}

export interface PhysarumParams {
  rho: number;
  alpha: number;
  d_min: number;
  d_max: number;
  source_current: number;
  max_pressure_iters: number;
  adaptation_steps: number;
  congestion_weight: number;
  bend_penalty: number;
  enforce_hard_capacity: boolean;
}

export interface BlockedRegion {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface RoutingParams {
  default_edge_capacity: number;
  dijkstra_constraints: DijkstraConstraints;
  physarum_params: PhysarumParams;
  blocked_regions: BlockedRegion[];
}

export interface PathPoint {
  gx: number;
  gy: number;
  x: number;
  y: number;
}

export interface NetRouteRecord {
  net_id: number;
  net_name: string;
  routed: boolean;
  demand: number;
  wirelength: number;
  bends: number;
  max_edge_congestion: number;
  fail_reason?: string;
  path?: PathPoint[];
}

export interface ActiveSegment {
  x1: number;
  y1: number;
  x2: number;
  y2: number;
  demand: number;
  capacity: number;
  ratio: number;
}

export interface RoutingSummary {
  total_nets: number;
  routed_nets: number;
  unrouted_nets: number;
  success_rate: number;
  total_wirelength: number;
  avg_wirelength: number;
  peak_congestion: number;
  avg_congestion: number;
  congested_edge_count: number;
  capacity_violations: number;
  length_violations: number;
  total_bends: number;
  abstract_total_cost: number;
  runtime_ms: number;
  total_iterations: number;
}

export interface RoutingResults {
  algorithm: string;
  summary: RoutingSummary;
  routes: NetRouteRecord[];
  active_segments?: ActiveSegment[];
}

export interface ComparisonDifferences {
  wirelength_diff_pct: number;
  peak_congestion_diff: number;
  avg_congestion_diff: number;
  runtime_diff_ms: number;
}

export interface ComparisonReport {
  physarum: RoutingResults;
  dijkstra: RoutingResults;
  differences: ComparisonDifferences;
}
