import test from 'node:test';
import assert from 'node:assert/strict';
import createMimfs from '../public/wasm/mimfs-threaded.mjs';
import {runNativeCommand} from '../src/lib/mimfs.ts';

test('real WASM pthreads append 400 complete, unique records to one file', async () => {
  const module=await createMimfs();
  assert.equal(module.ccall('shell_init','number',[],[]),0);
  const result=runNativeCommand(module,'concurrency');
  assert.ok(!result.lines.some(l=>l.type==='error'),JSON.stringify(result));
  assert.match(result.lines.at(-1).text,/4 real POSIX threads/);
  const content=runNativeCommand(module,'cat /concurrency-1.log').lines.at(-1).text;
  const records=content.trim().split('\n');
  assert.equal(records.length,400);
  assert.equal(new Set(records).size,400);
  for(let thread=0;thread<4;thread++) for(let record=0;record<100;record++) {
    assert.ok(records.includes(`worker ${thread} record ${String(record).padStart(3,'0')}`));
  }
});
