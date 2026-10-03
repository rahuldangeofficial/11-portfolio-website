import test from 'node:test';
import assert from 'node:assert/strict';
import { executeLlfpl as runWasm } from '../src/lib/llfpl.ts';
import { PRESETS } from '../src/lib/llfpl-presets.ts';
import createLlfpl from '../public/wasm/llfpl.mjs';
const runtime = await createLlfpl();
const executeLlfpl = source => runWasm(runtime, source);
for(const [key,preset] of Object.entries(PRESETS))test(`LLFPL preset ${key} completes`,()=>{
  const result=executeLlfpl(preset.code);
  assert.deepEqual(result.logs.filter(l=>l.type==='error'),[]);
  assert.ok(result.stats.commits>0);
  if(key==='vector_dot')assert.equal(result.logs.filter(l=>l.type==='commit').at(-1).value,22);
  if(key==='packet_conditioning')assert.deepEqual(result.logs.filter(l=>l.type==='commit').slice(-4).map(l=>l.value),[100,-100,42,-99.9]);
  if(key==='pid_controller'){
    const values=result.logs.filter(l=>l.type==='commit').slice(-3).map(l=>l.value);
    [43.5,18.75,2.175].forEach((expected,i)=>assert.ok(Math.abs(values[i]-expected)<1e-10));
    assert.equal(result.slabs.PIDState[0],46.5);assert.equal(result.slabs.PIDState[1],1.5);
  }
  if(key==='newton_sqrt')assert.ok(Math.abs(result.slabs.State[0]-Math.sqrt(2))<1e-12);
});
test('LLFPL rejects malformed syntax, out-of-bounds access and oversized allocation',()=>{
  for(const code of ['Commit(plus(1,2)','Nonsense(1)','Commit(1) trailing','Slab(A, 999999999)','Slab(A, 8) Commit(write_offset(A, 8, 1))','Commit(plus(1))']) {
    assert.ok(executeLlfpl(code).logs.some(l=>l.type==='error'),code);
  }
});
test('native runtime rejects Identity used inside an expression',()=>{
  const result=executeLlfpl('Map(F,x,Identity(Y,plus(x,1)),multiply(Y,2)) Commit(F(3))');
  assert.ok(result.logs.some(log=>log.type==='error'));
});

test('real C semantics support unaligned stores and enforce fresh sessions',()=>{
  const result=executeLlfpl('Slab(A,16) Commit(write_offset(A,1,3.5)) Commit(read_offset(A,1))');
  assert.deepEqual(result.logs.filter(l=>l.type==='error'),[]);
  assert.equal(result.logs.filter(l=>l.type==='commit').at(-1).value,3.5);
  const fresh=executeLlfpl('Commit(read_offset(A,0))');
  assert.ok(fresh.logs.some(l=>l.type==='error'));
  assert.deepEqual(Object.keys(fresh.slabs),[]);
});

test('UTF-8 source limits and NUL validation are enforced before C entry',()=>{
  assert.throws(()=>executeLlfpl('--'+ 'é'.repeat(17000)),/32 KiB/);
  assert.throws(()=>executeLlfpl('Commit(1)\0Commit(2)'),/NUL/);
});
