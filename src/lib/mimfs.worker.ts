import { runNativeCommand, type MimfsModule, type Volume, type Line } from './mimfs';

export interface ShellRequest { id: number; commands: string[] }
export interface ShellResponse { id: number; volume?: Volume; lines: Line[]; clear: boolean; fatal?: boolean; threaded?: boolean }
let runtime: Promise<MimfsModule> | undefined;
let announced = false;
const threaded = self.crossOriginIsolated && typeof SharedArrayBuffer !== 'undefined';
async function loadRuntime() {
  const name = threaded ? 'mimfs-threaded' : 'mimfs';
  const url = new URL(`/wasm/${name}.mjs`, self.location.origin).href;
  const { default: factory } = await import(/* @vite-ignore */ url);
  const module: MimfsModule = await factory({ locateFile: (file: string) => new URL(`/wasm/${file}`, self.location.origin).href });
  if (Number(module.ccall('shell_init', 'number', [], [])) !== 0) throw new Error('MIMFS could not format its WebAssembly memory.');
  return module;
}
self.onmessage = async ({ data }: MessageEvent<ShellRequest>) => {
  try {
    const module = await (runtime ??= loadRuntime());
    let lines: Line[] = [];
    let clear = false;
    if (!announced) {
      lines.push({type:'success',text:`Real MIMFS C / WebAssembly ready · 8 MiB volume · ${threaded ? '4 POSIX threads available' : 'single-threaded (deployment is not cross-origin isolated)'}. Type help.`});
      announced = true;
    }
    for (const command of data.commands) {
      const result = runNativeCommand(module, command);
      if (result.clear) {lines=[];clear=true;}
      lines.push(...result.lines);
      if (result.lines.some(line=>line.type==='error')) break;
    }
    const volume: Volume = JSON.parse(String(module.ccall('shell_snapshot','string',[],[])));
    self.postMessage({id:data.id,volume,lines,clear,threaded} satisfies ShellResponse);
  } catch(error) {
    self.postMessage({id:data.id,lines:[{type:'error',text:`WebAssembly runtime failed: ${error instanceof Error?error.message:String(error)}. Retry to start a fresh volume.`}],clear:false,fatal:true} satisfies ShellResponse);
  }
};
