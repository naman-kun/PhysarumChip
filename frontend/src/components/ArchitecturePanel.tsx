import React, { useState } from 'react';
import { Cpu, RefreshCw, Layers, ShieldCheck, Zap, Box, Compass } from 'lucide-react';
import { Card, CardHeader, CardTitle, CardContent, CardFooter, Button, Slider, Badge } from './UIComponents';
import { ArchConfig, Architecture, InterconnectTopology } from '../types';

interface ArchitecturePanelProps {
  arch: Architecture | null;
  onGenerate: (config: ArchConfig) => Promise<void>;
  isGenerating: boolean;
}

export function ArchitecturePanel({ arch, onGenerate, isGenerating }: ArchitecturePanelProps) {
  const [cfg, setCfg] = useState<ArchConfig>(
    arch?.config || {
      num_p_cores: 8,
      num_e_cores: 8,
      e_cluster_size: 4,
      num_llc_slices: 4,
      num_mem_ctrls: 2,
      has_system_agent: true,
      has_pcie_io: true,
      topology: 'RING',
      die_width: 100.0,
      die_height: 80.0,
      routing_pitch: 1.0
    }
  );

  const handleApply = async () => {
    await onGenerate(cfg);
  };

  return (
    <Card className="flex flex-col h-full overflow-hidden border-slate-800">
      <CardHeader className="py-3 px-4">
        <div className="flex items-center justify-between">
          <CardTitle>
            <Cpu className="w-4 h-4 text-cyan-400" />
            CPU Architecture Config
          </CardTitle>
          <Badge variant="cyan">
            {cfg.num_p_cores}P + {cfg.num_e_cores}E
          </Badge>
        </div>
      </CardHeader>

      <CardContent className="flex-1 overflow-y-auto p-4 space-y-4">
        {/* Core & Cluster Controls */}
        <div className="space-y-3 bg-slate-950/60 p-3 rounded-lg border border-slate-800/80">
          <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block">
            Compute Cores
          </span>

          <Slider
            label="Performance Cores (P-Cores)"
            value={cfg.num_p_cores}
            min={1}
            max={16}
            step={1}
            unit="cores"
            onChange={(v) => setCfg({ ...cfg, num_p_cores: v })}
          />

          <Slider
            label="Efficiency Cores (E-Cores)"
            value={cfg.num_e_cores}
            min={0}
            max={32}
            step={2}
            unit="cores"
            onChange={(v) => setCfg({ ...cfg, num_e_cores: v })}
          />

          {cfg.num_e_cores > 0 && (
            <Slider
              label="E-Cluster Size (Shared L2)"
              value={cfg.e_cluster_size}
              min={2}
              max={8}
              step={2}
              unit="per L2"
              onChange={(v) => setCfg({ ...cfg, e_cluster_size: v })}
            />
          )}
        </div>

        {/* Cache & Memory Subsystem */}
        <div className="space-y-3 bg-slate-950/60 p-3 rounded-lg border border-slate-800/80">
          <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block">
            Memory & Caches
          </span>

          <Slider
            label="Last-Level Cache (LLC) Slices"
            value={cfg.num_llc_slices}
            min={1}
            max={8}
            step={1}
            unit="slices"
            onChange={(v) => setCfg({ ...cfg, num_llc_slices: v })}
          />

          <Slider
            label="Memory Controllers"
            value={cfg.num_mem_ctrls}
            min={1}
            max={4}
            step={1}
            unit="channels"
            onChange={(v) => setCfg({ ...cfg, num_mem_ctrls: v })}
          />
        </div>

        {/* Interconnect & IO */}
        <div className="space-y-3 bg-slate-950/60 p-3 rounded-lg border border-slate-800/80">
          <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block">
            Interconnect & I/O
          </span>

          <div className="flex flex-col gap-1 text-xs">
            <label className="text-slate-300 font-medium">Logical Topology</label>
            <select
              value={cfg.topology}
              onChange={(e) => setCfg({ ...cfg, topology: e.target.value as InterconnectTopology })}
              className="w-full font-mono text-cyan-300 font-semibold"
            >
              <option value="RING">Ring Interconnect (Bidirectional Loop)</option>
              <option value="MESH_2D">2D Mesh (Grid Router Stops)</option>
              <option value="CROSSBAR">Crossbar Matrix (Star Backplane)</option>
            </select>
          </div>

          <div className="grid grid-cols-2 gap-2 pt-1">
            <label className="flex items-center gap-2 text-xs text-slate-300 cursor-pointer bg-slate-900 px-2.5 py-1.5 rounded border border-slate-800">
              <input
                type="checkbox"
                checked={cfg.has_system_agent}
                onChange={(e) => setCfg({ ...cfg, has_system_agent: e.target.checked })}
                className="accent-cyan-400"
              />
              System Agent
            </label>
            <label className="flex items-center gap-2 text-xs text-slate-300 cursor-pointer bg-slate-900 px-2.5 py-1.5 rounded border border-slate-800">
              <input
                type="checkbox"
                checked={cfg.has_pcie_io}
                onChange={(e) => setCfg({ ...cfg, has_pcie_io: e.target.checked })}
                className="accent-cyan-400"
              />
              PCIe / IO Hub
            </label>
          </div>
        </div>

        {/* Die Dimensions & Pitch */}
        <div className="space-y-3 bg-slate-950/60 p-3 rounded-lg border border-slate-800/80">
          <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block">
            Physical Die Size
          </span>

          <div className="grid grid-cols-2 gap-2 text-xs">
            <div>
              <label className="text-slate-400 text-[10px] block mb-1">Die Width</label>
              <input
                type="number"
                min={40}
                max={200}
                value={cfg.die_width}
                onChange={(e) => setCfg({ ...cfg, die_width: parseFloat(e.target.value) || 100 })}
                className="w-full font-mono text-xs"
              />
            </div>
            <div>
              <label className="text-slate-400 text-[10px] block mb-1">Die Height</label>
              <input
                type="number"
                min={40}
                max={200}
                value={cfg.die_height}
                onChange={(e) => setCfg({ ...cfg, die_height: parseFloat(e.target.value) || 80 })}
                className="w-full font-mono text-xs"
              />
            </div>
          </div>
        </div>

        {/* Dynamic Architectural Budget Card */}
        {arch && (
          <div className="bg-gradient-to-br from-slate-900 to-slate-950 p-3 rounded-lg border border-cyan-500/20 text-xs space-y-1.5">
            <div className="flex justify-between items-center text-slate-400">
              <span>Estimated Silicon Area:</span>
              <span className="font-mono text-cyan-300 font-semibold">{arch.total_estimated_area.toFixed(0)} mm²</span>
            </div>
            <div className="flex justify-between items-center text-slate-400">
              <span>Estimated Dynamic Power:</span>
              <span className="font-mono text-emerald-400 font-semibold">{arch.total_estimated_power.toFixed(1)} W</span>
            </div>
            <div className="flex justify-between items-center text-slate-400">
              <span>Die Utilization:</span>
              <span className="font-mono text-slate-200">
                {((arch.total_estimated_area / (cfg.die_width * cfg.die_height)) * 100).toFixed(1)}%
              </span>
            </div>
          </div>
        )}
      </CardContent>

      <CardFooter className="py-2.5 px-4 bg-slate-950/80">
        <Button
          variant="primary"
          size="md"
          className="w-full"
          disabled={isGenerating}
          onClick={handleApply}
          icon={<RefreshCw className={`w-3.5 h-3.5 ${isGenerating ? 'animate-spin' : ''}`} />}
        >
          {isGenerating ? 'Generating in C11...' : 'Regenerate Architecture & Netlist'}
        </Button>
      </CardFooter>
    </Card>
  );
}
