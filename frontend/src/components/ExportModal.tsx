import React, { useState } from 'react';
import { Download, Upload, FileText, Check, X } from 'lucide-react';
import { Button } from './UIComponents';
import { Architecture, RoutingResults, ComparisonReport, RoutingParams } from '../types';

interface ExportModalProps {
  isOpen: boolean;
  onClose: () => void;
  arch: Architecture | null;
  params: RoutingParams;
  results: RoutingResults | null;
  comparison: ComparisonReport | null;
  onImportDesign: (design: any) => void;
}

export function ExportModal({
  isOpen,
  onClose,
  arch,
  params,
  results,
  comparison,
  onImportDesign
}: ExportModalProps) {
  const [importJsonText, setImportJsonText] = useState('');
  const [importError, setImportError] = useState<string | null>(null);

  if (!isOpen) return null;

  // Export Design JSON
  const handleExportDesignJSON = () => {
    if (!arch) return;
    const payload = {
      version: '1.0.0',
      export_date: new Date().toISOString(),
      architecture: arch,
      routing_params: params
    };
    const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `physarum_chip_design_${arch.config.topology.toLowerCase()}_${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };

  // Export Results CSV
  const handleExportResultsCSV = () => {
    const activeRes = comparison?.physarum || results;
    if (!activeRes) return;

    let csv = 'Net_ID,Net_Name,Routed,Wirelength,Bends,Peak_Congestion,Failure_Reason\n';
    for (const r of activeRes.routes) {
      csv += `${r.net_id},"${r.net_name}",${r.routed},${r.wirelength.toFixed(1)},${r.bends},${(r.max_edge_congestion * 100).toFixed(1)}%,"${r.fail_reason || ''}"\n`;
    }

    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `physarum_chip_routes_${activeRes.algorithm.toLowerCase()}_${Date.now()}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  };

  // Export Results JSON
  const handleExportResultsJSON = () => {
    const data = comparison || results;
    if (!data) return;
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `physarum_chip_metrics_${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };

  // Import Design
  const handleImportSubmit = () => {
    try {
      setImportError(null);
      const parsed = JSON.parse(importJsonText.trim());
      onImportDesign(parsed);
      onClose();
    } catch (err: any) {
      setImportError(`Invalid JSON format: ${err.message}`);
    }
  };

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (evt) => {
      const content = evt.target?.result as string;
      setImportJsonText(content);
    };
    reader.readAsText(file);
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/80 backdrop-blur-sm flex items-center justify-center p-4">
      <div className="bg-slate-900 border border-slate-700 rounded-xl max-w-lg w-full p-6 shadow-2xl space-y-5 text-slate-100">
        <div className="flex items-center justify-between border-b border-slate-800 pb-3">
          <h2 className="text-sm font-bold flex items-center gap-2">
            <Download className="w-4 h-4 text-cyan-400" />
            Export & Persistence
          </h2>
          <button onClick={onClose} className="text-slate-400 hover:text-slate-100">
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Export Options */}
        <div className="space-y-2">
          <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block">
            Export Design & Benchmark Results
          </span>

          <div className="grid grid-cols-3 gap-2">
            <button
              onClick={handleExportDesignJSON}
              className="bg-slate-950 hover:bg-slate-800 p-3 rounded-lg border border-slate-800 flex flex-col items-center gap-2 text-center transition-all"
            >
              <FileText className="w-5 h-5 text-cyan-400" />
              <span className="text-xs font-semibold text-slate-200">Design JSON</span>
              <span className="text-[10px] text-slate-500">Floorplan + Netlist</span>
            </button>

            <button
              onClick={handleExportResultsCSV}
              disabled={!results && !comparison}
              className="bg-slate-950 hover:bg-slate-800 disabled:opacity-40 p-3 rounded-lg border border-slate-800 flex flex-col items-center gap-2 text-center transition-all"
            >
              <FileText className="w-5 h-5 text-emerald-400" />
              <span className="text-xs font-semibold text-slate-200">Metrics CSV</span>
              <span className="text-[10px] text-slate-500">Wirelength & Congestion</span>
            </button>

            <button
              onClick={handleExportResultsJSON}
              disabled={!results && !comparison}
              className="bg-slate-950 hover:bg-slate-800 disabled:opacity-40 p-3 rounded-lg border border-slate-800 flex flex-col items-center gap-2 text-center transition-all"
            >
              <FileText className="w-5 h-5 text-purple-400" />
              <span className="text-xs font-semibold text-slate-200">Results JSON</span>
              <span className="text-[10px] text-slate-500">Full Raw Coordinates</span>
            </button>
          </div>
        </div>

        {/* Import Option */}
        <div className="space-y-2 pt-2 border-t border-slate-800">
          <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block">
            Import Design JSON
          </span>

          <div className="flex items-center gap-2">
            <input
              type="file"
              accept=".json"
              onChange={handleFileUpload}
              className="text-xs text-slate-400 file:mr-2 file:py-1 file:px-2 file:rounded file:border-0 file:text-xs file:bg-slate-800 file:text-cyan-400 hover:file:bg-slate-700"
            />
          </div>

          <textarea
            rows={4}
            placeholder="Or paste exported design JSON here..."
            value={importJsonText}
            onChange={(e) => setImportJsonText(e.target.value)}
            className="w-full bg-slate-950 border border-slate-800 rounded p-2 text-[10px] font-mono"
          />

          {importError && (
            <p className="text-rose-400 text-xs font-mono">{importError}</p>
          )}

          <Button
            variant="primary"
            size="sm"
            onClick={handleImportSubmit}
            disabled={!importJsonText.trim()}
            icon={<Upload className="w-3.5 h-3.5" />}
          >
            Load Design into Workspace
          </Button>
        </div>
      </div>
    </div>
  );
}
