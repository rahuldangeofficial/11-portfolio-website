import test from 'node:test';
import assert from 'node:assert/strict';
import { executeLlfpl } from '../src/lib/llfpl.ts';
import { PRESETS } from '../src/lib/llfpl-presets.ts';
for(const [key,preset] of Object.entries(PRESETS))test(`LLFPL preset ${key} completes`,()=>{
  const result=executeLlfpl(preset.code);
  assert.deepEqual(result.logs.filter(l=>l.type==='error'),[]);
  assert.ok(result.stats.commits>0);
  if(key==='newton_sqrt')assert.ok(Math.abs(result.slabs.State[0]-Math.sqrt(2))<1e-12);
});
test('LLFPL rejects malformed syntax, unaligned access and excessive work',()=>{
  for(const code of ['Commit(plus(1,2)','Nonsense(1)','Commit(1) trailing','Slab(A, 999999999)','Slab(A, 8) Commit(read_offset(A, 1))','Slab(A, 8) Commit(write_offset(A, 8, 1))','Map(F,x,plus(x,1)) Commit(Loop(100000000,F))','Commit(plus(1))']) {
    assert.ok(executeLlfpl(code).logs.some(l=>l.type==='error'),code);
  }
});
test('nested Identity is evaluated inside its Map, not parsed as a top-level directive',()=>{
  const r=executeLlfpl('Map(F,x,Identity(Y,plus(x,1)),multiply(Y,2)) Commit(F(3))');
  assert.deepEqual(r.logs.filter(l=>l.type==='error'),[]);
  assert.equal(r.logs.find(l=>l.type==='commit').value,8);
});

