import React, { useState, useEffect, useCallback } from 'react';
import { IconPlay, IconRotateCcw, IconTerminal, IconCpu } from '../Icons';

const PRESETS: Record<string, { name: string; tag: string; description: string; code: string }> = {
  newton_sqrt: {
    name: "Newton's Square Root (Deterministic)",
    tag: "Numerical Recurrence",
    description: "Computes sqrt(2) via Newton-Raphson iteration in a 64-byte memory slab.",
    code: `-- --------------------------------------------------
-- LLFPL: Newton-Raphson Method for Sqrt(2.0)
-- --------------------------------------------------
Require(stdlib.LLFPL)

Slab(State, 64)

Identity(Target, 2.0)
Identity(Steps, 15)
Identity(Offset, 0)

Map(InitState, x,
    write_offset(State, Offset, x)
)

Map(GetCurrent, ignored, read_offset(State, Offset))

Map(Refine, ignored,
    write_offset(State, Offset,
        divide(
            plus(GetCurrent(0), divide(Target, GetCurrent(0))),
            TWO
        )
    )
)

-- 1. Seed initial estimate with 1.0
Commit(InitState(ONE))

-- 2. Run bounded iteration loop in arena
Commit(Loop(Steps, Refine))

-- 3. Report converged root and residual error
Commit(GetCurrent(0))
Commit(minus(Square(GetCurrent(0)), Target))
`
  },
  packet_conditioning: {
    name: "Packet Saturation Filter",
    tag: "Signal Processing",
    description: "Clamps signal amplitude within saturation bounds using a 512-byte ring buffer.",
    code: `-- --------------------------------------------------
-- LLFPL: Packet Saturation Filter
-- --------------------------------------------------
Require(stdlib.LLFPL)

Slab(PacketRing, 512)
Identity(Saturation, 100.0)

Map(Condition, value,
    ClampToRange(value, Negate(Saturation), Saturation)
)

-- Populate ring buffer slots
Commit(write_offset(PacketRing, 0, 245.5))
Commit(write_offset(PacketRing, 8, -180.2))
Commit(write_offset(PacketRing, 16, 42.0))
Commit(write_offset(PacketRing, 24, -99.9))

-- Run condition filter
Commit(Condition(read_offset(PacketRing, 0)))
Commit(Condition(read_offset(PacketRing, 8)))
Commit(Condition(read_offset(PacketRing, 16)))
Commit(Condition(read_offset(PacketRing, 24)))
`
  },
  vector_dot: {
    name: "Vector Dot Product (Cache Slabs)",
    tag: "Vector Math",
    description: "Computes 3D vector dot product using memory-mapped slab offsets.",
    code: `-- --------------------------------------------------
-- LLFPL: 3D Vector Dot Product
-- A = [3.0, 4.0, 5.0], B = [2.0, -1.0, 4.0]
-- --------------------------------------------------
Require(stdlib.LLFPL)

Slab(VecA, 64)
Slab(VecB, 64)

-- Store Vector A
Commit(write_offset(VecA, 0, 3.0))
Commit(write_offset(VecA, 8, 4.0))
Commit(write_offset(VecA, 16, 5.0))

-- Store Vector B
Commit(write_offset(VecB, 0, 2.0))
Commit(write_offset(VecB, 8, -1.0))
Commit(write_offset(VecB, 16, 4.0))

Map(Dot3D, ignored,
    plus(
        multiply(read_offset(VecA, 0), read_offset(VecB, 0)),
        plus(
            multiply(read_offset(VecA, 8), read_offset(VecB, 8)),
            multiply(read_offset(VecA, 16), read_offset(VecB, 16))
        )
    )
)

Commit(Dot3D(0))
`
  },
  pid_controller: {
    name: "Closed-Loop PID Controller",
    tag: "Control Systems",
    description: "Proportional-integral-derivative control step simulation with state preserved across cycles.",
    code: `-- --------------------------------------------------
-- LLFPL: Discrete PID Controller Step
-- --------------------------------------------------
Require(stdlib.LLFPL)

Slab(PIDState, 64)
Identity(Kp, 1.25)
Identity(Ki, 0.05)
Identity(Kd, 0.15)
Identity(Setpoint, 50.0)

Identity(OFFSET_INT, 0)
Identity(OFFSET_PREV, 8)

Map(InitPID, val,
    plus(
        write_offset(PIDState, OFFSET_INT, 0.0),
        write_offset(PIDState, OFFSET_PREV, 0.0)
    )
)

Map(ComputeControl, current,
    Identity(Err, minus(Setpoint, current)),
    write_offset(PIDState, OFFSET_INT, plus(read_offset(PIDState, OFFSET_INT), Err)),
    plus(
        multiply(Kp, Err),
        plus(
            multiply(Ki, read_offset(PIDState, OFFSET_INT)),
            multiply(Kd, minus(Err, read_offset(PIDState, OFFSET_PREV)))
        )
    )
)

Commit(InitPID(0))
Commit(ComputeControl(20.0))
Commit(ComputeControl(35.0))
Commit(ComputeControl(48.5))
`
  }
};

