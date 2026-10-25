import React, { useState, useRef, useEffect, useCallback } from 'react';
import { ZoomIn, ZoomOut, Maximize2, Layers, ShieldAlert, Activity, Hand, Move } from 'lucide-react';
import { Architecture, ComponentInstance, RoutingResults, LogicalNet, BlockedRegion } from '../types';

interface ChipCanvasProps {
  arch: Architecture;
  results: RoutingResults | null;
  selectedCompId: number | null;
  selectedNetId: number | null;
  blockedRegions: BlockedRegion[];
  onSelectComponent: (id: number | null) => void;
  onSelectNet: (id: number | null) => void;
  onComponentMoved: (compId: number, newX: number, newY: number) => Promise<boolean>;
  onAddBlockedRegion: (region: BlockedRegion) => void;
}

const COMPONENT_COLORS: Record<string, { fill: string; stroke: string; text: string }> = {
  P_CORE: { fill: '#1e3a8a', stroke: '#3b82f6', text: '#93c5fd' },
  E_CORE: { fill: '#0c4a6e', stroke: '#0284c7', text: '#7dd3fc' },
  L1_INST: { fill: '#064e3b', stroke: '#059669', text: '#6ee7b7' },
  L1_DATA: { fill: '#064e3b', stroke: '#059669', text: '#6ee7b7' },
  L2_CACHE: { fill: '#134e4a', stroke: '#0d9488', text: '#5eead4' },
  LLC_SLICE: { fill: '#164e63', stroke: '#0891b2', text: '#67e8f9' },
  MEM_CTRL: { fill: '#581c87', stroke: '#9333ea', text: '#d8b4fe' },
  SYSTEM_AGENT: { fill: '#78350f', stroke: '#d97706', text: '#fde68a' },
  PCIE_IO: { fill: '#7c2d12', stroke: '#ea580c', text: '#fed7aa' },
  ROUTER: { fill: '#334155', stroke: '#64748b', text: '#cbd5e1' },
  UNKNOWN: { fill: '#1f2937', stroke: '#4b5563', text: '#9ca3af' }
};

