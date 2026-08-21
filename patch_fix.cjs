const fs = require('fs');
let code = fs.readFileSync('src/components/playgrounds/LlfplPlayground.tsx', 'utf8');

// 1. Remove setActiveTab('output'); from executeCode
code = code.replace(/    setIsRunning\(true\);\n    setActiveTab\('output'\);\n/, '    setIsRunning(true);\n');

// 2. Add setActiveTab to Run button onClick
code = code.replace(
  /onClick=\{executeCode\}/,
  `onClick={() => {\n              setActiveTab('output');\n              executeCode();\n            }}`
);

// 3. Update handlePresetChange to switch back to editor tab
const handlePresetRegex = /const handlePresetChange = \(key: string\) => \{\n\s*setSelectedPreset\(key\);\n\s*if \(PRESETS\[key\]\) \{\n\s*setSourceCode\(PRESETS\[key\]\.code\);\n\s*\}\n\s*\};/;
const newHandlePreset = `const handlePresetChange = (key: string) => {
    setSelectedPreset(key);
    if (PRESETS[key]) {
      setSourceCode(PRESETS[key].code);
      setActiveTab('editor');
    }
  };`;
code = code.replace(handlePresetRegex, newHandlePreset);

// 4. Remove live evaluation from useEffect!
// We only want to execute on initial mount, or when preset changes. But wait, if we change preset, sourceCode changes, and executeCode changes.
// If we just remove executeCode from the dependency array, the linter will complain.
// Actually, let's just make it NOT run on every keystroke. 
// The easiest way is to remove the useEffect that calls executeCode entirely, and manually call executeCode() when the preset changes or on mount.
// But wait, executeCode uses state. If we call it directly, we need a ref or something.
// Let's just remove the executeCode dependency from useEffect and use a separate ref to track initial mount, or just let it be, but remove `sourceCode` from `executeCode`'s dependencies? No, it needs `sourceCode`.
// A better way: In LlfplPlayground, we only auto-run if the user hasn't typed anything recently, but actually we don't need live evaluation if they can't see both tabs!
// Let's replace the useEffect block:
const useEffectRegex = /  useEffect\(\(\) => \{\n\s*executeCode\(\);\n\s*\}, \[executeCode\]\);/;
const newUseEffect = `  // Run once on mount
  useEffect(() => {
    executeCode();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []); // Disable live evaluation on keystrokes since view is tabbed`;
code = code.replace(useEffectRegex, newUseEffect);

// And when preset changes, we should run it?
// We can just add executeCode() to handlePresetChange, but state updates are async, so sourceCode won't be updated yet.
// That's fine, the user can just click Run after changing presets, or we let the useEffect stay but only trigger on `selectedPreset` change.
const newHandlePreset2 = `const handlePresetChange = (key: string) => {
    setSelectedPreset(key);
    if (PRESETS[key]) {
      setSourceCode(PRESETS[key].code);
      setActiveTab('editor');
    }
  };

  useEffect(() => {
    executeCode();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [selectedPreset]); // Auto-run when preset changes`;
code = code.replace(newUseEffect, newHandlePreset2);

fs.writeFileSync('src/components/playgrounds/LlfplPlayground.tsx', code);
