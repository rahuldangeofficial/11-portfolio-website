const fs = require('fs');
let code = fs.readFileSync('src/components/playgrounds/LlfplPlayground.tsx', 'utf8');

const highlightRegex = /const highlightCode = \([\s\S]*?return h;\n\};/;
const newHighlight = `const highlightCode = (code: string) => {
  const escaped = code.replace(/</g, '&lt;').replace(/>/g, '&gt;');
  const regex = /(--.*)|(\\b\\d+(?:\\.\\d+)?\\b)|\\b(Require|Slab|Identity|Map|Commit|Loop)\\b|\\b([A-Z][a-zA-Z0-9_]*)\\b|\\b([a-z_][a-z0-9_]*)\\b|([(),])/g;
  
  return escaped.replace(regex, (match, comment, num, kw, varName, funcOrVar, sym) => {
    if (comment) return \`<span class="text-[#8b9eb7] italic">\${comment}</span>\`;
    if (num) return \`<span class="text-[#79c0ff]">\${num}</span>\`;
    if (kw) return \`<span class="text-[#ff7b72] font-bold">\${kw}</span>\`;
    if (varName) return \`<span class="text-[#ffa657]">\${varName}</span>\`;
    if (funcOrVar) return \`<span class="text-[#d2a8ff]">\${funcOrVar}</span>\`;
    if (sym) return \`<span class="text-[#c9d1d9]">\${sym}</span>\`;
    return match;
  });
};`;

code = code.replace(highlightRegex, newHighlight);
fs.writeFileSync('src/components/playgrounds/LlfplPlayground.tsx', code);
