import test from 'node:test';
import assert from 'node:assert/strict';
import createMimfs from '../public/wasm/mimfs.mjs';
import {runNativeCommand} from '../src/lib/mimfs.ts';
const module = await createMimfs();
const reset = () => assert.equal(module.ccall('shell_init','number',[],[]),0);
const snapshot = () => JSON.parse(module.ccall('shell_snapshot','string',[],[]));
const run = command => runNativeCommand(module,command);
const ok = command => { const r=run(command); assert.ok(!r.lines.some(l=>l.type==='error'), JSON.stringify(r)); return r.lines.at(-1)?.text; };

test('real WASM volume, UTF-8 transfers, hard links, symlinks and atomic rename', () => {
  reset();
  assert.ok(snapshot().totalBlocks>1000);
  ok('mkdir var');ok('write "var/file name" "café"');
  assert.equal(ok('cat "var/file name"'),'café');
  assert.match(ok('stat "var/file name"'),/Size: 5 bytes/);
  ok('ln "var/file name" alias');ok('ln -s /alias symbolic');
  ok('rm "var/file name"');assert.equal(ok('cat symbolic'),'café');
  ok('write replacement replaced');ok('mv replacement alias');
  assert.equal(ok('cat symbolic'),'replaced');
  ok('touch alias');assert.equal(ok('cat alias'),'replaced');
  ok('append alias " café"');assert.equal(ok('cat alias'),'replaced café');
});

test('real WASM sparse files, directory errors, cycles and allocation accounting', () => {
  reset();const initial=snapshot();
  ok('write file x');const before=snapshot();
  ok('truncate file 1048576');assert.equal(snapshot().freeBlocks,before.freeBlocks);
  assert.match(ok('stat file'),/Size: 1048576 bytes/);
  ok('truncate file 0');ok('rm file');assert.equal(snapshot().freeBlocks,initial.freeBlocks);
  ok('mkdir dir');ok('write dir/file hi');
  for(const command of ['rm dir','mv dir dir/child','write /missing/file x']) assert.equal(run(command).lines.at(-1).type,'error',command);
  ok('ln -s loop2 loop1');ok('ln -s loop1 loop2');
  assert.equal(run('cat loop1').lines.at(-1).type,'error');
});

test('shell validation never passes malformed commands into the real runtime', () => {
  reset();ok('write file safe');
  for(const command of ['write "unterminated','rm file extra','write file','ls -z','df nonsense','write file "a\0b"']) {
    assert.equal(run(command).lines.at(-1).type,'error',command);
    assert.equal(ok('cat file'),'safe');
  }
  assert.equal(run('concurrency').lines.at(-1).type,'error');
});

test('visualizer reads actual allocated inodes and blocks from WASM memory', () => {
  reset();ok('write file contents');const s=snapshot();
  const file=Object.values(s.inodes).find(n=>n.type==='file' && n.size===8);
  assert.ok(file);assert.ok(file.directBlocks.length);
  for(const id of file.directBlocks) {
    const block=s.blocks.find(b=>b.id===id);
    if(block) { assert.equal(block.ownerInode,file.id);assert.equal(block.type,'data_file'); }
  }
});
