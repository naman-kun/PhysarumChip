import React, { useState, useMemo } from 'react';
import { Network, Search, Filter, Plus, Trash2, CheckCircle2, XCircle, ArrowRight } from 'lucide-react';
import { Card, CardHeader, CardTitle, CardContent, Button, Badge } from './UIComponents';
import { Architecture, LogicalNet, NetRouteRecord, RoutingResults, NetType } from '../types';

interface NetlistPanelProps {
  arch: Architecture | null;
  results: RoutingResults | null;
  selectedNetId: number | null;
  onSelectNet: (id: number | null) => void;
  onAddNet: (newNet: Omit<LogicalNet, 'id'>) => void;
  onDeleteNet: (netId: number) => void;
}

export function NetlistPanel({
  arch,
  results,
  selectedNetId,
  onSelectNet,
  onAddNet,
  onDeleteNet
}: NetlistPanelProps) {
  const [searchTerm, setSearchTerm] = useState('');
  const [filterType, setFilterType] = useState<string>('ALL');
  const [filterStatus, setFilterStatus] = useState<string>('ALL');
  const [showAddModal, setShowAddModal] = useState(false);

  // New net form state
  const [newNetName, setNewNetName] = useState('');
  const [newSrcComp, setNewSrcComp] = useState<number>(1);
  const [newSrcPort, setNewSrcPort] = useState<string>('');
  const [newTgtComp, setNewTgtComp] = useState<number>(2);
  const [newTgtPort, setNewTgtPort] = useState<string>('');
  const [newType, setNewType] = useState<NetType>('DATA');
  const [newPriority, setNewPriority] = useState<number>(3);
  const [newDemand, setNewDemand] = useState<number>(1);

  // Map route results by net ID
  const routeMap = useMemo(() => {
    const map = new Map<number, NetRouteRecord>();
    if (results?.routes) {
      for (const r of results.routes) {
        map.set(r.net_id, r);
      }
    }
    return map;
  }, [results]);

  // Filtered nets
  const filteredNets = useMemo(() => {
    if (!arch) return [];
    return arch.nets.filter((net) => {
      // Search
      const matchesSearch =
        searchTerm === '' ||
        net.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
        net.id.toString() === searchTerm;

      // Type filter
      const matchesType = filterType === 'ALL' || net.type === filterType;

      // Status filter
      let matchesStatus = true;
      if (filterStatus !== 'ALL') {
        const record = routeMap.get(net.id);
        if (filterStatus === 'ROUTED') matchesStatus = !!record?.routed;
        else if (filterStatus === 'UNROUTED') matchesStatus = !record?.routed;
      }

      return matchesSearch && matchesType && matchesStatus;
    });
  }, [arch, searchTerm, filterType, filterStatus, routeMap]);

  const handleAddSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!newNetName.trim()) return;

    onAddNet({
      name: newNetName.trim(),
      source_comp_id: newSrcComp,
      source_port: newSrcPort || 'PORT',
      target_comp_id: newTgtComp,
      target_port: newTgtPort || 'PORT',
      type: newType,
      priority: newPriority,
      track_demand: newDemand,
      max_length: 0
    });

    setNewNetName('');
    setShowAddModal(false);
  };

  const getCompName = (id: number) => {
    const comp = arch?.components.find((c) => c.id === id);
    return comp ? comp.name : `Comp #${id}`;
  };

  return (
    <Card className="flex flex-col h-full overflow-hidden border-slate-800">
      <CardHeader className="py-2.5 px-4">
        <div className="flex items-center justify-between">
          <CardTitle>
            <Network className="w-4 h-4 text-cyan-400" />
            Logical Netlist ({arch?.nets.length || 0})
          </CardTitle>
          <Button
            variant="outline"
            size="sm"
            onClick={() => setShowAddModal(true)}
            icon={<Plus className="w-3.5 h-3.5" />}
          >
            Add Net
          </Button>
        </div>

        {/* Filter Toolbar */}
        <div className="flex items-center gap-2 pt-2">
          <div className="relative flex-1">
            <Search className="w-3.5 h-3.5 absolute left-2.5 top-2.5 text-slate-500" />
            <input
              type="text"
              placeholder="Search nets..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              className="w-full pl-8 pr-2 py-1 text-xs bg-slate-950 border border-slate-800 rounded font-mono"
            />
          </div>

          <select
            value={filterType}
            onChange={(e) => setFilterType(e.target.value)}
            className="text-xs bg-slate-950 border border-slate-800 py-1 px-2 rounded font-mono text-slate-300"
          >
            <option value="ALL">All Types</option>
            <option value="DATA">DATA</option>
            <option value="CONTROL">CONTROL</option>
            <option value="CLOCK">CLOCK</option>
            <option value="POWER">POWER</option>
          </select>

          <select
            value={filterStatus}
            onChange={(e) => setFilterStatus(e.target.value)}
            className="text-xs bg-slate-950 border border-slate-800 py-1 px-2 rounded font-mono text-slate-300"
          >
            <option value="ALL">All Status</option>
            <option value="ROUTED">Routed</option>
            <option value="UNROUTED">Unrouted</option>
          </select>
        </div>
      </CardHeader>

      {/* Table Content */}
      <CardContent className="flex-1 overflow-y-auto p-0">
        <table className="w-full text-left text-xs border-collapse">
          <thead className="bg-slate-950/80 sticky top-0 border-b border-slate-800 text-slate-400 font-mono text-[11px]">
            <tr>
              <th className="py-2 px-3">ID</th>
              <th className="py-2 px-3">Net Name</th>
              <th className="py-2 px-3">Connection</th>
              <th className="py-2 px-2 text-center">Type</th>
              <th className="py-2 px-2 text-center">Prio</th>
              <th className="py-2 px-3 text-right">Status</th>
              <th className="py-2 px-2 text-center"></th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-800/40 font-mono text-[11px]">
            {filteredNets.map((net) => {
              const record = routeMap.get(net.id);
              const isSelected = selectedNetId === net.id;

              return (
                <tr
                  key={net.id}
                  onClick={() => onSelectNet(net.id)}
                  className={`hover:bg-slate-800/50 cursor-pointer transition-colors ${
                    isSelected ? 'bg-cyan-500/15 border-l-2 border-cyan-400' : ''
                  }`}
                >
                  <td className="py-1.5 px-3 text-slate-500">{net.id}</td>
                  <td className="py-1.5 px-3 font-semibold text-slate-200">{net.name}</td>
                  <td className="py-1.5 px-3 text-slate-400">
                    <span className="text-cyan-300">{getCompName(net.source_comp_id)}</span>
                    <ArrowRight className="w-3 h-3 inline mx-1 text-slate-600" />
                    <span className="text-purple-300">{getCompName(net.target_comp_id)}</span>
                  </td>
                  <td className="py-1.5 px-2 text-center">
                    <span className={`px-1.5 py-0.5 rounded text-[9px] font-semibold ${
                      net.type === 'DATA' ? 'bg-cyan-500/10 text-cyan-400' :
                      net.type === 'CONTROL' ? 'bg-amber-500/10 text-amber-400' :
                      net.type === 'CLOCK' ? 'bg-emerald-500/10 text-emerald-400' :
                      'bg-purple-500/10 text-purple-400'
                    }`}>
                      {net.type}
                    </span>
                  </td>
                  <td className="py-1.5 px-2 text-center text-slate-300">{net.priority}</td>
                  <td className="py-1.5 px-3 text-right">
                    {record ? (
                      record.routed ? (
                        <span className="text-emerald-400 inline-flex items-center gap-1 font-semibold">
                          <CheckCircle2 className="w-3 h-3" />
                          {record.wirelength.toFixed(0)}u
                        </span>
                      ) : (
                        <span className="text-rose-400 inline-flex items-center gap-1 font-semibold" title={record.fail_reason}>
                          <XCircle className="w-3 h-3" />
                          Failed
                        </span>
                      )
                    ) : (
                      <span className="text-slate-600 font-mono">Unrouted</span>
                    )}
                  </td>
                  <td className="py-1.5 px-2 text-center">
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onDeleteNet(net.id);
                      }}
                      className="p-1 text-slate-500 hover:text-rose-400 opacity-60 hover:opacity-100"
                    >
                      <Trash2 className="w-3 h-3" />
                    </button>
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </CardContent>

      {/* Add Net Modal */}
      {showAddModal && (
        <div className="fixed inset-0 z-50 bg-black/70 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="bg-slate-900 border border-slate-700 rounded-xl max-w-md w-full p-5 shadow-2xl space-y-4">
            <h3 className="font-semibold text-sm text-slate-100 flex items-center gap-2">
              <Plus className="w-4 h-4 text-cyan-400" />
              Add Logical Connection
            </h3>

            <form onSubmit={handleAddSubmit} className="space-y-3 text-xs">
              <div>
                <label className="text-slate-300 block mb-1">Net Identifier Name</label>
                <input
                  type="text"
                  required
                  placeholder="e.g. Core0_to_LLC1"
                  value={newNetName}
                  onChange={(e) => setNewNetName(e.target.value)}
                  className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5"
                />
              </div>

              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="text-slate-300 block mb-1">Source Block</label>
                  <select
                    value={newSrcComp}
                    onChange={(e) => setNewSrcComp(parseInt(e.target.value))}
                    className="w-full bg-slate-950 border border-slate-800 rounded px-2 py-1.5"
                  >
                    {arch?.components.map((c) => (
                      <option key={c.id} value={c.id}>{c.name}</option>
                    ))}
                  </select>
                </div>
                <div>
                  <label className="text-slate-300 block mb-1">Target Block</label>
                  <select
                    value={newTgtComp}
                    onChange={(e) => setNewTgtComp(parseInt(e.target.value))}
                    className="w-full bg-slate-950 border border-slate-800 rounded px-2 py-1.5"
                  >
                    {arch?.components.map((c) => (
                      <option key={c.id} value={c.id}>{c.name}</option>
                    ))}
                  </select>
                </div>
              </div>

              <div className="grid grid-cols-3 gap-2">
                <div>
                  <label className="text-slate-300 block mb-1">Type</label>
                  <select
                    value={newType}
                    onChange={(e) => setNewType(e.target.value as NetType)}
                    className="w-full bg-slate-950 border border-slate-800 rounded px-2 py-1.5"
                  >
                    <option value="DATA">DATA</option>
                    <option value="CONTROL">CONTROL</option>
                    <option value="CLOCK">CLOCK</option>
                    <option value="POWER">POWER</option>
                  </select>
                </div>
                <div>
                  <label className="text-slate-300 block mb-1">Priority (1-5)</label>
                  <input
                    type="number"
                    min={1}
                    max={5}
                    value={newPriority}
                    onChange={(e) => setNewPriority(parseInt(e.target.value) || 3)}
                    className="w-full bg-slate-950 border border-slate-800 rounded px-2 py-1.5"
                  />
                </div>
                <div>
                  <label className="text-slate-300 block mb-1">Demand Tracks</label>
                  <input
                    type="number"
                    min={1}
                    max={4}
                    value={newDemand}
                    onChange={(e) => setNewDemand(parseInt(e.target.value) || 1)}
                    className="w-full bg-slate-950 border border-slate-800 rounded px-2 py-1.5"
                  />
                </div>
              </div>

              <div className="flex justify-end gap-2 pt-3 border-t border-slate-800">
                <Button variant="ghost" size="sm" type="button" onClick={() => setShowAddModal(false)}>
                  Cancel
                </Button>
                <Button variant="primary" size="sm" type="submit">
                  Create Net
                </Button>
              </div>
            </form>
          </div>
        </div>
      )}
    </Card>
  );
}
