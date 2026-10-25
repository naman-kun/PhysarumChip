import React from 'react';
import { Cpu, Play, Route, GitCompare, RotateCcw, Download, Upload, ShieldCheck, AlertTriangle } from 'lucide-react';
import { Button, Badge } from './UIComponents';
import { Architecture, InterconnectTopology } from '../types';

interface HeaderProps {
  arch: Architecture | null;
  isRunning: boolean;
  activeAlgo: string | null;
  onRunPhysarum: () => void;
  onRunDijkstra: () => void;
  onCompare: () => void;
  onResetRoutes: () => void;
  onOpenExport: () => void;
  onImportClick: () => void;
  errorMessage: string | null;
  onDismissError: () => void;
}

export function Header({
  arch,
  isRunning,
  activeAlgo,
  onRunPhysarum,
  onRunDijkstra,
  onCompare,
  onResetRoutes,
  onOpenExport,
  onImportClick,
  errorMessage,
  onDismissError
}: HeaderProps) {
  const topo = arch?.config.topology || 'RING';

  return (
    <header className="border-b border-slate-800 bg-slate-950/95 sticky top-0 z-40 backdrop-blur-md">
      <div className="max-w-[1920px] mx-auto px-4 py-2.5 flex items-center justify-between gap-4">
        {/* Left: Branding & Core Stats */}
        <div className="flex items-center gap-3">
          <div className="h-9 w-9 rounded-lg bg-gradient-to-br from-cyan-500/20 to-blue-600/30 border border-cyan-500/40 flex items-center justify-center text-cyan-400 shadow-inner">
            <Cpu className="w-5 h-5 animate-pulse" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h1 className="font-bold text-sm tracking-wide text-white uppercase flex items-center gap-1.5">
                Physarum<span className="text-cyan-400">Chip</span>
              </h1>
              <span className="text-[10px] px-1.5 py-0.2 bg-slate-800 border border-slate-700 text-slate-300 rounded font-mono">
                C11 Core
              </span>
              <Badge variant="cyan">
                {topo} INTERCONNECT
              </Badge>
            </div>
            <p className="text-[11px] text-slate-400">
              Adaptive Slime Mould Flow vs Dijkstra Routing on CPU Floorplan
            </p>
          </div>
        </div>

        {/* Center: Primary Routing Execution Controls */}
        <div className="flex items-center gap-2 bg-slate-900/90 p-1.5 rounded-xl border border-slate-800/80 shadow-inner">
          <Button
            variant="primary"
            size="sm"
            disabled={isRunning || !arch}
            onClick={onRunPhysarum}
            icon={<Play className="w-3.5 h-3.5 fill-current" />}
          >
            {isRunning && activeAlgo === 'physarum' ? 'Adapting Flow...' : 'Run Physarum'}
          </Button>

          <Button
            variant="secondary"
            size="sm"
            disabled={isRunning || !arch}
            onClick={onRunDijkstra}
            icon={<Route className="w-3.5 h-3.5 text-purple-400" />}
          >
            {isRunning && activeAlgo === 'dijkstra' ? 'Routing Dijkstra...' : 'Run Dijkstra'}
          </Button>

          <Button
            variant="success"
            size="sm"
            disabled={isRunning || !arch}
            onClick={onCompare}
            icon={<GitCompare className="w-3.5 h-3.5" />}
          >
            {isRunning && activeAlgo === 'compare' ? 'Evaluating Both...' : 'Compare Both'}
          </Button>

          <div className="h-4 w-px bg-slate-800 mx-1" />

          <Button
            variant="ghost"
            size="sm"
            disabled={isRunning}
            onClick={onResetRoutes}
            title="Clear routed wires"
            icon={<RotateCcw className="w-3.5 h-3.5" />}
          >
            Reset
          </Button>
        </div>

        {/* Right: Architecture Info & IO */}
        <div className="flex items-center gap-2">
          {arch && (
            <div className="hidden lg:flex items-center gap-2 text-[11px] text-slate-400 font-mono bg-slate-900/60 px-2.5 py-1 rounded-lg border border-slate-800">
              <span>{arch.components.length} Blocks</span>
              <span className="text-slate-600">•</span>
              <span>{arch.nets.length} Nets</span>
              <span className="text-slate-600">•</span>
              <span className="text-emerald-400 font-medium">{arch.total_estimated_power.toFixed(1)}W Est.</span>
            </div>
          )}

          <Button
            variant="outline"
            size="sm"
            onClick={onOpenExport}
            icon={<Download className="w-3.5 h-3.5" />}
          >
            Export
          </Button>

          <Button
            variant="outline"
            size="sm"
            onClick={onImportClick}
            icon={<Upload className="w-3.5 h-3.5" />}
          >
            Load
          </Button>
        </div>
      </div>

      {/* Error Alert Bar */}
      {errorMessage && (
        <div className="bg-rose-950/90 border-t border-rose-800/80 px-4 py-2 text-rose-200 text-xs flex items-center justify-between animate-fadeIn">
          <div className="flex items-center gap-2">
            <AlertTriangle className="w-4 h-4 text-rose-400 shrink-0" />
            <span className="font-mono">{errorMessage}</span>
          </div>
          <button
            onClick={onDismissError}
            className="text-rose-400 hover:text-rose-100 text-xs px-2 py-0.5 rounded bg-rose-900/40"
          >
            Dismiss
          </button>
        </div>
      )}
    </header>
  );
}
