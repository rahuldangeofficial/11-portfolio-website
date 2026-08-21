import React, { useEffect, useRef, useState, useCallback } from 'react';

type VisMode = 'space_curve' | 'memory_slab' | 'concurrency_mesh';

interface Point3D {
  x: number;
  y: number;
  z: number;
  color?: string;
  size?: number;
  highlight?: boolean;
}

interface Edge3D {
  from: number;
  to: number;
  intensity?: number;
}

export const SystemsVisualizer3D: React.FC = React.memo(() => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const containerRef = useRef<HTMLDivElement | null>(null);
  const [mode, setMode] = useState<VisMode>('space_curve');
  const [isInteracting, setIsInteracting] = useState(false);

  // Rotation state
  const rotX = useRef<number>(0.3);
  const rotY = useRef<number>(0.5);
  const targetRotX = useRef<number>(0.3);
  const targetRotY = useRef<number>(0.5);
  const velX = useRef<number>(0.003);
  const velY = useRef<number>(0.005);
  const isDragging = useRef<boolean>(false);
  const lastMousePos = useRef<{ x: number; y: number }>({ x: 0, y: 0 });
  const isVisible = useRef<boolean>(true);
  const animFrameId = useRef<number | null>(null);

  // Generate geometric datasets for 3D modes
  const generateModelData = useCallback((currentMode: VisMode) => {
    const points: Point3D[] = [];
    const edges: Edge3D[] = [];

    if (currentMode === 'space_curve') {
      const order = 3;
      const size = Math.pow(2, order);
      const scale = 140 / size;

      for (let i = 0; i < size * size * size; i += 2) {
        let x = 0, y = 0, z = 0;
        for (let b = 0; b < order; b++) {
          x |= ((i >> (3 * b + 0)) & 1) << b;
          y |= ((i >> (3 * b + 1)) & 1) << b;
          z |= ((i >> (3 * b + 2)) & 1) << b;
        }

        const px = (x - size / 2 + 0.5) * scale;
        const py = (y - size / 2 + 0.5) * scale;
        const pz = (z - size / 2 + 0.5) * scale;

        const isHub = (i % 8 === 0);
        points.push({
          x: px,
          y: py,
          z: pz,
          highlight: isHub,
          size: isHub ? 2.5 : 1.5,
          color: isHub ? '#1877F2' : '#0ea5e9'
        });

        if (points.length > 1) {
          edges.push({
            from: points.length - 2,
            to: points.length - 1,
            intensity: 0.7
          });
        }
      }
    } else if (currentMode === 'memory_slab') {
      const layers = 5;
      const rows = 5;
      const cols = 5;
      const spacing = 28;

      for (let l = 0; l < layers; l++) {
        for (let r = 0; r < rows; r++) {
          for (let c = 0; c < cols; c++) {
            const px = (c - (cols - 1) / 2) * spacing;
            const py = (r - (rows - 1) / 2) * spacing;
            const pz = (l - (layers - 1) / 2) * spacing;

            const isBound = (r === 2 || c === 2 || l === 2);
            const idx = points.length;

            points.push({
              x: px,
              y: py,
              z: pz,
              highlight: isBound,
              size: isBound ? 2.8 : 1.4,
              color: isBound ? '#10b981' : '#1877F2'
            });

            if (c > 0) edges.push({ from: idx - 1, to: idx, intensity: 0.3 });
            if (r > 0) edges.push({ from: idx - cols, to: idx, intensity: 0.3 });
            if (l > 0) edges.push({ from: idx - (rows * cols), to: idx, intensity: 0.3 });
          }
        }
      }
    } else {
      const nodeCount = 36;
      const radius = 80;

      for (let i = 0; i < nodeCount; i++) {
        const phi = Math.acos(-1 + (2 * i) / nodeCount);
        const theta = Math.sqrt(nodeCount * Math.PI) * phi;

        const px = radius * Math.cos(theta) * Math.sin(phi);
        const py = radius * Math.sin(theta) * Math.sin(phi);
        const pz = radius * Math.cos(phi);

        const isCore = i < 4;
        points.push({
          x: px,
          y: py,
          z: pz,
          highlight: isCore,
          size: isCore ? 3.5 : 1.8,
          color: isCore ? '#ef4444' : (i % 2 === 0 ? '#1877F2' : '#8b5cf6')
        });
      }

      for (let i = 0; i < points.length; i++) {
        for (let j = i + 1; j < points.length; j++) {
          const dx = points[i].x - points[j].x;
          const dy = points[i].y - points[j].y;
          const dz = points[i].z - points[j].z;
          const dist = Math.sqrt(dx * dx + dy * dy + dz * dz);
          if (dist < 48) {
            edges.push({ from: i, to: j, intensity: Math.max(0.1, 1 - dist / 48) });
          }
        }
      }
    }

    return { points, edges };
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d', { alpha: true });
    if (!ctx) return;

    let width = 0;
    let height = 0;

    const handleResize = () => {
      const container = containerRef.current;
      if (!container || !canvas) return;
      const rect = container.getBoundingClientRect();
      width = Math.floor(rect.width);
      height = Math.floor(rect.height);
      const dpr = Math.min(window.devicePixelRatio || 1, 2);

      canvas.width = width * dpr;
      canvas.height = height * dpr;
      canvas.style.width = `${width}px`;
      canvas.style.height = `${height}px`;
      ctx.scale(dpr, dpr);
    };

    handleResize();
    window.addEventListener('resize', handleResize);

    const observer = new IntersectionObserver((entries) => {
      entries.forEach((entry) => {
        isVisible.current = entry.isIntersecting;
      });
    }, { threshold: 0.05 });

    if (containerRef.current) {
      observer.observe(containerRef.current);
    }

    const { points, edges } = generateModelData(mode);

    const render = () => {
      animFrameId.current = requestAnimationFrame(render);

      if (!isVisible.current || width === 0 || height === 0) return;

      if (!isDragging.current) {
        targetRotX.current += velX.current * 0.8;
        targetRotY.current += velY.current * 0.8;
      }

      rotX.current += (targetRotX.current - rotX.current) * 0.08;
      rotY.current += (targetRotY.current - rotY.current) * 0.08;

      ctx.clearRect(0, 0, width, height);

      const cx = width / 2;
      const cy = height / 2;
      const fov = 340;

      const sinX = Math.sin(rotX.current);
      const cosX = Math.cos(rotX.current);
      const sinY = Math.sin(rotY.current);
      const cosY = Math.cos(rotY.current);

      const projected: { x: number; y: number; z: number; scale: number; p: Point3D }[] = [];

      for (let i = 0; i < points.length; i++) {
        const p = points[i];

        const x1 = p.x * cosY + p.z * sinY;
        const y1 = p.y;
        const z1 = -p.x * sinY + p.z * cosY;

        const x2 = x1;
        const y2 = y1 * cosX - z1 * sinX;
        let z2 = y1 * sinX + z1 * cosX;

        z2 += 220;

        const scale = fov / Math.max(z2, 10);
        const projX = cx + x2 * scale;
        const projY = cy + y2 * scale;

        projected.push({ x: projX, y: projY, z: z2, scale, p });
      }

      ctx.lineWidth = 1;
      for (let i = 0; i < edges.length; i++) {
        const e = edges[i];
        const p1 = projected[e.from];
        const p2 = projected[e.to];
        if (!p1 || !p2) continue;

        const avgZ = (p1.z + p2.z) / 2;
        const depthAlpha = Math.max(0.08, Math.min(0.65, (300 - avgZ) / 200));

        ctx.strokeStyle = `rgba(24, 119, 242, ${depthAlpha * (e.intensity || 0.5)})`;
        ctx.beginPath();
        ctx.moveTo(p1.x, p1.y);
        ctx.lineTo(p2.x, p2.y);
        ctx.stroke();
      }

      projected.sort((a, b) => b.z - a.z);

      for (let i = 0; i < projected.length; i++) {
        const item = projected[i];
        const nodeSize = (item.p.size || 2) * Math.max(0.6, item.scale * 0.9);
        const alpha = Math.max(0.2, Math.min(0.95, (320 - item.z) / 180));

        if (item.p.highlight) {
          ctx.fillStyle = item.p.color || '#1877F2';
          ctx.beginPath();
          ctx.arc(item.x, item.y, nodeSize * 2.2, 0, Math.PI * 2);
          ctx.globalAlpha = alpha * 0.3;
          ctx.fill();
        }

        ctx.globalAlpha = alpha;
        ctx.fillStyle = item.p.color || '#1877F2';
        ctx.beginPath();
        ctx.arc(item.x, item.y, nodeSize, 0, Math.PI * 2);
        ctx.fill();
      }
      ctx.globalAlpha = 1.0;
    };

    animFrameId.current = requestAnimationFrame(render);

    return () => {
      window.removeEventListener('resize', handleResize);
      observer.disconnect();
      if (animFrameId.current) cancelAnimationFrame(animFrameId.current);
    };
  }, [mode, generateModelData]);

  const handlePointerDown = (e: React.PointerEvent) => {
    isDragging.current = true;
    setIsInteracting(true);
    lastMousePos.current = { x: e.clientX, y: e.clientY };
    (e.target as HTMLElement).setPointerCapture(e.pointerId);
  };

  const handlePointerMove = (e: React.PointerEvent) => {
    if (!isDragging.current) return;
    const dx = e.clientX - lastMousePos.current.x;
    const dy = e.clientY - lastMousePos.current.y;
    lastMousePos.current = { x: e.clientX, y: e.clientY };

    targetRotY.current += dx * 0.008;
    targetRotX.current -= dy * 0.008;
  };

  const handlePointerUp = (e: React.PointerEvent) => {
    isDragging.current = false;
    setIsInteracting(false);
    try {
      (e.target as HTMLElement).releasePointerCapture(e.pointerId);
    } catch {
      // Ignored
    }
  };

  return (
    <div className="relative w-full max-w-lg mx-auto flex flex-col items-center select-none" ref={containerRef}>
      <div 
        className="w-full h-64 sm:h-72 md:h-80 relative flex items-center justify-center cursor-grab active:cursor-grabbing touch-none"
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onPointerCancel={handlePointerUp}
      >
        <canvas ref={canvasRef} className="w-full h-full block" />

        <div className="absolute top-2 left-2 pointer-events-none text-[10px]  tracking-wider text-[var(--fg-muted)] flex items-center gap-1.5 bg-[var(--bg-surface)] px-2 py-0.5 rounded border border-[var(--border-subtle)]">
          <span className="w-1.5 h-1.5 rounded-full bg-[var(--accent-cyan)] animate-pulse" />
          <span>{mode === 'space_curve' ? 'SFC_4D_MANIFOLD' : mode === 'memory_slab' ? 'CACHE_SLAB_MATRIX' : 'CONCURRENT_DAG'}</span>
        </div>

        <div className="absolute bottom-2 right-2 pointer-events-none text-[9px]  text-[var(--fg-faint)] bg-[var(--bg-surface)] px-2 py-0.5 rounded border border-[var(--border-subtle)]">
          {isInteracting ? 'ACTIVE_INTERACTION' : 'DRAG_TO_ORBIT'}
        </div>
      </div>

      <div className="flex items-center gap-1.5 mt-2 p-1 rounded-full bg-[var(--bg-surface)] border border-[var(--border-subtle)] shadow-sm">
        <button
          onClick={() => setMode('space_curve')}
          className={`px-3 py-1 text-xs  rounded-full transition-all duration-200 ${
            mode === 'space_curve'
              ? 'bg-[var(--fg-primary)] text-[var(--bg-primary)] font-semibold shadow-sm'
              : 'text-[var(--fg-secondary)] hover:text-[var(--fg-primary)]'
          }`}
        >
          Space-Filling Curve
        </button>
        <button
          onClick={() => setMode('memory_slab')}
          className={`px-3 py-1 text-xs  rounded-full transition-all duration-200 ${
            mode === 'memory_slab'
              ? 'bg-[var(--fg-primary)] text-[var(--bg-primary)] font-semibold shadow-sm'
              : 'text-[var(--fg-secondary)] hover:text-[var(--fg-primary)]'
          }`}
        >
          Memory Slabs
        </button>
        <button
          onClick={() => setMode('concurrency_mesh')}
          className={`px-3 py-1 text-xs  rounded-full transition-all duration-200 ${
            mode === 'concurrency_mesh'
              ? 'bg-[var(--fg-primary)] text-[var(--bg-primary)] font-semibold shadow-sm'
              : 'text-[var(--fg-secondary)] hover:text-[var(--fg-primary)]'
          }`}
        >
          Concurrency Graph
        </button>
      </div>
    </div>
  );
});

SystemsVisualizer3D.displayName = 'SystemsVisualizer3D';
export default SystemsVisualizer3D;