const STDLIB_CONSTANTS: Record<string, number> = {
  PI: 3.141592653589793,
  E: 2.718281828459045,
  SQRT_TWO: 1.4142135623730951,
  EPSILON: 2.220446049250313e-16,
  ZERO: 0.0,
  ONE: 1.0,
  TWO: 2.0
};

interface ExecutionLog {
  type: 'trace' | 'commit' | 'error' | 'summary';
  message: string;
  value?: number;
  cost?: number;
  timeNs?: number;
}

export const LlfplPlayground: React.FC = React.memo(() => {
  const [selectedPreset, setSelectedPreset] = useState<string>('newton_sqrt');
  const [sourceCode, setSourceCode] = useState<string>(PRESETS['newton_sqrt'].code);
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

  const handlePresetChange = (key: string) => {
    setSelectedPreset(key);
    if (PRESETS[key]) {
      setSourceCode(PRESETS[key].code);
      setActiveTab('editor');
      setLogs([]);
      setSlabs({});
      setStats({ commits: 0, totalCost: 0, timeNs: 0 });
    }
  };

  const executeCode = useCallback(() => {
    setIsRunning(true);
    const newLogs: ExecutionLog[] = [];
    const newSlabs: Record<string, Float64Array> = {};
    const identities: Record<string, number> = { ...STDLIB_CONSTANTS };
    const templates: Record<string, { params: string[]; body: string[] }> = {};

    let totalReductionCost = 0;
    let commitCount = 0;
    const startTime = performance.now();

    try {
      newLogs.push({
        type: 'trace',
        message: 'Runtime: IEEE-754 binary64 | 64B slab alignment'
      });

      const cleanLines = sourceCode
        .split('\n')
        .map(l => l.replace(/--.*$/, '').trim())
        .filter(l => l.length > 0);

      const fullScript = cleanLines.join(' ');

      const directiveRegex = /\b(Require|Slab|Identity|Map|Commit)\s*\(/g;
      let match;
      const directives: { name: string; content: string }[] = [];

      while ((match = directiveRegex.exec(fullScript)) !== null) {
        const dName = match[1];
        const startPos = match.index + match[0].length;
        let depth = 1;
        let pos = startPos;

        while (pos < fullScript.length && depth > 0) {
          if (fullScript[pos] === '(') depth++;
          else if (fullScript[pos] === ')') depth--;
          pos++;
        }

        if (depth === 0) {
          directives.push({
            name: dName,
            content: fullScript.substring(startPos, pos - 1).trim()
          });
        }
      }

      const evaluateExpr = (expr: string, scope: Record<string, number>): number => {
        totalReductionCost++;
        expr = expr.trim();

        if (!isNaN(Number(expr))) {
          return Number(expr);
        }

        if (scope[expr] !== undefined) return scope[expr];
        if (identities[expr] !== undefined) return identities[expr];

        const callMatch = expr.match(/^([a-zA-Z0-9_]+)\s*\((.*)\)$/);
        if (!callMatch) {
          throw new Error(`Unresolved symbol or invalid syntax: '${expr}'`);
        }

        const fnName = callMatch[1];
        const rawArgs = callMatch[2];

        const args: string[] = [];
        let cur = '';
        let d = 0;
        for (let i = 0; i < rawArgs.length; i++) {
          const char = rawArgs[i];
          if (char === '(') d++;
          else if (char === ')') d--;

          if (char === ',' && d === 0) {
            args.push(cur.trim());
            cur = '';
          } else {
            cur += char;
          }
        }
        if (cur.trim().length > 0) args.push(cur.trim());

        switch (fnName) {
          case 'Identity': {
            const idName = args[0];
            const val = evaluateExpr(args[1], scope);
            scope[idName] = val;
            return val;
          }
          case 'plus':
            return evaluateExpr(args[0], scope) + evaluateExpr(args[1], scope);
          case 'minus':
            return evaluateExpr(args[0], scope) - evaluateExpr(args[1], scope);
          case 'multiply':
            return evaluateExpr(args[0], scope) * evaluateExpr(args[1], scope);
          case 'divide': {
            const num = evaluateExpr(args[0], scope);
            const denom = evaluateExpr(args[1], scope);
            return num / denom;
          }
          case 'modulo':
            return evaluateExpr(args[0], scope) % evaluateExpr(args[1], scope);
          case 'Absolute':
            return Math.abs(evaluateExpr(args[0], scope));
          case 'Negate':
            return -evaluateExpr(args[0], scope);
          case 'Square': {
            const v = evaluateExpr(args[0], scope);
            return v * v;
          }
          case 'SquareRoot':
            return Math.sqrt(evaluateExpr(args[0], scope));
          case 'Branch': {
            const sel = evaluateExpr(args[0], scope);
            const consequent = evaluateExpr(args[1], scope);
            const alternative = evaluateExpr(args[2], scope);
            return sel * consequent + (1.0 - sel) * alternative;
          }
          case 'ClampToRange': {
            const val = evaluateExpr(args[0], scope);
            const min = evaluateExpr(args[1], scope);
            const max = evaluateExpr(args[2], scope);
            return Math.min(Math.max(val, min), max);
          }
          case 'NearlyEqual': {
            const a = evaluateExpr(args[0], scope);
            const b = evaluateExpr(args[1], scope);
            const tol = evaluateExpr(args[2], scope);
            return Math.abs(a - b) <= tol ? 1.0 : 0.0;
          }
          case 'write_offset': {
            const slabName = args[0].trim();
            const byteOffset = Math.floor(evaluateExpr(args[1], scope));
            const val = evaluateExpr(args[2], scope);

            if (!newSlabs[slabName]) {
              throw new Error(`Memory slab '${slabName}' not declared`);
            }
            const floatIdx = Math.floor(byteOffset / 8);
            if (floatIdx < 0 || floatIdx >= newSlabs[slabName].length) {
              throw new Error(`Out of bounds write at offset ${byteOffset} in slab '${slabName}'`);
            }
            newSlabs[slabName][floatIdx] = val;
            return val;
          }
          case 'read_offset': {
            const slabName = args[0].trim();
            const byteOffset = Math.floor(evaluateExpr(args[1], scope));

            if (!newSlabs[slabName]) {
              throw new Error(`Memory slab '${slabName}' not declared`);
            }
            const floatIdx = Math.floor(byteOffset / 8);
            if (floatIdx < 0 || floatIdx >= newSlabs[slabName].length) {
              throw new Error(`Out of bounds read at offset ${byteOffset} in slab '${slabName}'`);
            }
            const val = newSlabs[slabName][floatIdx];
            return val !== undefined ? val : 0.0;
          }
          case 'Loop': {
            const count = Math.floor(evaluateExpr(args[0], scope));
            const templateName = args[1].trim();
            const tpl = templates[templateName];
            if (!tpl) throw new Error(`Template '${templateName}' not defined for Loop`);

            let lastRes = 0;
            for (let iter = 0; iter < count; iter++) {
              const loopScope = { ...scope, [tpl.params[0] || 'iter']: iter };
              for (const stmt of tpl.body) {
                lastRes = evaluateExpr(stmt, loopScope);
              }
            }
            return lastRes;
          }
          default: {
            if (templates[fnName]) {
              const tpl = templates[fnName];
              const callScope: Record<string, number> = { ...scope };
              tpl.params.forEach((param, idx) => {
                if (args[idx] !== undefined) {
                  callScope[param] = evaluateExpr(args[idx], scope);
                }
              });
              let res = 0;
              for (const stmt of tpl.body) {
                res = evaluateExpr(stmt, callScope);
              }
              return res;
            }
            throw new Error(`Unknown primitive or form '${fnName}'`);
          }
        }
      };

      for (const d of directives) {
        if (d.name === 'Require') {
          newLogs.push({ type: 'trace', message: `module load: '${d.content}'` });
        } else if (d.name === 'Slab') {
          const parts = d.content.split(',').map(s => s.trim());
          const sName = parts[0];
          const byteCap = parseInt(parts[1], 10) || 64;
          const floatCount = Math.max(1, Math.ceil(byteCap / 8));
          newSlabs[sName] = new Float64Array(floatCount);
          newLogs.push({ type: 'trace', message: `slab '${sName}': reserved ${byteCap} bytes (${floatCount} binary64 slots, 64B aligned)` });
        } else if (d.name === 'Identity') {
          const firstComma = d.content.indexOf(',');
          if (firstComma !== -1) {
            const idName = d.content.substring(0, firstComma).trim();
            const valExpr = d.content.substring(firstComma + 1).trim();
            const val = evaluateExpr(valExpr, {});
            identities[idName] = val;
            newLogs.push({ type: 'trace', message: `bound immutable identity '${idName}' = ${val}` });
          }
        } else if (d.name === 'Map') {
          const args: string[] = [];
          let cur = '';
          let d_map = 0;
          for (let i = 0; i < d.content.length; i++) {
            const char = d.content[i];
            if (char === '(') d_map++;
            else if (char === ')') d_map--;
            if (char === ',' && d_map === 0) {
              args.push(cur.trim());
              cur = '';
            } else {
              cur += char;
            }
          }
          if (cur.trim().length > 0) args.push(cur.trim());

          const mapName = args[0];
          const params = [args[1]];
          const body = args.slice(2);
          templates[mapName] = { params, body };
          newLogs.push({ type: 'trace', message: `declared template '${mapName}' (${params.length} parameters)` });
        } else if (d.name === 'Commit') {
          commitCount++;
          const cStart = performance.now();
          const result = evaluateExpr(d.content, {});
          const cElapsed = Math.max(12, Math.round((performance.now() - cStart) * 1000000));

          newLogs.push({
            type: 'commit',
            message: `Commit ${commitCount} => ${result}`,
            value: result,
            timeNs: cElapsed,
            cost: totalReductionCost
          });
        }
      }

      const totalElapsedNs = Math.round((performance.now() - startTime) * 1000000);
      setStats({
        commits: commitCount,
        totalCost: totalReductionCost,
        timeNs: totalElapsedNs
      });

      newLogs.push({
        type: 'summary',
        message: `Completed: ${commitCount} commit(s), ${totalReductionCost} reductions, ${totalElapsedNs} ns`
      });

    } catch (err: unknown) {
      const errMsg = err instanceof Error ? err.message : String(err);
      newLogs.push({
        type: 'error',
        message: `Runtime Diagnostic: ${errMsg}`
      });
    }

    setSlabs({ ...newSlabs });
    const slabNames = Object.keys(newSlabs);
    if (slabNames.length > 0) {
      setActiveSlabTab(prev => prev || slabNames[0]);
    }
    setLogs(newLogs);
    setIsRunning(false);
  }, [sourceCode]);


  const highlightCode = (code: string) => {
  const escaped = code.replace(/</g, '&lt;').replace(/>/g, '&gt;');
  const regex = /(--.*)|(\b\d+(?:\.\d+)?\b)|\b(Require|Slab|Identity|Map|Commit|Loop)\b|\b([A-Z][a-zA-Z0-9_]*)\b|\b([a-z_][a-z0-9_]*)\b|([(),])/g;
  
  return escaped.replace(regex, (match, comment, num, kw, varName, funcOrVar, sym) => {
    if (comment) return `<span class="text-gray-500 italic">${comment}</span>`;
    if (num) return `<span class="text-orange-300">${num}</span>`;
    if (kw) return `<span class="text-pink-400 font-bold">${kw}</span>`;
    if (varName) return `<span class="text-emerald-300">${varName}</span>`;
    if (funcOrVar) return `<span class="text-cyan-200">${funcOrVar}</span>`;
    if (sym) return `<span class="text-gray-400">${sym}</span>`;
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

  return (
    <div className="border border-[var(--border-default)] rounded-lg overflow-hidden bg-[#0d1117] text-[#e6edf3] font-mono text-xs sm:text-[13px] shadow-lg">
      
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
            className={`px-2.5 py-1 text-xs rounded transition-colors ${
              activeTab === 'editor' ? 'bg-emerald-700 text-white font-bold' : 'text-gray-400 hover:text-white'
            }`}
          >
            Code Editor
          </button>
          <button
            onClick={() => setActiveTab('output')}
            className={`px-2.5 py-1 text-xs rounded transition-colors ${
              activeTab === 'output' ? 'bg-emerald-700 text-white font-bold' : 'text-gray-400 hover:text-white'
            }`}
          >
            Terminal & Memory
          </button>
        </div>

        
      </div>

      {/* Preset Subtitle & Controls */}
      <div className="px-4 py-1.5 bg-[#161b22]/70 border-b border-[#30363d] flex flex-wrap items-center justify-between gap-2">
        <div className="flex items-center gap-2 flex-wrap">
          <select
            value={selectedPreset}
            onChange={(e) => handlePresetChange(e.target.value)}
            className="bg-[#21262d] text-white text-xs border border-[#30363d] rounded px-2.5 py-1 focus:outline-none focus:border-emerald-500 cursor-pointer"
          >
            {Object.entries(PRESETS).map(([key, item]) => (
              <option key={key} value={key}>
                {item.name}
              </option>
            ))}
          </select>

          <button
            onClick={() => {
              setActiveTab('output');
              executeCode();
            }}
            disabled={isRunning}
            className="bg-emerald-950/80 hover:bg-emerald-900 text-emerald-400 border border-emerald-800/80 shadow-sm font-bold px-2.5 py-1 text-xs rounded flex items-center gap-1 transition-all"
          >
            <IconPlay size={12} />
            <span>{isRunning ? 'Evaluating...' : 'Run'}</span>
          </button>

          <button
            onClick={() => handlePresetChange(selectedPreset)}
            className="p-1.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d] transition-colors"
            title="Reset code"
          >
            <IconRotateCcw size={12} />
          </button>
        </div>
        <div className="flex items-center gap-2 text-[11px] text-gray-400 hidden lg:flex">
          <span>{PRESETS[selectedPreset]?.description}</span>
          <span className="text-emerald-400 font-bold">[{PRESETS[selectedPreset]?.tag}]</span>
        </div>
      </div>

      {/* ── Main Split View ── */}
      <div className="flex flex-col h-[320px] sm:h-[350px] lg:h-[400px] border-t border-[#30363d] overflow-hidden">
        
        {/* Left/Tab 1: Source Editor */}
        <div className={`flex-1 bg-[#0d1117] flex-col min-h-0 h-full ${activeTab === 'editor' ? 'flex' : 'hidden'}`}>
          <div className="flex items-center justify-between p-3 border-b border-[#21262d] text-gray-400 text-[11px] bg-[#161b22]/40 shrink-0">
            <span>program.LLFPL</span>
            <span className="text-gray-500">AST-less evaluation</span>
          </div>

          <div className="flex-1 flex relative overflow-hidden group">
            {/* Line Numbers Gutter */}
            <div 
              id="llfpl-gutter"
              className="w-10 bg-[#090d13] border-r border-[#21262d] flex flex-col items-end py-3 pr-2 text-[#484f58] font-mono text-xs sm:text-[13px] leading-relaxed select-none overflow-hidden shrink-0"
            >
              {sourceCode.split('\n').map((_, i) => (
                <div key={i}>{i + 1}</div>
              ))}
            </div>
            
            {/* Syntax Editor */}
            <div className="flex-1 relative w-full h-full overflow-hidden bg-transparent">
              <textarea
                value={sourceCode}
                onChange={(e) => setSourceCode(e.target.value)}
                spellCheck={false}
                wrap="off"
                onScroll={(e) => {
                  const gutter = document.getElementById('llfpl-gutter');
                  const pre = document.getElementById('llfpl-syntax-pre');
                  if (gutter) gutter.scrollTop = (e.target as HTMLTextAreaElement).scrollTop;
                  if (pre) {
                    pre.scrollTop = (e.target as HTMLTextAreaElement).scrollTop;
                    pre.scrollLeft = (e.target as HTMLTextAreaElement).scrollLeft;
                  }
                }}
                className="absolute inset-0 w-full h-full p-3 bg-transparent text-transparent caret-emerald-400 resize-none outline-none font-mono text-xs sm:text-[13px] leading-relaxed dark-scrollbar overflow-auto z-10"
                style={{ color: 'transparent', whiteSpace: 'pre' }}
              />
              <pre
                id="llfpl-syntax-pre"
                className="absolute inset-0 w-full h-full p-3 bg-transparent text-[#e6edf3] font-mono text-xs sm:text-[13px] leading-relaxed overflow-hidden pointer-events-none z-0"
                style={{ whiteSpace: 'pre' }}
                aria-hidden="true"
                dangerouslySetInnerHTML={{ __html: highlightCode(sourceCode) + '<br/>' }}
              />
            </div>
          </div>
        </div>

        {/* Right/Tab 2: Real-time Output & Memory Slab Hex View */}
        <div className={`flex-1 p-3 bg-[#090d13] flex-col min-h-0 h-full overflow-hidden ${activeTab === 'output' ? 'flex' : 'hidden'}`}>
          <div className="flex-1 flex flex-col overflow-hidden mb-2">
            <div className="flex items-center justify-between pb-1.5 mb-2 border-b border-[#21262d] shrink-0">
              <span className="text-xs font-bold text-gray-300 flex items-center gap-1.5">
                <IconTerminal size={13} className="text-emerald-400" />
                Execution Trace &amp; Output
              </span>
              <span className="text-[10px] text-emerald-400 font-bold">
                {stats.commits} Commits ({stats.timeNs} ns)
              </span>
            </div>

            <div className="space-y-1 text-[11px] overflow-y-auto dark-scrollbar flex-1 pr-1">
              {logs.length === 0 && (
                <div className="text-gray-500 italic mt-2">Waiting for execution... Click "Run" to evaluate code.</div>
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
                  Memory Slab Inspector
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
                  {Array.from(slabs[activeSlabTab]).slice(0, 4).map((val, idx) => (
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
  );
});

LlfplPlayground.displayName = 'LlfplPlayground';
export default LlfplPlayground;

LlfplPlayground.displayName = 'LlfplPlayground';