export function ChipCanvas({
  arch,
  results,
  selectedCompId,
  selectedNetId,
  blockedRegions,
  onSelectComponent,
  onSelectNet,
  onComponentMoved,
  onAddBlockedRegion
}: ChipCanvasProps) {
  const containerRef = useRef<HTMLDivElement>(null);

  // Zoom and pan state
  const [zoom, setZoom] = useState<number>(7.5);
  const [pan, setPan] = useState<{ x: number; y: number }>({ x: 50, y: 40 });
  const [isPanning, setIsPanning] = useState(false);
  const [isSpacePressed, setIsSpacePressed] = useState(false);

  // Keep live references for zero-lag event handling
  const zoomRef = useRef(zoom);
  zoomRef.current = zoom;
  const panRef = useRef(pan);
  panRef.current = pan;

  const isPanningRef = useRef(false);
  const panStartRef = useRef<{ x: number; y: number }>({ x: 0, y: 0 });

  // Dragging component block
  const [draggingCompId, setDraggingCompId] = useState<number | null>(null);
  const [dragOffset, setDragOffset] = useState<{ x: number; y: number }>({ x: 0, y: 0 });
  const [dragPos, setDragPos] = useState<{ x: number; y: number } | null>(null);

  const draggingCompIdRef = useRef<number | null>(null);
  draggingCompIdRef.current = draggingCompId;
  const dragOffsetRef = useRef<{ x: number; y: number }>({ x: 0, y: 0 });
  dragOffsetRef.current = dragOffset;
  const dragPosRef = useRef<{ x: number; y: number } | null>(null);
  dragPosRef.current = dragPos;

  // Visualization toggles
  const [showHeatmap, setShowHeatmap] = useState(true);
  const [showPorts, setShowPorts] = useState(true);
  const [showWires, setShowWires] = useState(true);
  const [showUnrouted, setShowUnrouted] = useState(true);

  // Add keepout tool
  const [isDrawingKeepout, setIsDrawingKeepout] = useState(false);
  const [keepoutStart, setKeepoutStart] = useState<{ x: number; y: number } | null>(null);

  const dieW = arch.config.die_width;
  const dieH = arch.config.die_height;

  // Fit to screen / center view
  const handleResetView = useCallback(() => {
    if (!containerRef.current) {
      setZoom(7.5);
      setPan({ x: 50, y: 40 });
      return;
    }
    const rect = containerRef.current.getBoundingClientRect();
    const padding = 50;
    const availW = Math.max(rect.width - padding * 2, 200);
    const availH = Math.max(rect.height - padding * 2, 200);

    const fitZoom = Math.min(availW / dieW, availH / dieH);
    const clampedZoom = Math.max(Math.min(fitZoom, 14.0), 3.0);

    const fitPanX = (rect.width - dieW * clampedZoom) / 2;
    const fitPanY = (rect.height - dieH * clampedZoom) / 2;

    setZoom(clampedZoom);
    setPan({ x: Math.round(fitPanX), y: Math.round(fitPanY) });
  }, [dieW, dieH]);

  // Center on mount or when die size changes
  useEffect(() => {
    handleResetView();
  }, [handleResetView]);

  // Convert mouse screen client coords to chip die coordinates
  const clientToDieCoords = useCallback((clientX: number, clientY: number) => {
    if (!containerRef.current) return { x: 0, y: 0 };
    const rect = containerRef.current.getBoundingClientRect();
    const currentPan = panRef.current;
    const currentZoom = zoomRef.current;
    const svgX = clientX - rect.left - currentPan.x;
    const svgY = clientY - rect.top - currentPan.y;
    return {
      x: svgX / currentZoom,
      y: svgY / currentZoom
    };
  }, []);

  // Native non-passive wheel listener for smooth zoom centered on mouse cursor
  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const onNativeWheel = (e: WheelEvent) => {
      e.preventDefault();
      const rect = container.getBoundingClientRect();
      const mouseX = e.clientX - rect.left;
      const mouseY = e.clientY - rect.top;

      const currentZoom = zoomRef.current;
      const currentPan = panRef.current;

      // Smooth zoom scaling
      const zoomFactor = e.deltaY < 0 ? 1.14 : 0.88;
      const newZoom = Math.min(Math.max(currentZoom * zoomFactor, 2.0), 45.0);

      // Keep point under cursor completely stationary
      const mouseDieX = (mouseX - currentPan.x) / currentZoom;
      const mouseDieY = (mouseY - currentPan.y) / currentZoom;

      const newPanX = mouseX - mouseDieX * newZoom;
      const newPanY = mouseY - mouseDieY * newZoom;

      setZoom(newZoom);
      setPan({ x: newPanX, y: newPanY });
    };

    container.addEventListener('wheel', onNativeWheel, { passive: false });
    return () => {
      container.removeEventListener('wheel', onNativeWheel);
    };
  }, []);

  // Spacebar pan listener
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.code === 'Space' && !e.repeat && (e.target as HTMLElement).tagName !== 'INPUT') {
        setIsSpacePressed(true);
      }
    };
    const handleKeyUp = (e: KeyboardEvent) => {
      if (e.code === 'Space') {
        setIsSpacePressed(false);
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, []);

  // Global window listeners for drag & pan so rapid mouse swipes never lose tracking
  useEffect(() => {
    const handleGlobalMouseMove = (e: MouseEvent) => {
      if (isPanningRef.current) {
        const newX = e.clientX - panStartRef.current.x;
        const newY = e.clientY - panStartRef.current.y;
        setPan({ x: newX, y: newY });
        return;
      }

      if (draggingCompIdRef.current !== null) {
        const coords = clientToDieCoords(e.clientX, e.clientY);
        const rawX = coords.x - dragOffsetRef.current.x;
        const rawY = coords.y - dragOffsetRef.current.y;
        // Snap to 0.5 pitch grid
        const snappedX = Math.round(rawX * 2) / 2;
        const snappedY = Math.round(rawY * 2) / 2;
        setDragPos({ x: snappedX, y: snappedY });
      }
    };

    const handleGlobalMouseUp = async () => {
      if (isPanningRef.current) {
        isPanningRef.current = false;
        setIsPanning(false);
      }

      if (draggingCompIdRef.current !== null && dragPosRef.current) {
        const compId = draggingCompIdRef.current;
        const newX = dragPosRef.current.x;
        const newY = dragPosRef.current.y;

        setDraggingCompId(null);
        setDragPos(null);

        // Validate move with C backend
        await onComponentMoved(compId, newX, newY);
      } else {
        setDraggingCompId(null);
        setDragPos(null);
      }
    };

    window.addEventListener('mousemove', handleGlobalMouseMove);
    window.addEventListener('mouseup', handleGlobalMouseUp);

    return () => {
      window.removeEventListener('mousemove', handleGlobalMouseMove);
      window.removeEventListener('mouseup', handleGlobalMouseUp);
    };
  }, [clientToDieCoords, onComponentMoved]);

  // Toolbar zoom buttons (centered on viewport)
  const handleZoomCenter = (factor: number) => {
    if (!containerRef.current) return;
    const rect = containerRef.current.getBoundingClientRect();
    const centerX = rect.width / 2;
    const centerY = rect.height / 2;

    const newZoom = Math.min(Math.max(zoom * factor, 2.0), 45.0);
    const centerDieX = (centerX - pan.x) / zoom;
    const centerDieY = (centerY - pan.y) / zoom;

    const newPanX = centerX - centerDieX * newZoom;
    const newPanY = centerY - centerDieY * newZoom;

    setZoom(newZoom);
    setPan({ x: newPanX, y: newPanY });
  };

  const handleZoomIn = () => handleZoomCenter(1.25);
  const handleZoomOut = () => handleZoomCenter(0.8);

  // Canvas Mouse Down: Start Panning or Start Keepout
  const handleCanvasMouseDown = (e: React.MouseEvent) => {
    if (isDrawingKeepout) {
      const coords = clientToDieCoords(e.clientX, e.clientY);
      setKeepoutStart(coords);
      return;
    }

    // Left click on background, or middle click, or right click, or spacebar down
    if (e.button === 0 || e.button === 1 || e.button === 2 || isSpacePressed) {
      isPanningRef.current = true;
      panStartRef.current = { x: e.clientX - pan.x, y: e.clientY - pan.y };
      setIsPanning(true);
      onSelectComponent(null);
    }
  };

  const handleCanvasMouseUp = () => {
    if (isDrawingKeepout && keepoutStart) {
      onAddBlockedRegion({
        x: Math.max(0, Math.round(keepoutStart.x)),
        y: Math.max(0, Math.round(keepoutStart.y)),
        width: 8,
        height: 6
      });
      setIsDrawingKeepout(false);
      setKeepoutStart(null);
    }
  };

  // Start dragging component block
  const startDragComponent = (e: React.MouseEvent, comp: ComponentInstance) => {
    // If holding spacebar or middle/right click, pan instead of moving component
    if (isSpacePressed || e.button === 1 || e.button === 2) {
      isPanningRef.current = true;
      panStartRef.current = { x: e.clientX - pan.x, y: e.clientY - pan.y };
      setIsPanning(true);
      return;
    }

    e.stopPropagation();
    onSelectComponent(comp.id);
    const coords = clientToDieCoords(e.clientX, e.clientY);
    setDraggingCompId(comp.id);
    setDragOffset({
      x: coords.x - comp.bounds.x,
      y: coords.y - comp.bounds.y
    });
    setDragPos({ x: comp.bounds.x, y: comp.bounds.y });
  };

  // Determine cursor style
  let cursorClass = 'cursor-grab';
  if (isDrawingKeepout) cursorClass = 'cursor-crosshair';
  else if (isPanning) cursorClass = 'cursor-grabbing';
  else if (isSpacePressed) cursorClass = 'cursor-grab';
  else if (draggingCompId !== null) cursorClass = 'cursor-move';

  return (
    <div
      ref={containerRef}
      onMouseDown={handleCanvasMouseDown}
      onMouseUp={handleCanvasMouseUp}
      onContextMenu={(e) => e.preventDefault()}
      className={`relative w-full h-full bg-slate-950 overflow-hidden select-none ${cursorClass} border border-slate-800 rounded-xl shadow-2xl`}
    >
      {/* CAD Floating Toolbar */}
      <div className="absolute top-3 left-3 z-30 flex items-center gap-1.5 bg-slate-900/95 p-1.5 rounded-lg border border-slate-800 backdrop-blur-md shadow-lg">
        <button
          onClick={handleZoomIn}
          title="Zoom In (or Scroll Up on Canvas)"
          className="p-1.5 hover:bg-slate-800 text-slate-300 hover:text-cyan-400 rounded transition-colors"
        >
          <ZoomIn className="w-4 h-4" />
        </button>
        <button
          onClick={handleZoomOut}
          title="Zoom Out (or Scroll Down on Canvas)"
          className="p-1.5 hover:bg-slate-800 text-slate-300 hover:text-cyan-400 rounded transition-colors"
        >
          <ZoomOut className="w-4 h-4" />
        </button>
        <button
          onClick={handleResetView}
          title="Fit & Center View"
          className="p-1.5 hover:bg-slate-800 text-slate-300 hover:text-cyan-400 rounded transition-colors"
        >
          <Maximize2 className="w-4 h-4" />
        </button>

        <div className="h-4 w-px bg-slate-800 mx-1" />

        <div className="flex items-center gap-1.5 px-2 py-1 text-[11px] text-slate-300 bg-slate-800/80 rounded font-mono border border-slate-700/50 shadow-inner">
          <Hand className="w-3.5 h-3.5 text-cyan-400" />
          <span>Pan: Drag canvas</span>
        </div>

        <div className="h-4 w-px bg-slate-800 mx-1" />

        <button
          onClick={() => setShowWires(!showWires)}
          title="Toggle Wires"
          className={`px-2 py-1 text-[11px] font-mono rounded flex items-center gap-1 transition-all ${
            showWires ? 'bg-cyan-500/20 text-cyan-400 border border-cyan-500/40' : 'text-slate-400 hover:bg-slate-800'
          }`}
        >
          <Layers className="w-3.5 h-3.5" />
          Wires
        </button>

        <button
          onClick={() => setShowHeatmap(!showHeatmap)}
          title="Toggle Congestion Heatmap"
          className={`px-2 py-1 text-[11px] font-mono rounded flex items-center gap-1 transition-all ${
            showHeatmap ? 'bg-amber-500/20 text-amber-400 border border-amber-500/40' : 'text-slate-400 hover:bg-slate-800'
          }`}
        >
          <Activity className="w-3.5 h-3.5" />
          Traffic
        </button>

        <button
          onClick={() => setIsDrawingKeepout(!isDrawingKeepout)}
          title="Add Blocked Region"
          className={`px-2 py-1 text-[11px] font-mono rounded flex items-center gap-1 transition-all ${
            isDrawingKeepout ? 'bg-rose-500/30 text-rose-300 border border-rose-500/50 animate-pulse' : 'text-slate-400 hover:bg-slate-800'
          }`}
        >
          <ShieldAlert className="w-3.5 h-3.5" />
          {isDrawingKeepout ? 'Click to Place' : '+ Keepout'}
        </button>
      </div>

      {/* Die Coordinate HUD */}
      <div className="absolute bottom-3 left-3 z-30 flex items-center gap-2 text-[10px] font-mono text-slate-400 bg-slate-900/90 px-3 py-1.5 rounded-lg border border-slate-800 shadow-md">
        <span>Die: {dieW} x {dieH} units</span>
        <span className="text-slate-600">•</span>
        <span>Zoom: {(zoom * 10).toFixed(0)}%</span>
        <span className="text-slate-600">•</span>
        <span className="text-cyan-400">💡 Drag background to pan • Scroll to zoom centered at cursor</span>
      </div>

      {/* SVG Canvas */}
      <svg className="w-full h-full block">
        <defs>
          {/* Small Grid pattern */}
          <pattern id="smallGrid" width={zoom} height={zoom} patternUnits="userSpaceOnUse">
            <path d={`M ${zoom} 0 L 0 0 0 ${zoom}`} fill="none" stroke="rgba(56, 189, 248, 0.05)" strokeWidth="0.5" />
          </pattern>
          {/* Major Grid pattern */}
          <pattern id="majorGrid" width={zoom * 10} height={zoom * 10} patternUnits="userSpaceOnUse">
            <rect width={zoom * 10} height={zoom * 10} fill="url(#smallGrid)" />
            <path d={`M ${zoom * 10} 0 L 0 0 0 ${zoom * 10}`} fill="none" stroke="rgba(56, 189, 248, 0.12)" strokeWidth="1" />
          </pattern>

          {/* Blocked Keepout diagonal hatch */}
          <pattern id="blockedHatch" width="8" height="8" patternTransform="rotate(45 0 0)" patternUnits="userSpaceOnUse">
            <line x1="0" y1="0" x2="0" y2="8" stroke="rgba(244, 63, 94, 0.5)" strokeWidth="2" />
          </pattern>
        </defs>

        {/* Master transform group for panning and zoom positioning */}
        <g transform={`translate(${pan.x}, ${pan.y})`}>
          {/* Die Background Grid */}
          <rect
            x={0}
            y={0}
            width={dieW * zoom}
            height={dieH * zoom}
            fill="url(#majorGrid)"
            stroke="#0284c7"
            strokeWidth="2"
            className="shadow-2xl"
          />

          {/* User-defined Keepouts / Blocked Regions */}
          {blockedRegions.map((blk, idx) => (
            <g key={`blocked-${idx}`}>
              <rect
                x={blk.x * zoom}
                y={blk.y * zoom}
                width={blk.width * zoom}
                height={blk.height * zoom}
                fill="url(#blockedHatch)"
                stroke="#f43f5e"
                strokeWidth="1.5"
                strokeDasharray="4,2"
              />
              <text
                x={(blk.x + blk.width / 2) * zoom}
                y={(blk.y + blk.height / 2) * zoom}
                fill="#fda4af"
                fontSize="9"
                fontFamily="var(--font-mono)"
                textAnchor="middle"
                dominantBaseline="middle"
                className="pointer-events-none font-semibold"
              >
                KEEPOUT
              </text>
            </g>
          ))}

          {/* Active Congestion Heatmap Segments */}
          {showHeatmap && results?.active_segments && (
            <g opacity="0.85">
              {results.active_segments.map((seg, sIdx) => {
                let color = '#34d399'; // < 0.5
                if (seg.ratio > 0.85) color = '#f43f5e'; // Heavy congestion
                else if (seg.ratio > 0.5) color = '#fbbf24'; // Medium

                return (
                  <line
                    key={`seg-${sIdx}`}
                    x1={seg.x1 * zoom}
                    y1={seg.y1 * zoom}
                    x2={seg.x2 * zoom}
                    y2={seg.y2 * zoom}
                    stroke={color}
                    strokeWidth={Math.max(2, seg.demand * 1.5)}
                    strokeLinecap="round"
                  />
                );
              })}
            </g>
          )}

          {/* Routed Wires */}
          {showWires && results?.routes && (
            <g>
              {results.routes.map((rec) => {
                if (!rec.routed || !rec.path || rec.path.length < 2) return null;

                const isSelected = selectedNetId === rec.net_id;
                const pointsStr = rec.path.map((pt) => `${pt.x * zoom},${pt.y * zoom}`).join(' ');

                let strokeColor = '#38bdf8';
                let strokeW = 1.5;

                if (isSelected) {
                  strokeColor = '#facc15';
                  strokeW = 3.5;
                } else if (rec.max_edge_congestion > 0.85) {
                  strokeColor = '#f43f5e';
                } else if (rec.demand > 1) {
                  strokeColor = '#a855f7';
                }

                return (
                  <g key={`wire-${rec.net_id}`} onClick={() => onSelectNet(rec.net_id)} className="cursor-pointer">
                    <polyline
                      points={pointsStr}
                      fill="none"
                      stroke={strokeColor}
                      strokeWidth={strokeW}
                      strokeLinecap="round"
                      strokeLinejoin="round"
                      opacity={isSelected ? 1.0 : 0.75}
                      className={isSelected ? 'wire-animated' : ''}
                    />
                  </g>
                );
              })}
            </g>
          )}

          {/* Unrouted Net Airwires */}
          {showUnrouted && results?.routes && (
            <g opacity="0.6">
              {results.routes
                .filter((r) => !r.routed)
                .map((r) => {
                  const net = arch.nets.find((n) => n.id === r.net_id);
                  if (!net) return null;
                  const src = arch.components.find((c) => c.id === net.source_comp_id);
                  const tgt = arch.components.find((c) => c.id === net.target_comp_id);
                  if (!src || !tgt) return null;

                  const sx = (src.bounds.x + src.bounds.width / 2) * zoom;
                  const sy = (src.bounds.y + src.bounds.height / 2) * zoom;
                  const tx = (tgt.bounds.x + tgt.bounds.width / 2) * zoom;
                  const ty = (tgt.bounds.y + tgt.bounds.height / 2) * zoom;

                  return (
                    <line
                      key={`unrouted-${r.net_id}`}
                      x1={sx}
                      y1={sy}
                      x2={tx}
                      y2={ty}
                      stroke="#f43f5e"
                      strokeWidth="1.5"
                      strokeDasharray="4,4"
                    />
                  );
                })}
            </g>
          )}

          {/* Component Blocks */}
          {arch.components.map((comp) => {
            const isSelected = selectedCompId === comp.id;
            const isDragging = draggingCompId === comp.id;
            const colors = COMPONENT_COLORS[comp.type] || COMPONENT_COLORS.UNKNOWN;

            const posX = isDragging && dragPos ? dragPos.x : comp.bounds.x;
            const posY = isDragging && dragPos ? dragPos.y : comp.bounds.y;

            return (
              <g
                key={`comp-${comp.id}`}
                onMouseDown={(e) => startDragComponent(e, comp)}
                className="cursor-move group"
              >
                {/* Block Rect */}
                <rect
                  x={posX * zoom}
                  y={posY * zoom}
                  width={comp.bounds.width * zoom}
                  height={comp.bounds.height * zoom}
                  rx={3}
                  fill={colors.fill}
                  stroke={isSelected ? '#38bdf8' : colors.stroke}
                  strokeWidth={isSelected ? 2.5 : 1}
                  opacity={isDragging ? 0.8 : 0.95}
                  className="transition-colors hover:brightness-125"
                />

                {/* Label */}
                <text
                  x={(posX + comp.bounds.width / 2) * zoom}
                  y={(posY + comp.bounds.height / 2) * zoom}
                  fill={colors.text}
                  fontSize={Math.max(8, Math.min(11, comp.bounds.height * zoom * 0.28))}
                  fontFamily="var(--font-mono)"
                  fontWeight="600"
                  textAnchor="middle"
                  dominantBaseline="middle"
                  className="pointer-events-none select-none tracking-tight"
                >
                  {comp.name}
                </text>

                {/* Ports */}
                {showPorts &&
                  comp.ports.map((port, pIdx) => {
                    const px = (posX + port.rel_x * comp.bounds.width) * zoom;
                    const py = (posY + port.rel_y * comp.bounds.height) * zoom;

                    return (
                      <circle
                        key={`port-${comp.id}-${pIdx}`}
                        cx={px}
                        cy={py}
                        r={2}
                        fill="#38bdf8"
                        stroke="#0f172a"
                        strokeWidth="0.8"
                        className="hover:scale-150 transition-transform"
                      >
                        <title>{`${comp.name} : ${port.name}`}</title>
                      </circle>
                    );
                  })}
              </g>
            );
          })}
        </g>
      </svg>
    </div>
  );
}
