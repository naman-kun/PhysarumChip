import React from 'react';
import { GitCompare, CheckCircle2, AlertTriangle, TrendingDown, TrendingUp, Clock, Zap, Cpu } from 'lucide-react';
import { Card, CardHeader, CardTitle, CardContent, Badge } from './UIComponents';
import { ComparisonReport, RoutingResults } from '../types';

interface ComparisonPanelProps {
  report: ComparisonReport | null;
  physarumOnly: RoutingResults | null;
  dijkstraOnly: RoutingResults | null;
}

export function ComparisonPanel({ report, physarumOnly, dijkstraOnly }: ComparisonPanelProps) {
  const pRes = report?.physarum || physarumOnly;
  const dRes = report?.dijkstra || dijkstraOnly;
  const diffs = report?.differences;

  if (!pRes && !dRes) {
    return (
      <Card className="flex flex-col h-full border-slate-800">
        <CardHeader className="py-2.5 px-4">
          <CardTitle>
            <GitCompare className="w-4 h-4 text-cyan-400" />
            Algorithm Benchmark & Comparison
          </CardTitle>
        </CardHeader>
        <CardContent className="flex-1 flex flex-col items-center justify-center text-center p-6 text-slate-500">
          <GitCompare className="w-10 h-10 mb-2 opacity-30" />
          <p className="text-xs">No routing results yet.</p>
          <p className="text-[11px] text-slate-600 mt-1 max-w-xs">
            Click "Run Physarum", "Run Dijkstra", or "Compare Both" above to execute C11 routing and inspect measured trade-offs.
          </p>
        </CardContent>
      </Card>
    );
  }

  return (
    <Card className="flex flex-col h-full overflow-hidden border-slate-800">
      <CardHeader className="py-2.5 px-4">
        <div className="flex items-center justify-between">
          <CardTitle>
            <GitCompare className="w-4 h-4 text-cyan-400" />
            Routing Comparison & Metrics
          </CardTitle>
          {diffs && (
            <Badge variant="emerald">
              Measured Comparison Ready
            </Badge>
          )}
        </div>
      </CardHeader>

      <CardContent className="flex-1 overflow-y-auto p-4 space-y-4">
        {/* Trade-off summary alert */}
        {diffs && (
          <div className="bg-slate-950 p-3 rounded-lg border border-cyan-500/30 text-xs space-y-2">
            <span className="font-semibold text-slate-200 block text-[11px] uppercase tracking-wider">
              Measured Trade-Off Summary
            </span>
            <div className="grid grid-cols-2 gap-2 font-mono text-[11px]">
              <div className="bg-slate-900/80 p-2 rounded border border-slate-800">
                <span className="text-slate-400 block text-[10px]">Wirelength Delta</span>
                <span className={`font-bold ${diffs.wirelength_diff_pct <= 0 ? 'text-emerald-400' : 'text-amber-400'}`}>
                  {diffs.wirelength_diff_pct > 0 ? `+${diffs.wirelength_diff_pct.toFixed(1)}%` : `${diffs.wirelength_diff_pct.toFixed(1)}%`}
                </span>
                <span className="text-[9px] text-slate-500 block mt-0.5">vs Dijkstra shortest path</span>
              </div>

              <div className="bg-slate-900/80 p-2 rounded border border-slate-800">
                <span className="text-slate-400 block text-[10px]">Peak Congestion Delta</span>
                <span className={`font-bold ${diffs.peak_congestion_diff <= 0 ? 'text-emerald-400' : 'text-rose-400'}`}>
                  {diffs.peak_congestion_diff > 0 ? `+${diffs.peak_congestion_diff.toFixed(2)}` : diffs.peak_congestion_diff.toFixed(2)}
                </span>
                <span className="text-[9px] text-slate-500 block mt-0.5">demand / capacity</span>
              </div>
            </div>
          </div>
        )}

        {/* Detailed Comparison Table */}
        <div className="border border-slate-800 rounded-lg overflow-hidden bg-slate-950/60">
          <table className="w-full text-xs font-mono border-collapse">
            <thead>
              <tr className="bg-slate-900/90 text-slate-400 border-b border-slate-800 text-[10px]">
                <th className="py-2 px-3 text-left">Metric</th>
                <th className="py-2 px-3 text-right text-cyan-400">Physarum Flow</th>
                <th className="py-2 px-3 text-right text-purple-400">Dijkstra Baseline</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/40 text-[11px]">
              <tr>
                <td className="py-2 px-3 text-slate-300">Success Rate</td>
                <td className="py-2 px-3 text-right text-emerald-400 font-bold">
                  {pRes ? `${pRes.summary.success_rate.toFixed(1)}%` : '—'}
                </td>
                <td className="py-2 px-3 text-right text-emerald-400 font-bold">
                  {dRes ? `${dRes.summary.success_rate.toFixed(1)}%` : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Routed Nets</td>
                <td className="py-2 px-3 text-right text-slate-100">
                  {pRes ? `${pRes.summary.routed_nets} / ${pRes.summary.total_nets}` : '—'}
                </td>
                <td className="py-2 px-3 text-right text-slate-100">
                  {dRes ? `${dRes.summary.routed_nets} / ${dRes.summary.total_nets}` : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Total Wirelength</td>
                <td className="py-2 px-3 text-right text-cyan-300 font-semibold">
                  {pRes ? `${pRes.summary.total_wirelength.toFixed(1)} u` : '—'}
                </td>
                <td className="py-2 px-3 text-right text-purple-300 font-semibold">
                  {dRes ? `${dRes.summary.total_wirelength.toFixed(1)} u` : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Average Wirelength</td>
                <td className="py-2 px-3 text-right text-slate-300">
                  {pRes ? `${pRes.summary.avg_wirelength.toFixed(1)} u` : '—'}
                </td>
                <td className="py-2 px-3 text-right text-slate-300">
                  {dRes ? `${dRes.summary.avg_wirelength.toFixed(1)} u` : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Peak Congestion</td>
                <td className="py-2 px-3 text-right text-amber-300">
                  {pRes ? `${(pRes.summary.peak_congestion * 100).toFixed(0)}%` : '—'}
                </td>
                <td className="py-2 px-3 text-right text-amber-300">
                  {dRes ? `${(dRes.summary.peak_congestion * 100).toFixed(0)}%` : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Average Congestion</td>
                <td className="py-2 px-3 text-right text-slate-300">
                  {pRes ? `${(pRes.summary.avg_congestion * 100).toFixed(1)}%` : '—'}
                </td>
                <td className="py-2 px-3 text-right text-slate-300">
                  {dRes ? `${(dRes.summary.avg_congestion * 100).toFixed(1)}%` : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Capacity Violations</td>
                <td className="py-2 px-3 text-right">
                  {pRes ? (
                    pRes.summary.capacity_violations === 0 ? (
                      <span className="text-emerald-400">0 (Clean)</span>
                    ) : (
                      <span className="text-rose-400 font-bold">{pRes.summary.capacity_violations}</span>
                    )
                  ) : '—'}
                </td>
                <td className="py-2 px-3 text-right">
                  {dRes ? (
                    dRes.summary.capacity_violations === 0 ? (
                      <span className="text-emerald-400">0 (Clean)</span>
                    ) : (
                      <span className="text-rose-400 font-bold">{dRes.summary.capacity_violations}</span>
                    )
                  ) : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Total Direction Bends</td>
                <td className="py-2 px-3 text-right text-slate-300">
                  {pRes ? pRes.summary.total_bends : '—'}
                </td>
                <td className="py-2 px-3 text-right text-slate-300">
                  {dRes ? dRes.summary.total_bends : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Abstract Cost Score</td>
                <td className="py-2 px-3 text-right text-slate-100 font-semibold">
                  {pRes ? pRes.summary.abstract_total_cost.toFixed(1) : '—'}
                </td>
                <td className="py-2 px-3 text-right text-slate-100 font-semibold">
                  {dRes ? dRes.summary.abstract_total_cost.toFixed(1) : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">C Engine Execution Time</td>
                <td className="py-2 px-3 text-right text-cyan-400 font-bold">
                  {pRes ? `${pRes.summary.runtime_ms.toFixed(1)} ms` : '—'}
                </td>
                <td className="py-2 px-3 text-right text-purple-400 font-bold">
                  {dRes ? `${dRes.summary.runtime_ms.toFixed(1)} ms` : '—'}
                </td>
              </tr>
              <tr>
                <td className="py-2 px-3 text-slate-300">Total Solver Iterations</td>
                <td className="py-2 px-3 text-right text-slate-400">
                  {pRes ? `${pRes.summary.total_iterations} GS iters` : '—'}
                </td>
                <td className="py-2 px-3 text-right text-slate-400">
                  {dRes ? `${dRes.summary.total_iterations} passes` : '—'}
                </td>
              </tr>
            </tbody>
          </table>
        </div>

        {/* Academic Notes */}
        <div className="bg-slate-900/50 p-2.5 rounded-lg border border-slate-800 text-[11px] text-slate-400 leading-relaxed">
          <span className="font-semibold text-slate-300 block mb-0.5">Algorithm Note:</span>
          Dijkstra optimizes purely for static non-negative edge length/bends, whereas Physarum dynamically reinforces high-traffic tubes and dampens congested channels through fluid flux conservation.
        </div>
      </CardContent>
    </Card>
  );
}
