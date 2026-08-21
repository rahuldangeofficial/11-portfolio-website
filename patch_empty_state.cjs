const fs = require('fs');
let code = fs.readFileSync('src/components/playgrounds/LlfplPlayground.tsx', 'utf8');

// 1. Remove the auto-execute useEffect
const useEffectRegex = /  useEffect\(\(\) => \{\n\s*executeCode\(\);\n\s*\/\/ eslint-disable-next-line react-hooks\/exhaustive-deps\n\s*\}, \[selectedPreset\]\); \/\/ Auto-run when preset changes\n/;
code = code.replace(useEffectRegex, '');

// 2. Update handlePresetChange to clear logs and slabs
const handlePresetRegex = /const handlePresetChange = \(key: string\) => \{\n\s*setSelectedPreset\(key\);\n\s*if \(PRESETS\[key\]\) \{\n\s*setSourceCode\(PRESETS\[key\]\.code\);\n\s*setActiveTab\('editor'\);\n\s*\}\n\s*\};/;
const newHandlePreset = `const handlePresetChange = (key: string) => {
    setSelectedPreset(key);
    if (PRESETS[key]) {
      setSourceCode(PRESETS[key].code);
      setActiveTab('editor');
      setLogs([]);
      setSlabs({});
      setStats({ commits: 0, totalCost: 0, timeNs: 0 });
    }
  };`;
code = code.replace(handlePresetRegex, newHandlePreset);

// 3. Add a placeholder message when logs are empty
const logsMapRegex = /\{logs\.map\(\(log, idx\) => \(/;
const newLogsMap = `{logs.length === 0 && (
                <div className="text-gray-500 italic mt-2">Waiting for execution... Click "Run" to evaluate code.</div>
              )}
              {logs.map((log, idx) => (`
code = code.replace(logsMapRegex, newLogsMap);

fs.writeFileSync('src/components/playgrounds/LlfplPlayground.tsx', code);
