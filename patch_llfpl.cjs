const fs = require('fs');
let code = fs.readFileSync('src/components/playgrounds/LlfplPlayground.tsx', 'utf8');

// 1. Add activeTab state
code = code.replace(
  "const [activeSlabTab, setActiveSlabTab] = useState<string | null>(null);",
  "const [activeSlabTab, setActiveSlabTab] = useState<string | null>(null);\n  const [activeTab, setActiveTab] = useState<'editor' | 'output'>('editor');"
);

// 2. Add Tab bar to the top header (like MIMFS)
const headerRegex = /<span className="font-bold text-white tracking-wide">LLFPL Studio<\/span>\s*<span className="px-2 py-0\.5 rounded text-\[10px\] bg-\[#21262d\] text-emerald-400 border border-\[#30363d\] flex items-center gap-1\.5">\s*<span className="w-1\.5 h-1\.5 rounded-full bg-emerald-400 animate-pulse" \/>\s*DETERMINISTIC REPL\s*<\/span>\s*<\/div>/;

const newHeader = `<span className="font-bold text-white tracking-wide">LLFPL Studio</span>
        </div>
        
        <div className="flex bg-[#21262d] p-0.5 rounded border border-[#30363d]">
          <button
            onClick={() => setActiveTab('editor')}
            className={\`px-2.5 py-1 text-xs rounded transition-colors \${
              activeTab === 'editor' ? 'bg-[#30363d] text-white shadow-sm' : 'text-gray-400 hover:text-gray-200'
            }\`}
          >
            Code Editor
          </button>
          <button
            onClick={() => setActiveTab('output')}
            className={\`px-2.5 py-1 text-xs rounded transition-colors \${
              activeTab === 'output' ? 'bg-[#30363d] text-white shadow-sm' : 'text-gray-400 hover:text-gray-200'
            }\`}
          >
            Terminal & Memory
          </button>
        </div>`;

code = code.replace(headerRegex, newHeader);

// 3. Add highlightCode function
const helperRegex = /const toHexFloat64 = \(val: number\) => {/;
const highlightFunc = `const highlightCode = (code: string) => {
  let h = code.replace(/</g, '&lt;').replace(/>/g, '&gt;');
  
  // Numbers
  h = h.replace(/\\b(\\d+(\\.\\d+)?|0x[0-9a-fA-F]+)\\b/g, '<span class="text-orange-300">$1</span>');
  
  // Types / Registers
  const types = ['bank', 'f64', 'i64', 'r0', 'r1', 'r2', 'r3', 'r4', 'r5'];
  h = h.replace(new RegExp(\`\\\\b(\${types.join('|')})\\\\b\`, 'g'), '<span class="text-cyan-300 font-bold">$1</span>');
  
  // Keywords
  const keywords = ['reg', 'fn', 'eval', 'ret', 'jmp', 'while', 'if', 'else', 'alloc'];
  h = h.replace(new RegExp(\`\\\\b(\${keywords.join('|')})\\\\b\`, 'g'), '<span class="text-pink-400 font-bold">$1</span>');
  
  // Comments (must be last to not highlight words inside comments)
  h = h.replace(/(\\/\\/.*)/g, '<span class="text-gray-500">$1</span>');
  
  return h;
};

const toHexFloat64 = (val: number) => {`;
code = code.replace(helperRegex, highlightFunc);

// 4. Update the split view to conditional rendering and syntax highlighter
const splitViewRegex = /<div className="flex flex-col lg:flex-row divide-y lg:divide-y-0 lg:divide-x divide-\[#30363d\] h-auto lg:h-\[400px\]">[\s\S]*?<\/div>\s*<\/div>\s*<\/div>\s*\);\s*\}\);/;

const newSplitView = `<div className="flex flex-col h-[400px] border-t border-[#30363d] overflow-hidden">
        
        {/* Left/Tab 1: Source Editor */}
        <div className={\`flex-1 bg-[#0d1117] flex-col min-h-0 h-full \${activeTab === 'editor' ? 'flex' : 'hidden'}\`}>
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
              {sourceCode.split('\\n').map((_, i) => (
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
        <div className={\`flex-1 p-3 bg-[#090d13] flex-col min-h-0 h-full overflow-hidden \${activeTab === 'output' ? 'flex' : 'hidden'}\`}>
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
              {logs.map((log, idx) => (
                <div
                  key={idx}
                  className={\`p-1.5 rounded leading-relaxed \${
                    log.type === 'commit'
                      ? 'bg-emerald-950/60 text-emerald-200 border border-emerald-800/60 font-bold'
                      : log.type === 'error'
                      ? 'bg-red-950/60 text-red-300 border border-red-800'
                      : log.type === 'summary'
                      ? 'bg-emerald-950/60 text-emerald-300 border border-emerald-800/70 font-semibold'
                      : 'text-gray-400'
                  }\`}
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
                    className={\`px-2 py-0.5 rounded text-[10px] \${
                      activeSlabTab === sName
                        ? 'bg-emerald-950 text-emerald-300 border border-emerald-700 font-bold'
                        : 'bg-[#161b22] text-gray-400 hover:text-white'
                    }\`}
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
                      <span className="text-gray-500 font-mono">+{\`0x\${(idx * 8).toString(16).padStart(2, '0')}\`}</span>
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
export default LlfplPlayground;`;
code = code.replace(splitViewRegex, newSplitView);

fs.writeFileSync('src/components/playgrounds/LlfplPlayground.tsx', code);
