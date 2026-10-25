import React, { useState, useEffect } from 'react';
import { Header } from './components/Header';
import { ChipCanvas } from './components/ChipCanvas';
import { ArchitecturePanel } from './components/ArchitecturePanel';
import { NetlistPanel } from './components/NetlistPanel';
import { ConstraintsPanel } from './components/ConstraintsPanel';
import { ComparisonPanel } from './components/ComparisonPanel';
import { InspectorPanel } from './components/InspectorPanel';
import { ExportModal } from './components/ExportModal';
import {
  generateArchitecture,
  validatePlacement,
  routePhysarum,
  routeDijkstra,
  compareRoutes,
  checkHealth
} from './api';
import {
  Architecture,
  ArchConfig,
  RoutingParams,
  RoutingResults,
  ComparisonReport,
  LogicalNet,
  BlockedRegion
} from './types';
import { Cpu, Network, Sliders, GitCompare, Info } from 'lucide-react';

export default function App() {
  const [arch, setArch] = useState<Architecture | null>(null);
  const [results, setResults] = useState<RoutingResults | null>(null);
  const [comparison, setComparison] = useState<ComparisonReport | null>(null);

  // Routing and model parameters
  const [params, setParams] = useState<RoutingParams>({
    default_edge_capacity: 4,
    dijkstra_constraints: {
      bend_penalty: 0.3,
      congestion_weight: 2.5,
      enforce_hard_capacity: false
    },
    physarum_params: {
      rho: 0.15,
      alpha: 1.2,
      d_min: 0.05,
      d_max: 50.0,
      source_current: 1.0,
      max_pressure_iters: 80,
      adaptation_steps: 15,
      congestion_weight: 3.0,
      bend_penalty: 0.3,
      enforce_hard_capacity: false
    },
    blocked_regions: []
  });

  // UI Selection State
  const [selectedCompId, setSelectedCompId] = useState<number | null>(null);
  const [selectedNetId, setSelectedNetId] = useState<number | null>(null);
  const [activeTab, setActiveTab] = useState<'architecture' | 'netlist' | 'constraints' | 'comparison'>('architecture');

  // Async Execution State
  const [isRunning, setIsRunning] = useState(false);
  const [activeAlgo, setActiveAlgo] = useState<string | null>(null);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [isExportOpen, setIsExportOpen] = useState(false);

  // Load default architecture at startup
  useEffect(() => {
    async function init() {
      try {
        const defaultArch = await generateArchitecture();
        setArch(defaultArch);
      } catch (err: any) {
        console.error('Failed to load default architecture:', err);
        setErrorMessage(`Initial startup error: ${err.message}. Check that Express bridge is running.`);
      }
    }
    init();
  }, []);

  // Handler: Regenerate architecture from config
  const handleGenerateArch = async (config: ArchConfig) => {
    setIsRunning(true);
    setErrorMessage(null);
    try {
      const newArch = await generateArchitecture(config);
      setArch(newArch);
      setResults(null);
      setComparison(null);
      setSelectedCompId(null);
      setSelectedNetId(null);
    } catch (err: any) {
      setErrorMessage(`Generation failed: ${err.message}`);
    } finally {
      setIsRunning(false);
    }
  };

  // Handler: Component block dragged and repositioned
  const handleComponentMoved = async (compId: number, newX: number, newY: number): Promise<boolean> => {
    if (!arch) return false;

    // Create speculative architecture with new position
    const newArch: Architecture = {
      ...arch,
      components: arch.components.map((c) => {
        if (c.id === compId) {
          return {
            ...c,
            bounds: {
              ...c.bounds,
              x: newX,
              y: newY
            }
          };
        }
        return c;
      })
    };

    // Validate with C engine
    const val = await validatePlacement(newArch, 1.0);
    if (!val.success) {
      setErrorMessage(val.error || 'Invalid placement: overlaps another block or exceeds die boundaries.');
      return false;
    }

    setErrorMessage(null);
    setArch(newArch);
    // Invalidate existing routes since physical block locations changed
    if (results || comparison) {
      setResults(null);
      setComparison(null);
    }
    return true;
  };

  // Handler: Add custom Keepout / Blocked region
  const handleAddBlockedRegion = (region: BlockedRegion) => {
    setParams((prev) => ({
      ...prev,
      blocked_regions: [...prev.blocked_regions, region]
    }));
    // Invalidate routes
    setResults(null);
    setComparison(null);
  };

  const handleRemoveBlockedRegion = (index: number) => {
    setParams((prev) => ({
      ...prev,
      blocked_regions: prev.blocked_regions.filter((_, i) => i !== index)
    }));
    setResults(null);
    setComparison(null);
  };

  // Handler: Netlist mutations
  const handleAddNet = (newNetData: Omit<LogicalNet, 'id'>) => {
    if (!arch) return;
    const nextId = (arch.nets.reduce((max, n) => Math.max(max, n.id), 0) || 0) + 1;
    const newNet: LogicalNet = {
      ...newNetData,
      id: nextId
    };
    setArch({
      ...arch,
      nets: [...arch.nets, newNet]
    });
    setResults(null);
    setComparison(null);
  };

  const handleDeleteNet = (netId: number) => {
    if (!arch) return;
    setArch({
      ...arch,
      nets: arch.nets.filter((n) => n.id !== netId)
    });
    if (selectedNetId === netId) setSelectedNetId(null);
    setResults(null);
    setComparison(null);
  };

  // Handler: Run Physarum
  const handleRunPhysarum = async () => {
    if (!arch) return;
    setIsRunning(true);
    setActiveAlgo('physarum');
    setErrorMessage(null);
    try {
      const res = await routePhysarum({
        architecture: arch,
        routing_params: params
      });
      setResults(res);
      setActiveTab('comparison');
    } catch (err: any) {
      setErrorMessage(`Physarum routing error: ${err.message}`);
    } finally {
      setIsRunning(false);
      setActiveAlgo(null);
    }
  };

  // Handler: Run Dijkstra
  const handleRunDijkstra = async () => {
    if (!arch) return;
    setIsRunning(true);
    setActiveAlgo('dijkstra');
    setErrorMessage(null);
    try {
      const res = await routeDijkstra({
        architecture: arch,
        routing_params: params
      });
      setResults(res);
      setActiveTab('comparison');
    } catch (err: any) {
      setErrorMessage(`Dijkstra routing error: ${err.message}`);
    } finally {
      setIsRunning(false);
      setActiveAlgo(null);
    }
  };

  // Handler: Compare Both
  const handleCompare = async () => {
    if (!arch) return;
    setIsRunning(true);
    setActiveAlgo('compare');
    setErrorMessage(null);
    try {
      const comp = await compareRoutes({
        architecture: arch,
        routing_params: params
      });
      setComparison(comp);
      setResults(comp.physarum); // Show Physarum wire paths by default
      setActiveTab('comparison');
    } catch (err: any) {
      setErrorMessage(`Comparison evaluation error: ${err.message}`);
    } finally {
      setIsRunning(false);
      setActiveAlgo(null);
    }
  };

  // Handler: Reset routes
  const handleResetRoutes = () => {
    setResults(null);
    setComparison(null);
  };

  // Handler: Import design
  const handleImportDesign = (imported: any) => {
    if (imported.architecture) {
      setArch(imported.architecture);
      if (imported.routing_params) setParams(imported.routing_params);
    } else if (imported.config && imported.components) {
      setArch(imported);
    } else {
      setErrorMessage('Unrecognized design JSON schema');
    }
  };

  return (
    <div className="flex flex-col h-screen w-screen overflow-hidden bg-slate-950 font-sans selection:bg-cyan-500/30">
      {/* Top Header */}
      <Header
        arch={arch}
        isRunning={isRunning}
        activeAlgo={activeAlgo}
        onRunPhysarum={handleRunPhysarum}
        onRunDijkstra={handleRunDijkstra}
        onCompare={handleCompare}
        onResetRoutes={handleResetRoutes}
        onOpenExport={() => setIsExportOpen(true)}
        onImportClick={() => setIsExportOpen(true)}
        errorMessage={errorMessage}
        onDismissError={() => setErrorMessage(null)}
      />

      {/* Main CAD Workspace Layout */}
      <div className="flex-1 flex overflow-hidden p-2 gap-2">
        {/* Left Side: Modular Navigation Tabs & Control Panel */}
        <div className="w-[380px] shrink-0 flex flex-col h-full gap-2">
          {/* Panel Selector Tabs */}
          <div className="flex bg-slate-900/90 p-1 rounded-xl border border-slate-800 text-xs font-mono">
            <button
              onClick={() => setActiveTab('architecture')}
              className={`flex-1 py-1.5 px-2 rounded-lg flex items-center justify-center gap-1.5 transition-all ${
                activeTab === 'architecture'
                  ? 'bg-cyan-500/20 text-cyan-300 font-bold border border-cyan-500/40'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <Cpu className="w-3.5 h-3.5" />
              Arch
            </button>

            <button
              onClick={() => setActiveTab('netlist')}
              className={`flex-1 py-1.5 px-2 rounded-lg flex items-center justify-center gap-1.5 transition-all ${
                activeTab === 'netlist'
                  ? 'bg-cyan-500/20 text-cyan-300 font-bold border border-cyan-500/40'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <Network className="w-3.5 h-3.5" />
              Netlist
            </button>

            <button
              onClick={() => setActiveTab('constraints')}
              className={`flex-1 py-1.5 px-2 rounded-lg flex items-center justify-center gap-1.5 transition-all ${
                activeTab === 'constraints'
                  ? 'bg-cyan-500/20 text-cyan-300 font-bold border border-cyan-500/40'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <Sliders className="w-3.5 h-3.5" />
              Rules
            </button>

            <button
              onClick={() => setActiveTab('comparison')}
              className={`flex-1 py-1.5 px-2 rounded-lg flex items-center justify-center gap-1.5 transition-all ${
                activeTab === 'comparison'
                  ? 'bg-cyan-500/20 text-cyan-300 font-bold border border-cyan-500/40'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <GitCompare className="w-3.5 h-3.5" />
              Compare
            </button>
          </div>

          {/* Active Tab Panel */}
          <div className="flex-1 overflow-hidden">
            {activeTab === 'architecture' && (
              <ArchitecturePanel
                arch={arch}
                onGenerate={handleGenerateArch}
                isGenerating={isRunning}
              />
            )}

            {activeTab === 'netlist' && (
              <NetlistPanel
                arch={arch}
                results={results}
                selectedNetId={selectedNetId}
                onSelectNet={setSelectedNetId}
                onAddNet={handleAddNet}
                onDeleteNet={handleDeleteNet}
              />
            )}

            {activeTab === 'constraints' && (
              <ConstraintsPanel
                params={params}
                onChange={setParams}
                onRemoveBlockedRegion={handleRemoveBlockedRegion}
              />
            )}

            {activeTab === 'comparison' && (
              <ComparisonPanel
                report={comparison}
                physarumOnly={results?.algorithm === 'Physarum_Adaptive_Flow' ? results : null}
                dijkstraOnly={results?.algorithm === 'Dijkstra_Baseline' ? results : null}
              />
            )}
          </div>
        </div>

        {/* Center: CAD Floorplan & Routing Canvas */}
        <div className="flex-1 h-full min-w-0">
          {arch ? (
            <ChipCanvas
              arch={arch}
              results={results}
              selectedCompId={selectedCompId}
              selectedNetId={selectedNetId}
              blockedRegions={params.blocked_regions}
              onSelectComponent={setSelectedCompId}
              onSelectNet={setSelectedNetId}
              onComponentMoved={handleComponentMoved}
              onAddBlockedRegion={handleAddBlockedRegion}
            />
          ) : (
            <div className="w-full h-full flex items-center justify-center bg-slate-900/50 rounded-xl border border-slate-800 text-slate-500">
              Initializing C11 Engine Floorplan...
            </div>
          )}
        </div>

        {/* Right Side: Diagnostics & Property Inspector */}
        <div className="w-[320px] shrink-0 h-full">
          <InspectorPanel
            arch={arch}
            results={results}
            selectedCompId={selectedCompId}
            selectedNetId={selectedNetId}
            onSelectComponent={setSelectedCompId}
            onSelectNet={setSelectedNetId}
          />
        </div>
      </div>

      {/* Export / Load Dialog Modal */}
      <ExportModal
        isOpen={isExportOpen}
        onClose={() => setIsExportOpen(false)}
        arch={arch}
        params={params}
        results={results}
        comparison={comparison}
        onImportDesign={handleImportDesign}
      />
    </div>
  );
}
