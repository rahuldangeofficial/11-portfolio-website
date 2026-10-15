import React, { useState, useCallback, useEffect, useRef, useMemo } from 'react';
import { PRESETS } from '../../lib/llfpl-presets';
import type { ExecutionLog, LlfplResult } from '../../lib/llfpl';
import { IconPlay, IconRotateCcw, IconTerminal, IconCpu } from '../Icons';

export const LlfplPlayground: React.FC = React.memo(() => {
  const [files, setFiles] = useState<Record<string, string>>(() => Object.fromEntries(
    Object.entries(PRESETS).map(([key, preset]) => [`${key}.LLFPL`, preset.code])
  ));
  const [activeFile, setActiveFile] = useState('newton_sqrt.LLFPL');
  const [newFileName, setNewFileName] = useState('');
  const [fileError, setFileError] = useState('');
  const sourceCode = files[activeFile] ?? '';
  const setSourceCode = (code: string) => setFiles(previous => ({ ...previous, [activeFile]: code }));
  const createFile = (event: React.FormEvent) => {
    event.preventDefault();
    const raw = newFileName.trim();
    const name = raw.endsWith('.LLFPL') ? raw : `${raw}.LLFPL`;
    if (!raw || !/^[a-zA-Z0-9_-]+\.LLFPL$/.test(name)) { setFileError('Use letters, numbers, hyphens or underscores.'); return; }
    if (Object.hasOwn(files, name)) { setFileError('That file already exists.'); return; }
    if (Object.keys(files).length >= 20) { setFileError('This browser workspace supports up to 20 files.'); return; }
    cancelExecution(); setIsRunning(false);
    setFiles(previous => ({ ...previous, [name]: '-- New LLFPL program\nRequire(stdlib.LLFPL)\n\nCommit(plus(ONE, TWO))\n' }));
    setActiveFile(name); setActiveTab('editor'); setNewFileName(''); setFileError('');
  };
  const [logs, setLogs] = useState<ExecutionLog[]>([]);
  const [slabs, setSlabs] = useState<Record<string, Float64Array>>({});
  const [activeSlabTab, setActiveSlabTab] = useState<string>('');
  const [activeTab, setActiveTab] = useState<'editor' | 'output'>('editor');
  const [isRunning, setIsRunning] = useState<boolean>(false);
  const [stats, setStats] = useState<{ commits: number; totalCost: number; timeNs: number }>({
    commits: 0,
    totalCost: 0,
    timeNs: 0
  });

  const resetFile = () => {
    cancelExecution(); setIsRunning(false);
    const preset = PRESETS[activeFile.replace(/\.LLFPL$/, '')];
    setSourceCode(preset?.code ?? '-- New LLFPL program\nRequire(stdlib.LLFPL)\n\nCommit(plus(ONE, TWO))\n');
    setActiveTab('editor'); setLogs([]); setSlabs({}); setActiveSlabTab('');
    setStats({ commits: 0, totalCost: 0, timeNs: 0 });
  };

  const gutterRef = useRef<HTMLDivElement>(null);
  const syntaxRef = useRef<HTMLPreElement>(null);
  const syncEditorScroll = useCallback((event: React.UIEvent<HTMLTextAreaElement>) => {
    // Read both coordinates before updating the two visual layers.
    const { scrollTop, scrollLeft } = event.currentTarget;
    if (gutterRef.current) gutterRef.current.scrollTop = scrollTop;
    if (syntaxRef.current) {
      syntaxRef.current.scrollTop = scrollTop;
      syntaxRef.current.scrollLeft = scrollLeft;
    }
  }, []);

  const workerRef = useRef<Worker | null>(null);
  const timeoutRef = useRef<ReturnType<typeof setTimeout>>();
  const cancelExecution = useCallback(() => {
    workerRef.current?.terminate();
    workerRef.current = null;
    clearTimeout(timeoutRef.current);
  }, []);
  useEffect(() => cancelExecution, [cancelExecution]);
  const executeCode = useCallback(() => {
    cancelExecution();
    setIsRunning(true);
    // Keep the previous result in place until the worker returns its replacement.
    const fail = (message: string) => {
      cancelExecution();
      setLogs([{ type: 'error', message }]);
      setSlabs({});
      setActiveSlabTab('');
      setStats({ commits: 0, totalCost: 0, timeNs: 0 });
      setIsRunning(false);
    };
    try {
      const worker = new Worker(new URL('../../lib/llfpl.worker.ts', import.meta.url), { type: 'module' });
      workerRef.current = worker;
      worker.onmessage = (event: MessageEvent<{type:'ready'} | {type:'error';message:string} | {type:'result';result:LlfplResult}>) => {
        if (workerRef.current !== worker) return;
        if (event.data.type === 'ready') {
          clearTimeout(timeoutRef.current);
          timeoutRef.current = setTimeout(() => fail('Execution stopped after 2 seconds. Reduce the workload or run the native project locally.'), 2000);
          return;
        }
        if (event.data.type === 'error') { fail(event.data.message); return; }
        const result = event.data.result;
        setLogs(result.logs);
        setSlabs(result.slabs);
        setStats(result.stats);
        setActiveSlabTab(Object.keys(result.slabs)[0] || '');
        cancelExecution();
        setIsRunning(false);
      };
      worker.onerror = () => { if (workerRef.current === worker) fail('WebAssembly worker failed. Reset and try again.'); };
      worker.onmessageerror = () => { if (workerRef.current === worker) fail('Could not read the WebAssembly result. Reset and try again.'); };
      timeoutRef.current = setTimeout(() => fail('WebAssembly could not load within 15 seconds. Check your connection and try again.'), 15000);
      worker.postMessage(sourceCode);
    } catch {
      fail('This browser could not start the demo worker. Use a browser with Web Worker support or run the native project locally.');
    }
  }, [sourceCode, cancelExecution]);

  const highlightCode = (code: string) => {
  const escaped = code.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  const regex = /(--.*)|(\b\d+(?:\.\d+)?\b)|\b(Require|Slab|Identity|Map|Commit|Loop)\b|\b([A-Z][a-zA-Z0-9_]*)\b|\b([a-z_][a-z0-9_]*)\b|([(),])/g;
  
  return escaped.replace(regex, (match, comment, num, kw, varName, funcOrVar, sym) => {
    if (comment) return `<span class="syntax-comment">${comment}</span>`;
    if (num) return `<span class="syntax-number">${num}</span>`;
    if (kw) return `<span class="syntax-keyword">${kw}</span>`;
    if (varName) return `<span class="syntax-variable">${varName}</span>`;
    if (funcOrVar) return `<span class="syntax-function">${funcOrVar}</span>`;
    if (sym) return `<span class="syntax-punctuation">${sym}</span>`;
    return match;
  });
};

// Helper to convert float64 to IEEE-754 Hex string

  const toHexFloat64 = (num: number): string => {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    view.setFloat64(0, num);
    let hex = '0x';
    for (let i = 0; i < 8; i++) {
      hex += view.getUint8(i).toString(16).padStart(2, '0');
    }
    return hex;
  };

  const highlightedCode = useMemo(() => highlightCode(sourceCode), [sourceCode]);

  return (
    <div className="interactive-demo border border-[#30363d] rounded-lg overflow-hidden bg-[#0d1117] text-[#e6edf3] font-mono text-xs sm:text-[13px] shadow-lg">
      
      {/* ── Studio Titlebar ── */}
      <div className="bg-[#161b22] px-4 py-2.5 border-b border-[#30363d] flex flex-wrap items-center justify-between gap-3">
        <div className="flex items-center gap-3">
          <div className="flex items-center gap-1.5 border-r border-[#30363d] pr-3">
            <div className="w-2.5 h-2.5 rounded-full bg-[#ff5f56]" />
            <div className="w-2.5 h-2.5 rounded-full bg-[#ffbd2e]" />
            <div className="w-2.5 h-2.5 rounded-full bg-[#27c93f]" />
          </div>
          <span className="font-bold text-white tracking-wide">LLFPL Studio</span>
        </div>
        
        <div className="flex bg-[#21262d] p-0.5 rounded border border-[#30363d]">
          <button
            onClick={() => setActiveTab('editor')}
            aria-pressed={activeTab === 'editor'}
            className={`px-2.5 py-1 text-xs rounded transition-colors ${
              activeTab === 'editor' ? 'bg-[#30363d] text-white font-semibold' : 'text-gray-400 hover:text-white'
            }`}
          >
            Code Editor
          </button>
          <button
            onClick={() => setActiveTab('output')}
            aria-pressed={activeTab === 'output'}
            className={`px-2.5 py-1 text-xs rounded transition-colors ${
              activeTab === 'output' ? 'bg-[#30363d] text-white font-semibold' : 'text-gray-400 hover:text-white'
            }`}
          >
            Output & Memory
          </button>
        </div>

        
      </div>

      {/* Preset Subtitle & Controls */}
      <div className="px-4 py-1.5 bg-[#161b22]/70 border-b border-[#30363d] flex flex-wrap items-center justify-between gap-2">
        <div className="demo-program-controls flex items-center gap-3 flex-wrap">

          <button
            onClick={() => {
              setActiveTab('output');
              executeCode();
            }}
            disabled={isRunning}
            className="demo-run bg-[#21262d] hover:bg-[#30363d] text-gray-200 border border-[#484f58] font-bold px-2.5 py-1 text-xs rounded flex items-center gap-1 transition-colors"
          >
            <IconPlay size={12} />
            <span>Run</span>
          </button>

          <button
            onClick={() => resetFile()}
            className="p-1.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d] transition-colors"
            title="Reset code"
            aria-label="Reset code"
          >
            <IconRotateCcw size={12} />
          </button>
        </div>
        <span className="text-[11px] text-gray-400">Run active file · temporary browser workspace</span>
      </div>

      <div className="llfpl-ide-layout">
        <aside className="llfpl-explorer" aria-label="LLFPL workspace files">
          <div className="llfpl-explorer-heading">EXPLORER <span>{Object.keys(files).length} files</span></div>
          <div className="llfpl-file-list">
            {Object.keys(files).map(name => (
              <button type="button" key={name} title={name} aria-current={activeFile === name ? 'true' : undefined}
                onClick={() => { cancelExecution(); setIsRunning(false); setActiveFile(name); setActiveTab('editor'); setLogs([]); setSlabs({}); setActiveSlabTab(''); setStats({ commits: 0, totalCost: 0, timeNs: 0 }); }}>
                <span aria-hidden="true">≡</span><span className="llfpl-filename">{name}</span>
              </button>
            ))}
          </div>
          <form onSubmit={createFile} className="llfpl-new-file">
            <label htmlFor="llfpl-new-file">New file</label>
            <input id="llfpl-new-file" value={newFileName} onChange={event => setNewFileName(event.target.value)} placeholder="example.LLFPL" maxLength={64} />
            <button type="submit">+ Create file</button>
            {fileError && <p role="alert">{fileError}</p>}
          </form>
        </aside>
        <div className="min-w-0">
      {/* ── Main Split View ── */}
      <div className="flex flex-col h-[380px] sm:h-[440px] lg:h-[480px] border-t border-[#30363d] overflow-hidden">
        
        {/* Left/Tab 1: Source Editor */}
        <div className={`flex-1 bg-[#0d1117] flex-col min-h-0 h-full ${activeTab === 'editor' ? 'flex' : 'hidden'}`}>
          <div className="flex items-center justify-between p-3 border-b border-[#21262d] text-gray-400 text-[11px] bg-[#161b22]/40 shrink-0">
            <span>{activeFile}</span>
            <span className="text-gray-500">C / WebAssembly</span>
          </div>

          <div className="flex-1 flex relative overflow-hidden group">
            {/* Line Numbers Gutter */}
            <div 
              ref={gutterRef}
              className="w-10 bg-[#090d13] border-r border-[#21262d] flex flex-col items-end py-3 pr-2 text-[#484f58] font-mono text-xs sm:text-[13px] leading-relaxed select-none overflow-hidden shrink-0"
            >
              {sourceCode.split('\n').map((_, i) => (
                <div key={i}>{i + 1}</div>
              ))}
            </div>
            
            {/* Syntax Editor */}
            <div className="flex-1 relative w-full h-full overflow-hidden bg-transparent">
              <textarea
                aria-label="LLFPL source code"
                maxLength={32768}
                value={sourceCode}
                onChange={(e) => setSourceCode(e.target.value)}
                spellCheck={false}
                wrap="off"
                onScroll={syncEditorScroll}
                className="absolute inset-0 w-full h-full p-3 bg-transparent text-transparent caret-emerald-400 resize-none outline-none font-mono text-xs sm:text-[13px] leading-relaxed dark-scrollbar overflow-auto z-10"
                style={{ color: 'transparent', whiteSpace: 'pre' }}
              />
              <pre
                ref={syntaxRef}
                className="absolute inset-0 w-full h-full p-3 bg-transparent text-[#e6edf3] font-mono text-xs sm:text-[13px] leading-relaxed overflow-hidden pointer-events-none z-0"
                style={{ whiteSpace: 'pre' }}
                aria-hidden="true"
                dangerouslySetInnerHTML={{ __html: highlightedCode + '<br/>' }}
              />
            </div>
          </div>
        </div>

        {/* Right/Tab 2: Real-time Output & Memory Slab Hex View */}
        <div aria-busy={isRunning} className={`flex-1 p-3 bg-[#090d13] flex-col min-h-0 h-full overflow-hidden ${activeTab === 'output' ? 'flex' : 'hidden'}`}>
          <div className="flex-1 flex flex-col overflow-hidden mb-2">
            <div className="flex items-center justify-between pb-1.5 mb-2 border-b border-[#21262d] shrink-0">
              <span className="text-xs font-bold text-gray-300 flex items-center gap-1.5">
                <IconTerminal size={13} className="text-emerald-400" />
                Output
              </span>
              <span className="text-[10px] text-emerald-400 font-bold">
                {stats.commits} Commits ({stats.timeNs} ns)
              </span>
            </div>

            <div className="space-y-1 text-[11px] overflow-y-auto dark-scrollbar flex-1 pr-1">
              {logs.length === 0 && (
                <div className="text-gray-500 italic mt-2">{isRunning ? 'Running program...' : 'Waiting for execution... Click "Run" to evaluate code.'}</div>
              )}
              {logs.map((log, idx) => (
                <div
                  key={idx}
                  className={`p-1.5 rounded leading-relaxed ${
                    log.type === 'commit'
                      ? 'bg-emerald-950/60 text-emerald-200 border border-emerald-800/60 font-bold'
                      : log.type === 'error'
                      ? 'bg-red-950/60 text-red-300 border border-red-800'
                      : log.type === 'summary'
                      ? 'bg-emerald-950/60 text-emerald-300 border border-emerald-800/70 font-semibold'
                      : 'text-gray-400'
                  }`}
                >
                  {log.type === 'commit' && (
                    <div className="flex items-center justify-between">
                      <span>{log.message}</span>
                      <span className="text-[9px] font-normal text-emerald-400/80">{log.timeNs} ns</span>
                    </div>
                  )}
                  {log.type !== 'commit' && log.message}
                </div>
              ))}
            </div>
          </div>

          {/* ── Memory Slab Bank Inspector (Hex & Binary64 Floats) ── */}
          {Object.keys(slabs).length > 0 && (
            <div className="pt-2 border-t border-[#21262d] shrink-0">
              <div className="flex items-center justify-between mb-2">
                <span className="text-[11px] font-bold text-gray-300 flex items-center gap-1">
                  <IconCpu size={12} className="text-emerald-400" />
                  Memory
                </span>
              </div>

              {/* Slab Selector Tabs */}
              <div className="flex gap-1 mb-2">
                {Object.keys(slabs).map(sName => (
                  <button
                    key={sName}
                    onClick={() => setActiveSlabTab(sName)}
                    className={`px-2 py-0.5 rounded text-[10px] ${
                      activeSlabTab === sName
                        ? 'bg-emerald-950 text-emerald-300 border border-emerald-700 font-bold'
                        : 'bg-[#161b22] text-gray-400 hover:text-white'
                    }`}
                  >
                    {sName}
                  </button>
                ))}
              </div>

              {/* Hex / Float table */}
              {activeSlabTab && slabs[activeSlabTab] && (
                <div className="space-y-1 bg-[#161b22] p-2 rounded border border-[#30363d] text-[10px]">
                  {Array.from(slabs[activeSlabTab].subarray(0, 4)).map((val, idx) => (
                    <div key={idx} className="flex items-center justify-between border-b border-[#21262d] pb-0.5 last:border-0">
                      <span className="text-gray-500 font-mono">+{`0x${(idx * 8).toString(16).padStart(2, '0')}`}</span>
                      <span className="text-gray-400 font-mono">{toHexFloat64(val)}</span>
                      <span className="text-emerald-300 font-bold font-mono">{val.toFixed(6)}</span>
                    </div>
                  ))}
                </div>
              )}
            </div>
          )}
        </div>
      </div>
        </div>
      </div>
    </div>
  );
});

LlfplPlayground.displayName = 'LlfplPlayground';
export default LlfplPlayground;
