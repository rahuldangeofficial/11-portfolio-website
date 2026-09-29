const STDLIB_CONSTANTS: Record<string, number> = {
  PI: 3.141592653589793,
  E: 2.718281828459045,
  SQRT_TWO: 1.4142135623730951,
  EPSILON: 2.220446049250313e-16,
  ZERO: 0.0,
  ONE: 1.0,
  TWO: 2.0
};

export interface ExecutionLog {
  type: 'trace' | 'commit' | 'error' | 'summary';
  message: string;
  value?: number;
  cost?: number;
  timeNs?: number;
}

export function executeLlfpl(sourceCode: string) {
    const newLogs: ExecutionLog[] = [];
    const newSlabs: Record<string, Float64Array> = Object.create(null);
    const identities: Record<string, number> = { ...STDLIB_CONSTANTS };
    const templates: Record<string, { params: string[]; body: string[] }> = Object.create(null);

    let totalReductionCost = 0;
    let commitCount = 0;
    const startTime = performance.now();

    try {
      if (sourceCode.length > 32768) throw new Error('Source exceeds the 32 KiB browser limit.');
      newLogs.push({
        type: 'trace',
        message: 'Runtime: JavaScript simulation | Float64Array memory | browser timings only'
      });

      const cleanLines = sourceCode
        .split('\n')
        .map(l => l.replace(/--.*$/, '').trim())
        .filter(l => l.length > 0);

      const fullScript = cleanLines.join(' ');

      const directiveRegex = /\b(Require|Slab|Identity|Map|Commit)\s*\(/g;
      let match;
      let consumed = 0;
      const directives: { name: string; content: string }[] = [];

      while ((match = directiveRegex.exec(fullScript)) !== null) {
        if (fullScript.slice(consumed, match.index).trim()) throw new Error('Unsupported directive or invalid syntax.');
        const dName = match[1];
        const startPos = match.index + match[0].length;
        let depth = 1;
        let pos = startPos;

        while (pos < fullScript.length && depth > 0) {
          if (fullScript[pos] === '(') depth++;
          else if (fullScript[pos] === ')') depth--;
          pos++;
        }

        if (depth !== 0) throw new Error('Unclosed directive parentheses.');
        consumed = pos;
        directiveRegex.lastIndex = pos;
        if (depth === 0) {
          directives.push({
            name: dName,
            content: fullScript.substring(startPos, pos - 1).trim()
          });
        }
      }

      if (fullScript.slice(consumed).trim()) throw new Error('Unsupported directive or invalid trailing syntax.');
      const evaluateExpr = (expr: string, scope: Record<string, number>): number => {
        totalReductionCost++;
        if (totalReductionCost > 100000 || performance.now() - startTime > 500) throw new Error('Browser execution limit reached. Reduce the workload or use the native runtime.');
        if (typeof expr !== 'string' || !expr.trim()) throw new Error('Missing expression or argument.');
        expr = expr.trim();

        if (!isNaN(Number(expr))) {
          return Number(expr);
        }

        if (Object.prototype.hasOwnProperty.call(scope, expr)) return scope[expr];
        if (Object.prototype.hasOwnProperty.call(identities, expr)) return identities[expr];

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
            const byteOffset = evaluateExpr(args[1], scope);
            if (!Number.isSafeInteger(byteOffset) || byteOffset < 0 || byteOffset % 8 !== 0) throw new Error('Memory offsets must be nonnegative multiples of 8.');
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
            const byteOffset = evaluateExpr(args[1], scope);
            if (!Number.isSafeInteger(byteOffset) || byteOffset < 0 || byteOffset % 8 !== 0) throw new Error('Memory offsets must be nonnegative multiples of 8.');

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
            const count = evaluateExpr(args[0], scope);
            if (!Number.isSafeInteger(count) || count < 0 || count > 10000) throw new Error('Loop count must be an integer from 0 to 10000.');
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
          const byteCap = Number(parts[1]);
          if (!Number.isSafeInteger(byteCap) || byteCap < 8 || byteCap % 8 !== 0 || byteCap > 65536) throw new Error('Slab size must be a multiple of 8, between 8 and 65536 bytes.');
          if (Object.keys(newSlabs).length >= 16 || newSlabs[sName]) throw new Error('Duplicate slab or maximum of 16 slabs reached.');
          const floatCount = Math.max(1, Math.ceil(byteCap / 8));
          newSlabs[sName] = new Float64Array(floatCount);
          newLogs.push({ type: 'trace', message: `slab '${sName}': reserved ${byteCap} bytes (${floatCount} binary64 slots; native alignment not simulated)` });
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
          const cElapsed = Math.round((performance.now() - cStart) * 1000000);

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

    return { logs: newLogs, slabs: newSlabs, stats: { commits: commitCount, totalCost: totalReductionCost, timeNs: Math.round((performance.now() - startTime) * 1000000) } };
}
