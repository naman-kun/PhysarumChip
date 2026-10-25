import React from 'react';
import { Info, Box, Network, CheckCircle2, XCircle, ArrowRight } from 'lucide-react';
import { Card, CardHeader, CardTitle, CardContent, Badge } from './UIComponents';
import { Architecture, RoutingResults } from '../types';

interface InspectorPanelProps {
  arch: Architecture | null;
  results: RoutingResults | null;
  selectedCompId: number | null;
  selectedNetId: number | null;
  onSelectComponent: (id: number | null) => void;
  onSelectNet: (id: number | null) => void;
}

export function InspectorPanel({
  arch,
  results,
  selectedCompId,
  selectedNetId,
  onSelectComponent,
  onSelectNet
}: InspectorPanelProps) {
  const selectedComp = arch?.components.find((c) => c.id === selectedCompId);
  const selectedNet = arch?.nets.find((n) => n.id === selectedNetId);
  const routeRecord = results?.routes.find((r) => r.net_id === selectedNetId);

  // Connected nets for selected component
  const connectedNets = arch?.nets.filter(
    (n) => n.source_comp_id === selectedCompId || n.target_comp_id === selectedCompId
  ) || [];

  return (
    <Card className="flex flex-col h-full overflow-hidden border-slate-800">
      <CardHeader className="py-2.5 px-4">
        <CardTitle>
          <Info className="w-4 h-4 text-cyan-400" />
          Inspector & Diagnostics
        </CardTitle>
      </CardHeader>

      <CardContent className="flex-1 overflow-y-auto p-4 space-y-4 text-xs font-mono">
        {/* Case 1: Net selected */}
        {selectedNet ? (
          <div className="space-y-3">
            <div className="flex items-center justify-between pb-2 border-b border-slate-800">
              <div>
                <span className="text-[10px] text-slate-500 uppercase block">Selected Net</span>
                <span className="text-sm font-bold text-slate-100">{selectedNet.name}</span>
              </div>
              <Badge variant={selectedNet.type === 'DATA' ? 'cyan' : selectedNet.type === 'CONTROL' ? 'amber' : 'purple'}>
                {selectedNet.type} (Prio {selectedNet.priority})
              </Badge>
            </div>

            <div className="bg-slate-950 p-2.5 rounded border border-slate-800 space-y-1.5 text-[11px]">
              <div className="flex justify-between">
                <span className="text-slate-400">Endpoints:</span>
                <span className="text-slate-200">
                  {arch?.components.find((c) => c.id === selectedNet.source_comp_id)?.name} : {selectedNet.source_port}
                  {' -> '}
                  {arch?.components.find((c) => c.id === selectedNet.target_comp_id)?.name} : {selectedNet.target_port}
                </span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Demand Tracks:</span>
                <span className="text-cyan-400 font-bold">{selectedNet.track_demand}</span>
              </div>
            </div>

            {routeRecord ? (
              <div className="bg-slate-950 p-2.5 rounded border border-slate-800 space-y-1.5 text-[11px]">
                <div className="flex justify-between items-center">
                  <span className="text-slate-400">Routing Status:</span>
                  {routeRecord.routed ? (
                    <span className="text-emerald-400 font-bold flex items-center gap-1">
                      <CheckCircle2 className="w-3.5 h-3.5" /> Routed
                    </span>
                  ) : (
                    <span className="text-rose-400 font-bold flex items-center gap-1">
                      <XCircle className="w-3.5 h-3.5" /> Failed
                    </span>
                  )}
                </div>

                {routeRecord.routed ? (
                  <>
                    <div className="flex justify-between">
                      <span className="text-slate-400">Physical Wirelength:</span>
                      <span className="text-cyan-300 font-bold">{routeRecord.wirelength.toFixed(1)} u</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-slate-400">Direction Bends:</span>
                      <span className="text-slate-200">{routeRecord.bends}</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-slate-400">Peak Edge Congestion:</span>
                      <span className="text-amber-400">{(routeRecord.max_edge_congestion * 100).toFixed(0)}%</span>
                    </div>
                    {routeRecord.path && (
                      <div className="pt-2">
                        <span className="text-slate-400 block mb-1">Path Coordinates ({routeRecord.path.length} hops):</span>
                        <div className="max-h-28 overflow-y-auto bg-slate-900 p-2 rounded text-[10px] text-slate-300 font-mono space-y-0.5">
                          {routeRecord.path.map((pt, i) => (
                            <div key={i}>
                              #{i + 1}: [{pt.gx}, {pt.gy}] &rarr; ({pt.x.toFixed(1)}, {pt.y.toFixed(1)})
                            </div>
                          ))}
                        </div>
                      </div>
                    )}
                  </>
                ) : (
                  <div className="text-rose-300 text-[11px]">
                    Failure Reason: {routeRecord.fail_reason || 'Could not find capacity-feasible path'}
                  </div>
                )}
              </div>
            ) : (
              <p className="text-[11px] text-slate-500 italic">Route has not been calculated yet.</p>
            )}

            <button
              onClick={() => onSelectNet(null)}
              className="text-[11px] text-cyan-400 hover:underline"
            >
              &larr; Clear Selection
            </button>
          </div>
        ) : selectedComp ? (
          /* Case 2: Component selected */
          <div className="space-y-3">
            <div className="flex items-center justify-between pb-2 border-b border-slate-800">
              <div>
                <span className="text-[10px] text-slate-500 uppercase block">Selected Block</span>
                <span className="text-sm font-bold text-slate-100">{selectedComp.name}</span>
              </div>
              <Badge variant="cyan">{selectedComp.type}</Badge>
            </div>

            <div className="bg-slate-950 p-2.5 rounded border border-slate-800 space-y-1 text-[11px]">
              <div className="flex justify-between">
                <span className="text-slate-400">Position:</span>
                <span className="text-slate-200">({selectedComp.bounds.x.toFixed(1)}, {selectedComp.bounds.y.toFixed(1)})</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Dimensions:</span>
                <span className="text-slate-200">{selectedComp.bounds.width.toFixed(1)} x {selectedComp.bounds.height.toFixed(1)} u</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Estimated Area:</span>
                <span className="text-cyan-300 font-bold">{selectedComp.area_estimate} mm²</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Estimated Power:</span>
                <span className="text-emerald-400 font-bold">{selectedComp.power_estimate} W</span>
              </div>
            </div>

            {/* Ports */}
            <div>
              <span className="text-slate-400 text-[10px] uppercase font-semibold block mb-1">
                Ports ({selectedComp.ports.length})
              </span>
              <div className="space-y-1">
                {selectedComp.ports.map((p, idx) => (
                  <div key={idx} className="bg-slate-950 px-2 py-1 rounded border border-slate-800 flex justify-between text-[10px]">
                    <span className="text-cyan-300">{p.name}</span>
                    <span className="text-slate-500 font-mono">abs: ({p.abs_x.toFixed(1)}, {p.abs_y.toFixed(1)})</span>
                  </div>
                ))}
              </div>
            </div>

            {/* Connected Nets */}
            <div>
              <span className="text-slate-400 text-[10px] uppercase font-semibold block mb-1">
                Connected Nets ({connectedNets.length})
              </span>
              <div className="max-h-36 overflow-y-auto space-y-1">
                {connectedNets.map((n) => (
                  <div
                    key={n.id}
                    onClick={() => onSelectNet(n.id)}
                    className="bg-slate-950 hover:bg-slate-900 p-1.5 rounded border border-slate-800 cursor-pointer flex items-center justify-between text-[10px]"
                  >
                    <span className="text-slate-200 font-semibold">{n.name}</span>
                    <Badge variant={n.type === 'DATA' ? 'cyan' : 'amber'}>{n.type}</Badge>
                  </div>
                ))}
              </div>
            </div>

            <button
              onClick={() => onSelectComponent(null)}
              className="text-[11px] text-cyan-400 hover:underline"
            >
              &larr; Clear Selection
            </button>
          </div>
        ) : (
          /* Case 3: Global Overview */
          <div className="space-y-3 text-[11px]">
            <span className="text-slate-400 text-[10px] uppercase font-semibold block pb-1 border-b border-slate-800">
              Processor Floorplan Overview
            </span>

            {arch ? (
              <div className="space-y-2">
                <div className="bg-slate-950 p-2.5 rounded border border-slate-800 space-y-1">
                  <div className="flex justify-between">
                    <span className="text-slate-400">Total Component Blocks:</span>
                    <span className="text-slate-100 font-bold">{arch.components.length}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Total Logical Nets:</span>
                    <span className="text-slate-100 font-bold">{arch.nets.length}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Die Grid Dimensions:</span>
                    <span className="text-cyan-300 font-bold">{arch.config.die_width} x {arch.config.die_height}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Interconnect Topology:</span>
                    <span className="text-purple-400 font-bold">{arch.config.topology}</span>
                  </div>
                </div>

                <div className="bg-slate-950 p-2.5 rounded border border-slate-800 text-[10px] text-slate-400 leading-relaxed">
                  <p className="font-semibold text-slate-300 mb-1">Interactive CAD Instructions:</p>
                  <ul className="list-disc pl-4 space-y-1">
                    <li>Click any block or wire to inspect detailed metrics.</li>
                    <li>Click and drag blocks to reposition them with real-time C placement validation.</li>
                    <li>Add Keepouts to test obstacle avoidance rerouting.</li>
                  </ul>
                </div>
              </div>
            ) : (
              <p className="text-slate-500 italic">No architecture loaded.</p>
            )}
          </div>
        )}
      </CardContent>
    </Card>
  );
}
