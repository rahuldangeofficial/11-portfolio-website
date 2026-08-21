const fs = require('fs');
let code = fs.readFileSync('src/components/playgrounds/LlfplPlayground.tsx', 'utf8');

// Replace the duplicate definition with just the useEffect
const duplicateRegex = /const handlePresetChange = \(key: string\) => \{\n\s*setSelectedPreset\(key\);\n\s*if \(PRESETS\[key\]\) \{\n\s*setSourceCode\(PRESETS\[key\]\.code\);\n\s*setActiveTab\('editor'\);\n\s*\}\n\s*\};\n\n\s*useEffect\(\(\) => \{\n\s*executeCode\(\);\n\s*\/\/ eslint-disable-next-line react-hooks\/exhaustive-deps\n\s*\}, \[selectedPreset\]\); \/\/ Auto-run when preset changes/;

const newUseEffect = `useEffect(() => {
    executeCode();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [selectedPreset]); // Auto-run when preset changes`;

code = code.replace(duplicateRegex, newUseEffect);

fs.writeFileSync('src/components/playgrounds/LlfplPlayground.tsx', code);
