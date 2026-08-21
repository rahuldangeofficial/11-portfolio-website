const fs = require('fs');
let code = fs.readFileSync('src/components/playgrounds/LlfplPlayground.tsx', 'utf8');

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

// Helper to convert float64 to IEEE-754 Hex string
`;

code = code.replace('// Helper to convert float64 to IEEE-754 Hex string', highlightFunc);
fs.writeFileSync('src/components/playgrounds/LlfplPlayground.tsx', code);
