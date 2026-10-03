// UI adapter only: all parsing, evaluation and slab storage live in the C/WASM runtime.
export interface ExecutionLog {
  type: 'trace' | 'commit' | 'error' | 'summary';
  message: string;
  value?: number;
  cost?: number;
  timeNs?: number;
}
export interface LlfplModule {
  ccall(name: string, result: 'number' | 'string', types: string[], args: (number | string)[]): number | string;
}
export interface LlfplResult {
  logs: ExecutionLog[];
  slabs: Record<string, Float64Array>;
  stats: {commits: number; totalCost: number; timeNs: number};
}
export function executeLlfpl(module: LlfplModule, source: string): LlfplResult {
  if (new TextEncoder().encode(source).length > 32768 || source.includes('\0')) {
    throw new Error('Source must fit in 32 KiB and contain no NUL characters.');
  }
  const number = (name: string, ...args: number[]) => Number(module.ccall(name, 'number', args.map(()=>'number'), args));
  const status = Number(module.ccall('browser_run', 'number', ['string'], [source]));
  const diagnostics = String(module.ccall('browser_output', 'string', ['number'], [1]));
  const logs: ExecutionLog[] = [{type:'trace',message:'Runtime: real LLFPL C / WebAssembly · browser execution timings'}];
  // Render the C runtime's trace; never reconstruct or evaluate expressions here.
  const outputLines = diagnostics.split('\n').filter(line => line && (!line.includes('llfpl: trace:') || /commit \d+:/.test(line)));
  for (const line of outputLines.slice(0, 200)) {
    const commit = line.match(/commit (\d+): (\S+) \(cost (\d+), elapsed (\d+) ns\)/);
    if (commit) logs.push({type:'commit', message:`Commit ${commit[1]} => ${commit[2]}`,value:Number(commit[2]),cost:Number(commit[3]),timeNs:Number(commit[4])});
    else logs.push({type:line.includes('error:')?'error':line.startsWith('llfpl:')?'summary':'trace',message:line});
  }
  if (outputLines.length > 200) logs.push({type:'summary',message:'Trace display limited to 200 lines.'});
  if (status !== 0 && !logs.some(log=>log.type==='error')) logs.push({type:'error',message:'The native LLFPL runtime could not complete this program.'});
  const slabs: Record<string, Float64Array> = Object.create(null);
  for (let i=0;i<number('browser_arena_count');i++) {
    const name=String(module.ccall('browser_arena_name','string',['number'],[i]));
    const count=Math.min(4,Math.floor(number('browser_arena_size',i)/8));
    slabs[name]=Float64Array.from({length:count},(_,slot)=>number('browser_arena_value',i,slot));
  }
  return {logs,slabs,stats:{commits:number('browser_stat',0),totalCost:number('browser_stat',1),timeNs:number('browser_stat',2)}};
}
