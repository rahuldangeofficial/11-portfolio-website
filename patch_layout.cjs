const fs = require('fs');
let code = fs.readFileSync('src/components/playgrounds/LlfplPlayground.tsx', 'utf8');

// 1. Remove the Controls from the Top Header
const topHeaderControlsRegex = /<div className="flex items-center gap-2 flex-wrap">\s*<select[\s\S]*?<\/button>\s*<\/div>/;
const controlsMatch = code.match(topHeaderControlsRegex);
const controlsHTML = controlsMatch ? controlsMatch[0] : '';
code = code.replace(topHeaderControlsRegex, '');

// 2. Modify the Controls to remove .btn class and make them perfectly rectangular/crisp
let newControlsHTML = controlsHTML.replace(
  /className="btn btn-sm bg-emerald-950\/80 hover:bg-emerald-900 text-emerald-400 border border-emerald-800\/80 shadow-sm font-bold px-3 py-1 rounded flex items-center gap-1\.5 transition-all"/,
  'className="bg-emerald-950/80 hover:bg-emerald-900 text-emerald-400 border border-emerald-800/80 shadow-sm font-bold px-2.5 py-1 text-xs rounded flex items-center gap-1 transition-all"'
);
newControlsHTML = newControlsHTML.replace(
  /className="p-1 rounded bg-\[#21262d\] text-gray-300 hover:text-white border border-\[#30363d\]"/,
  'className="p-1.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d] transition-colors"'
);

// 3. Inject Controls into the Secondary Header
const secondaryHeaderRegex = /\{\/\* Preset Subtitle \*\/\}\s*<div className="px-4 py-1\.5 bg-\[#161b22\]\/70 border-b border-\[#30363d\] text-\[11px\] text-gray-400 flex items-center justify-between">\s*<span>\{PRESETS\[selectedPreset\]\?\.description\}<\/span>\s*<span className="text-emerald-400 font-bold hidden sm:inline">\{PRESETS\[selectedPreset\]\?\.tag\}<\/span>\s*<\/div>/;

const newSecondaryHeader = `{/* Preset Subtitle & Controls */}
      <div className="px-4 py-1.5 bg-[#161b22]/70 border-b border-[#30363d] flex flex-wrap items-center justify-between gap-2">
        \${newControlsHTML}
        <div className="flex items-center gap-2 text-[11px] text-gray-400 hidden lg:flex">
          <span>{PRESETS[selectedPreset]?.description}</span>
          <span className="text-emerald-400 font-bold">[{PRESETS[selectedPreset]?.tag}]</span>
        </div>
      </div>`;

code = code.replace(secondaryHeaderRegex, newSecondaryHeader.replace('${newControlsHTML}', newControlsHTML));

fs.writeFileSync('src/components/playgrounds/LlfplPlayground.tsx', code);
