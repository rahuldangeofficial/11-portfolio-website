const fs = require('fs');
let code = fs.readFileSync('src/components/playgrounds/LlfplPlayground.tsx', 'utf8');

// Replace highlightCode
const highlightRegex = /const highlightCode = \([\s\S]*?return h;\n\};/;
const newHighlight = `const highlightCode = (code: string) => {
  const escaped = code.replace(/</g, '&lt;').replace(/>/g, '&gt;');
  const regex = /(--.*)|(\\b\\d+(?:\\.\\d+)?\\b)|\\b(Require|Slab|Identity|Map|Commit|Loop)\\b|\\b([A-Z][a-zA-Z0-9_]*)\\b|\\b([a-z_][a-z0-9_]*)\\b|([(),])/g;
  
  return escaped.replace(regex, (match, comment, num, kw, varName, funcOrVar, sym) => {
    if (comment) return \`<span class="text-gray-500 italic">\${comment}</span>\`;
    if (num) return \`<span class="text-orange-300">\${num}</span>\`;
    if (kw) return \`<span class="text-pink-400 font-bold">\${kw}</span>\`;
    if (varName) return \`<span class="text-emerald-300">\${varName}</span>\`;
    if (funcOrVar) return \`<span class="text-cyan-200">\${funcOrVar}</span>\`;
    if (sym) return \`<span class="text-gray-400">\${sym}</span>\`;
    return match;
  });
};`;

code = code.replace(highlightRegex, newHighlight);

// Replace Run Button
const runBtnRegex = /className="btn btn-sm bg-emerald-600 hover:bg-emerald-500 text-white font-bold px-3 py-1 rounded flex items-center gap-1\.5 transition-colors"/;
const newRunBtn = `className="btn btn-sm bg-emerald-950/80 hover:bg-emerald-900 text-emerald-400 border border-emerald-800/80 shadow-sm font-bold px-3 py-1 rounded flex items-center gap-1.5 transition-all"`;
code = code.replace(runBtnRegex, newRunBtn);

// Replace Tabs
const tabEditorRegex = /className=\{\`px-2\.5 py-1 text-xs rounded transition-colors \\\$\{\n\s*activeTab === 'editor' \? 'bg-\\[#30363d\\] text-white shadow-sm' : 'text-gray-400 hover:text-gray-200'\n\s*\}\`\}/;
const newTabEditor = `className={\`px-2.5 py-1 text-xs rounded transition-colors \${
              activeTab === 'editor' ? 'bg-emerald-950 text-emerald-400 border border-emerald-700/50 shadow-sm font-bold' : 'text-gray-400 hover:text-gray-200'
            }\`}`;
code = code.replace(tabEditorRegex, newTabEditor);

const tabOutputRegex = /className=\{\`px-2\.5 py-1 text-xs rounded transition-colors \\\$\{\n\s*activeTab === 'output' \? 'bg-\\[#30363d\\] text-white shadow-sm' : 'text-gray-400 hover:text-gray-200'\n\s*\}\`\}/;
const newTabOutput = `className={\`px-2.5 py-1 text-xs rounded transition-colors \${
              activeTab === 'output' ? 'bg-emerald-950 text-emerald-400 border border-emerald-700/50 shadow-sm font-bold' : 'text-gray-400 hover:text-gray-200'
            }\`}`;
code = code.replace(tabOutputRegex, newTabOutput);

fs.writeFileSync('src/components/playgrounds/LlfplPlayground.tsx', code);
