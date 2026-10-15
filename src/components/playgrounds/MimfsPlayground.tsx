import React, { useState, useEffect, useRef, useCallback } from 'react';
import { IconLayers } from '../Icons';
import type { ShellResponse } from '../../lib/mimfs.worker';
import { emptyView, type Line } from '../../lib/mimfs';

export const MimfsPlayground: React.FC = React.memo(() => {
  const [volume, setVolume] = useState(emptyView);
  const [history, setHistory] = useState<Line[]>([{type:'output',text:"Loading the real MIMFS WebAssembly runtime…"}]);
  const [inputVal, setInputVal] = useState('');
  const [selectedBlock, setSelectedBlock] = useState<number | null>(null);
  const [activeView, setActiveView] = useState<'terminal' | 'block_map'>('terminal');
  const terminalEndRef = useRef<HTMLDivElement | null>(null);
  const { inodes, blocks: blockAllocation } = volume;
  useEffect(() => {
    const container = terminalEndRef.current?.parentElement;
    if (container) container.scrollTop = container.scrollHeight;
  }, [history, activeView]);
  const workerRef = useRef<Worker | null>(null);
  const requestId = useRef(0);
  const pending = useRef(false);
  const timer = useRef<ReturnType<typeof setTimeout>>();
  const [busy, setBusy] = useState(false);
  const commandHistory = useRef<string[]>([]);
  const historyIndex = useRef(0);
  const draft = useRef('');
  useEffect(() => () => {
    workerRef.current?.terminate();
    clearTimeout(timer.current);
  }, []);

  const runCommands = useCallback((commands: string[]) => {
    if (pending.current) return;
    pending.current = true;
    setBusy(true);
    const id = ++requestId.current;
    const fail = () => {
      if (id !== requestId.current || !pending.current) return;
      workerRef.current?.terminate();
      workerRef.current = null;
      clearTimeout(timer.current);
      pending.current = false;
      setBusy(false);
      setVolume(emptyView());
      setHistory(previous => [...previous, {type: 'error' as const, text: 'WebAssembly stopped or could not load. The next command starts a fresh volume; previous data is no longer available.'}].slice(-200));
    };
    try {
      const worker = workerRef.current ?? new Worker(new URL('../../lib/mimfs.worker.ts', import.meta.url), { type: 'module' });
      workerRef.current = worker;
      worker.onmessage = ({ data }: MessageEvent<ShellResponse>) => {
        if (data.id !== requestId.current || !pending.current) return;
        clearTimeout(timer.current);
        pending.current = false;
        setBusy(false);
        if (data.fatal) {
          worker.terminate();
          workerRef.current = null;
          setVolume(emptyView());
        } else if (data.volume) setVolume(data.volume);
        setHistory(previous => [...(data.clear ? [] : previous), ...data.lines].slice(-200));
      };
      worker.onerror = fail;
      worker.onmessageerror = fail;
      timer.current = setTimeout(fail, 15000);
      worker.postMessage({id, commands});
    } catch { fail(); }
  }, []);

  useEffect(() => {
    runCommands([]);
    return () => { pending.current = false; workerRef.current = null; };
  }, [runCommands]);

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.nativeEvent.isComposing) return;
    if (e.key === 'Enter' && !pending.current && inputVal.trim()) {
      e.preventDefault();
      commandHistory.current = [...commandHistory.current, inputVal].slice(-100);
      historyIndex.current = commandHistory.current.length;
      draft.current = '';
      runCommands([inputVal]);
      setInputVal('');
    } else if (e.key === 'ArrowUp' || e.key === 'ArrowDown') {
      e.preventDefault();
      if (historyIndex.current === commandHistory.current.length) draft.current = inputVal;
      historyIndex.current = Math.max(0, Math.min(commandHistory.current.length, historyIndex.current + (e.key === 'ArrowUp' ? -1 : 1)));
      setInputVal(commandHistory.current[historyIndex.current] ?? draft.current);
    } else if (e.ctrlKey && e.key.toLowerCase() === 'l') {
      e.preventDefault();
      setHistory([]);
    }
  };

  const executePresetAction = (preset: string) => {
    const presets: Record<string, string[]> = {
      format_demo: ['format'],
      dir_tree: ['mkdir var', 'mkdir etc', 'write var/system.log Kernel boot sequence loaded.', 'tree'],
      concurrency: ['concurrency', 'ls -la'],
      links: ['write master.cfg version=2.4.0', 'ln master.cfg config_link.cfg', 'ln -s /master.cfg symlink_to_master', 'stat master.cfg'],
      hexdump_demo: ['write hexdump-demo.txt Hello MIMFS', 'hexdump hexdump-demo.txt'],
    };
    runCommands(presets[preset] || []);
  };

  return (
    <div className="interactive-demo border border-[#30363d] rounded-lg overflow-hidden bg-[#0d1117] text-[#e6edf3] font-mono text-xs sm:text-[13px] shadow-lg">
      
      {/* ── Titlebar ── */}
      <div className="bg-[#161b22] px-4 py-2.5 border-b border-[#30363d] flex flex-wrap items-center justify-between gap-3">
        <div className="flex items-center gap-3">
          <div className="flex items-center gap-1.5 border-r border-[#30363d] pr-3">
            <div className="w-2.5 h-2.5 rounded-full bg-[#ff5f56]" />
            <div className="w-2.5 h-2.5 rounded-full bg-[#ffbd2e]" />
            <div className="w-2.5 h-2.5 rounded-full bg-[#27c93f]" />
          </div>
          <span className="font-bold text-white tracking-wide">MIMFS Shell</span>
          <span className="px-2 py-0.5 rounded text-[10px] bg-[#21262d] text-gray-300 border border-[#30363d] flex items-center gap-1.5">
            <span className="w-1.5 h-1.5 rounded-full bg-gray-400" />
            C / WEBASSEMBLY
          </span>
        </div>

        {/* View Toggle Tabs */}
        <div className="flex bg-[#21262d] p-0.5 rounded border border-[#30363d]">
          <button
            onClick={() => setActiveView('terminal')}
            aria-pressed={activeView === 'terminal'}
            className={`px-2.5 py-1 text-xs rounded transition-colors ${
              activeView === 'terminal'
                ? 'bg-[#30363d] text-white font-semibold'
                : 'text-gray-400 hover:text-white'
            }`}
          >
            UNIX Terminal
          </button>
          <button
            onClick={() => setActiveView('block_map')}
            aria-pressed={activeView === 'block_map'}
            className={`px-2.5 py-1 text-xs rounded transition-colors ${
              activeView === 'block_map'
                ? 'bg-[#30363d] text-white font-semibold'
                : 'text-gray-400 hover:text-white'
            }`}
          >
            Block Visualizer
          </button>
        </div>
      </div>

      {/* Quick Demo Action Buttons */}
      <div className="mimfs-presets px-4 py-1.5 bg-[#161b22]/70 border-b border-[#30363d] flex items-center gap-2 flex-wrap text-[11px]">
        <span className="text-gray-400">Demos:</span>
        <button
          disabled={busy}
          onClick={() => executePresetAction('format_demo')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Format Volume
        </button>
        <button
          disabled={busy}
          onClick={() => executePresetAction('dir_tree')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Directories
        </button>
        <button
          disabled={busy}
          onClick={() => executePresetAction('concurrency')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Concurrency
        </button>
        <button
          disabled={busy}
          onClick={() => executePresetAction('links')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Hard / Symlinks
        </button>
        <button
          disabled={busy}
          onClick={() => executePresetAction('hexdump_demo')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Hexdump
        </button>
      </div>

      {/* ── Terminal Console ── */}
      {activeView === 'terminal' && (
        <div className="p-4 bg-[#090d13] text-[#e6edf3] font-mono text-xs sm:text-[13px] h-[320px] sm:h-[350px] lg:h-[400px] flex flex-col justify-between">
          <div className="overflow-y-auto dark-scrollbar space-y-1 pr-2 flex-1">
            {history.map((h, i) => (
              <div
                key={i}
                className={`whitespace-pre-wrap leading-relaxed ${
                  h.type === 'cmd'
                    ? 'text-emerald-400 font-bold'
                    : h.type === 'error'
                    ? 'text-red-400'
                    : h.type === 'success'
                    ? 'text-emerald-400 font-semibold'
                    : 'text-gray-300'
                }`}
              >
                {h.text}
              </div>
            ))}
            <div ref={terminalEndRef} />
          </div>

          <div className="flex items-center gap-2 pt-2 border-t border-[#21262d] mt-2">
            <span className="text-emerald-400 font-bold">$</span>
            <input
              type="text"
              aria-label="MIMFS command"
              aria-busy={busy}
              autoComplete="off"
              spellCheck={false}
              maxLength={16384}
              value={inputVal}
              onChange={(e) => setInputVal(e.target.value)}
              onKeyDown={handleKeyDown}
              placeholder="Type command (e.g. ls -la, mkdir var, concurrency, df -h)..."
              className="w-full bg-transparent text-[#e6edf3] font-mono text-xs sm:text-[13px] focus:outline-none placeholder:text-gray-600"
              
            />
          </div>
        </div>
      )}

      {/* ── Storage Block Map View ── */}
      {activeView === 'block_map' && (
        <div className="p-4 bg-[#090d13] h-[320px] sm:h-[350px] lg:h-[400px] overflow-y-auto dark-scrollbar space-y-4">
          <div className="flex flex-wrap items-center justify-between gap-2">
            <div className="flex items-center gap-2">
              <IconLayers size={15} className="text-emerald-400" />
              <span className="text-xs font-bold text-gray-200">
                First 48 data blocks (4096 B each)
              </span>
            </div>
            <span className="text-[11px] text-gray-400">
              {volume.inodeCount} inodes · {volume.freeBlocks}/{volume.totalBlocks} blocks free
            </span>
          </div>

          <div className="block-legend" aria-label="Block types">
            <span><i className="block-dot free" />Free</span>
            <span><i className="block-dot data_dir" />Directory</span>
            <span><i className="block-dot data_file" />File</span>
            <span><i className="block-dot allocated" />Reserved / indirect</span>
          </div>
          <div className="block-grid" aria-label="First 48 data blocks">
            {blockAllocation.map(block => (
              <button type="button" key={block.id}
                className={`block-cell ${block.type}`}
                aria-pressed={selectedBlock === block.id}
                aria-label={`Block ${block.id}, ${block.type}${block.ownerInode != null ? `, inode ${block.ownerInode}` : ''}`}
                onClick={() => setSelectedBlock(block.id)}>
                <span>{block.id}</span>
              </button>
            ))}
          </div>
          <div className="block-detail" aria-live="polite">
            {selectedBlock === null ? 'Select a block to inspect its allocation.' : (() => {
              const block = blockAllocation.find(item => item.id === selectedBlock);
              return block ? `Block ${block.id} · ${block.type === 'data_dir' ? 'Directory data' : block.type === 'data_file' ? 'File data' : block.type === 'allocated' ? 'Reserved / indirect' : 'Free'}${block.ownerInode != null ? ` · Inode ${block.ownerInode}` : ''} · 4096 bytes` : 'Block unavailable';
            })()}
          </div>

          {/* Inode Table Info */}
          <div className="space-y-2 pt-2 border-t border-[#21262d]">
            <h4 className="text-xs font-bold text-gray-300">Inode Table (first 128 live inodes)</h4>
            <div className="inode-table-wrap">
              <table className="inode-table">
                <thead><tr><th scope="col">Inode</th><th scope="col">Type</th><th scope="col">Size (bytes)</th><th scope="col">Links</th><th scope="col">Data blocks</th></tr></thead>
                <tbody>
                  {Object.values(inodes).map(node => (
                    <tr key={node.id}>
                      <th scope="row">#{node.id}</th><td>{node.type}</td><td>{node.size}</td><td>{node.linkCount}</td>
                      <td>{node.directBlocks.join(', ') || 'none / inline'}</td>
                    </tr>
                  ))}
                  {Object.keys(inodes).length === 0 && <tr><td colSpan={5}>No live inodes. Run a demo or create a file to populate this table.</td></tr>}
                </tbody>
              </table>
            </div>
          </div>
        </div>
      )}
    </div>
  );
});

MimfsPlayground.displayName = 'MimfsPlayground';
export default MimfsPlayground;
