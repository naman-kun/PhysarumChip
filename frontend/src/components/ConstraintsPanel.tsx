import React from 'react';
import { Sliders, Shield, Zap, AlertCircle, Plus, Trash2 } from 'lucide-react';
import { Card, CardHeader, CardTitle, CardContent, Slider, Button } from './UIComponents';
import { RoutingParams, BlockedRegion } from '../types';

interface ConstraintsPanelProps {
  params: RoutingParams;
  onChange: (newParams: RoutingParams) => void;
  onRemoveBlockedRegion: (index: number) => void;
}

export function ConstraintsPanel({
  params,
  onChange,
  onRemoveBlockedRegion
}: ConstraintsPanelProps) {
  const updatePhysarum = (field: string, val: any) => {
    onChange({
      ...params,
      physarum_params: {
        ...params.physarum_params,
        [field]: val
      }
    });
  };

  const updateDijkstra = (field: string, val: any) => {
    onChange({
      ...params,
      dijkstra_constraints: {
        ...params.dijkstra_constraints,
        [field]: val
      }
    });
  };

  return (
    <Card className="flex flex-col h-full overflow-hidden border-slate-800">
      <CardHeader className="py-2.5 px-4">
        <CardTitle>
          <Sliders className="w-4 h-4 text-cyan-400" />
          Constraints & Model Hyperparameters
        </CardTitle>
      </CardHeader>

      <CardContent className="flex-1 overflow-y-auto p-4 space-y-4">
        {/* Hard Constraints Box */}
        <div className="bg-rose-950/20 border border-rose-800/40 rounded-lg p-3 space-y-2">
          <div className="flex items-center gap-1.5 text-rose-400 text-xs font-semibold uppercase tracking-wider">
            <Shield className="w-3.5 h-3.5" />
            Hard Design Constraints
          </div>

          <label className="flex items-start gap-2.5 text-xs text-slate-300 cursor-pointer pt-1">
            <input
              type="checkbox"
              checked={params.physarum_params.enforce_hard_capacity}
              onChange={(e) => {
                updatePhysarum('enforce_hard_capacity', e.target.checked);
                updateDijkstra('enforce_hard_capacity', e.target.checked);
              }}
              className="accent-rose-500 mt-0.5"
            />
            <div>
              <span className="font-semibold text-rose-200">Enforce Hard Track Capacity</span>
              <p className="text-[11px] text-slate-400">
                If checked, any route that causes edge traffic to exceed maximum available tracks is strictly rejected as unroutable.
              </p>
            </div>
          </label>
        </div>

        {/* Global Track Capacity & Routing Costs */}
        <div className="bg-slate-950/60 p-3 rounded-lg border border-slate-800/80 space-y-3">
          <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block">
            Grid & Track Capacity
          </span>

          <Slider
            label="Default Track Capacity Per Grid Edge"
            value={params.default_edge_capacity}
            min={1}
            max={8}
            step={1}
            unit="tracks"
            onChange={(v) => onChange({ ...params, default_edge_capacity: v })}
          />

          <Slider
            label="Wire Bend Penalty"
            value={params.physarum_params.bend_penalty}
            min={0.0}
            max={2.0}
            step={0.1}
            unit="cost"
            onChange={(v) => {
              updatePhysarum('bend_penalty', v);
              updateDijkstra('bend_penalty', v);
            }}
          />

          <Slider
            label="Congestion Resistance Multiplier (β)"
            value={params.physarum_params.congestion_weight}
            min={0.5}
            max={10.0}
            step={0.5}
            unit="x"
            onChange={(v) => {
              updatePhysarum('congestion_weight', v);
              updateDijkstra('congestion_weight', v);
            }}
          />
        </div>

        {/* Physarum Model Parameters (Tero et al. Flow Model) */}
        <div className="bg-slate-950/60 p-3 rounded-lg border border-slate-800/80 space-y-3">
          <div className="flex items-center justify-between">
            <span className="text-[11px] font-semibold text-cyan-400 uppercase tracking-wider block">
              Physarum Adaptive Flow Model
            </span>
            <span className="text-[10px] text-slate-500 font-mono">Tero et al. Equations</span>
          </div>

          <Slider
            label="Conductivity Decay Factor (ρ)"
            value={params.physarum_params.rho}
            min={0.05}
            max={0.5}
            step={0.05}
            onChange={(v) => updatePhysarum('rho', v)}
          />

          <Slider
            label="Reinforcement Rate (α)"
            value={params.physarum_params.alpha}
            min={0.2}
            max={3.0}
            step={0.1}
            onChange={(v) => updatePhysarum('alpha', v)}
          />

          <Slider
            label="Adaptation Cycles Per Net"
            value={params.physarum_params.adaptation_steps}
            min={5}
            max={30}
            step={5}
            unit="cycles"
            onChange={(v) => updatePhysarum('adaptation_steps', v)}
          />

          <Slider
            label="Max Pressure Solver Iterations"
            value={params.physarum_params.max_pressure_iters}
            min={20}
            max={150}
            step={10}
            unit="iters"
            onChange={(v) => updatePhysarum('max_pressure_iters', v)}
          />
        </div>

        {/* Active Blocked Keepout Regions */}
        <div className="bg-slate-950/60 p-3 rounded-lg border border-slate-800/80 space-y-2">
          <div className="flex items-center justify-between">
            <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block">
              Keepouts & Obstacles ({params.blocked_regions.length})
            </span>
          </div>

          {params.blocked_regions.length === 0 ? (
            <p className="text-[11px] text-slate-500 italic">No custom keepouts. Use '+ Keepout' on the canvas toolbar to add obstacles.</p>
          ) : (
            <div className="space-y-1.5 font-mono text-[11px]">
              {params.blocked_regions.map((b, i) => (
                <div key={i} className="flex items-center justify-between bg-slate-900 px-2.5 py-1.5 rounded border border-slate-800">
                  <span className="text-rose-400">
                    [{b.x}, {b.y}] ({b.width}x{b.height})
                  </span>
                  <button
                    onClick={() => onRemoveBlockedRegion(i)}
                    className="text-slate-500 hover:text-rose-400"
                  >
                    <Trash2 className="w-3.5 h-3.5" />
                  </button>
                </div>
              ))}
            </div>
          )}
        </div>
      </CardContent>
    </Card>
  );
}
