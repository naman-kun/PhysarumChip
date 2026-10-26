import { Architecture, ArchConfig, RoutingParams, RoutingResults, ComparisonReport } from './types';

const BASE_URL = import.meta.env.VITE_API_URL || '/api';

export async function checkHealth() {
  const res = await fetch(`${BASE_URL}/health`);
  if (!res.ok) throw new Error(`Health check failed: HTTP ${res.status}`);
  return res.json();
}

export async function generateArchitecture(config?: Partial<ArchConfig>): Promise<Architecture> {
  const res = await fetch(`${BASE_URL}/architecture/generate`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(config || {})
  });
  const data = await res.json();
  if (!res.ok || data.success === false) {
    throw new Error(data.error || `Architecture generation failed: HTTP ${res.status}`);
  }
  return data;
}

export async function validatePlacement(arch: Architecture, minClearance = 1.0): Promise<{ success: boolean; error?: string }> {
  const payload = {
    ...arch,
    min_clearance: minClearance
  };
  const res = await fetch(`${BASE_URL}/architecture/validate-placement`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  const data = await res.json();
  if (!res.ok || data.success === false) {
    return { success: false, error: data.error || 'Placement validation failed' };
  }
  return { success: true };
}

export async function routePhysarum(payload: { architecture: Architecture; routing_params: RoutingParams }): Promise<RoutingResults> {
  const res = await fetch(`${BASE_URL}/route/physarum`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  const data = await res.json();
  if (!res.ok || data.success === false) {
    throw new Error(data.error || `Physarum routing failed: HTTP ${res.status}`);
  }
  return data;
}

export async function routeDijkstra(payload: { architecture: Architecture; routing_params: RoutingParams }): Promise<RoutingResults> {
  const res = await fetch(`${BASE_URL}/route/dijkstra`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  const data = await res.json();
  if (!res.ok || data.success === false) {
    throw new Error(data.error || `Dijkstra routing failed: HTTP ${res.status}`);
  }
  return data;
}

export async function compareRoutes(payload: { architecture: Architecture; routing_params: RoutingParams }): Promise<ComparisonReport> {
  const res = await fetch(`${BASE_URL}/route/compare`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  const data = await res.json();
  if (!res.ok || data.success === false) {
    throw new Error(data.error || `Comparison failed: HTTP ${res.status}`);
  }
  return data;
}
