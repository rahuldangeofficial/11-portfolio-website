import { readdirSync, mkdirSync, existsSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { join } from 'node:path';
const sdk = process.env.EMSDK || '/tmp/portfolio-emsdk';
const emcc = join(sdk, 'upstream/emscripten/emcc');
if (!existsSync(emcc)) throw new Error('Set EMSDK to an activated Emscripten 4.0.23 SDK. See native/README.md.');
const root = 'native/mimfs';
const sources = ['support', 'core', 'api'].flatMap(dir => readdirSync(`${root}/src/${dir}`).filter(f => f.endsWith('.c')).map(f => `${root}/src/${dir}/${f}`));
mkdirSync('public/wasm', { recursive: true });
for (const threaded of [false, true]) {
  const args = [...sources, 'native/bridge/mimfs_browser.c', '-I'+root+'/include', '-I'+root+'/src', '-std=c11', '-D_POSIX_C_SOURCE=200809L', '-O2', '-fno-strict-aliasing', '-Wall', '-Wextra', '-Werror',
    '-sMODULARIZE=1', '-sEXPORT_ES6=1', '-sENVIRONMENT=web,worker,node', '-sINITIAL_MEMORY=33554432', '-sSTACK_SIZE=1048576', '-sEXPORTED_RUNTIME_METHODS=["ccall"]', '-sFILESYSTEM=0', '-sMALLOC=emmalloc', '-o', `public/wasm/mimfs${threaded?'-threaded':''}.mjs`,
    ...(threaded?['-pthread','-sPTHREAD_POOL_SIZE=4','-sPTHREAD_POOL_SIZE_STRICT=2']:[])];
  const result = spawnSync(emcc, args, {stdio:'inherit'});
  if(result.status!==0) process.exit(result.status || 1);
}
