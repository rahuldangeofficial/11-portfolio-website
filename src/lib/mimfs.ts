export interface Inode {
  id: number; type: 'dir' | 'file' | 'symlink'; mode: number; size: number;
  linkCount: number; directBlocks: number[];
}
export interface Volume {
  totalBlocks: number; freeBlocks: number; inodeCount: number;
  inodes: Record<number, Inode>;
  blocks: { id: number; type: 'allocated' | 'data_dir' | 'data_file' | 'free'; ownerInode: number }[];
}
export interface Line { type: 'cmd' | 'output' | 'error' | 'success'; text: string }
export interface MimfsModule {
  ccall(name: string, result: 'number' | 'string', types: string[], args: (string | number)[]): number | string;
}
export const emptyView = (): Volume => ({ totalBlocks: 0, freeBlocks: 0, inodeCount: 0, inodes: {}, blocks: [] });
// Parse quoted and escaped arguments without executing shell syntax.
export function tokenizeCommand(command: string): string[] {
  const tokens: string[] = [];
  let token = '', quote = '', started = false;
  for (let i = 0; i < command.length; i++) {
    const char = command[i];
    if (char === '\\' && quote !== "'") {
      if (++i === command.length) throw new Error('Incomplete escape at end of command.');
      token += command[i]; started = true;
    } else if (quote) {
      if (char === quote) quote = ''; else token += char;
    } else if (char === '"' || char === "'") {
      quote = char; started = true;
    } else if (/\s/.test(char)) {
      if (started) tokens.push(token);
      token = ''; started = false;
    } else { token += char; started = true; }
  }
  if (quote) throw new Error('Unclosed quote.');
  if (started) tokens.push(token);
  return tokens;
}

export function runNativeCommand(module: MimfsModule, command: string): {lines: Line[]; clear?: boolean} {
  if (!command.trim()) return {lines: []};
  const lines: Line[] = [{type: 'cmd', text: `$ ${command}`}];
  try {
    if (command.length > 16384 || command.includes('\0')) throw new Error('Command must be at most 16 KiB and contain no NUL characters.');
    const [cmd, ...args] = tokenizeCommand(command);
    const bounds: Record<string, [number, number]> = {
      help:[0,0], clear:[0,0], format:[0,0], df:[0,1], mkdir:[1,1], touch:[1,1],
      write:[2,Infinity], append:[2,Infinity], cat:[1,1], hexdump:[1,1], stat:[1,1],
      rm:[1,1], mv:[2,2], ln:args[0]==='-s'?[3,3]:[2,2], ls:[0,2], tree:[0,1],
      truncate:[2,2], concurrency:[0,0],
    };
    if (!Object.prototype.hasOwnProperty.call(bounds, cmd)) throw new Error(`Unknown command: ${cmd}. Type help.`);
    if (args.length < bounds[cmd][0] || args.length > bounds[cmd][1]) throw new Error(`Invalid arguments for ${cmd}. Type help.`);
    if (cmd === 'clear') return {lines: [], clear:true};
    if (cmd === 'help') {
      lines.push({type:'output',text:'Real MIMFS C runtime compiled to WebAssembly. Paths start at /.\nformat — reset the 8 MiB volume\nls [-la] [path] | tree [path] | df -h\nmkdir <path> | touch <path>\nwrite <path> <text> | append <path> <text>\ncat <path> | hexdump <path> | stat <path>\nln <target> <link> | ln -s <target> <link>\nmv <source> <destination> | rm <path>\ntruncate <path> <bytes> — native sparse files (up to 64 MiB here)\nconcurrency — 4 real pthreads, 100 atomic appends each (requires isolation)\nclear | ↑/↓ command history | Ctrl+L clear output\nVolume is temporary: refresh or Format resets it.'});
      return {lines};
    }
    let nativeCommand = cmd, a=args[0] || '', b=args[1] || '';
    if (cmd==='write' || cmd==='append') b=args.slice(1).join(' ');
    if (cmd==='format') a='';
    if (cmd==='df' && args.some(arg=>arg!=='-h')) throw new Error('Usage: df [-h]');
    if (cmd==='ls') {
      if(args.filter(arg=>!arg.startsWith('-')).length>1 || args.some(arg=>arg.startsWith('-') && !['-l','-a','-la','-al'].includes(arg))) throw new Error('Usage: ls [-la] [path]');
      a=args.find(arg=>!arg.startsWith('-')) || '/';b=args.some(arg=>arg.includes('l') && arg.startsWith('-'))?'long':'';
    }
    if (cmd==='ln' && args[0]==='-s') {nativeCommand='symlink';a=args[1];b=args[2];}
    const absolute = (path: string) => {
      if (!path) throw new Error('A nonempty path is required.');
      return path.startsWith('/') ? path : '/' + path;
    };
    if (!['format','df','concurrency','symlink'].includes(nativeCommand)) a=absolute(a || (cmd==='tree'?'/':''));
    if (['ln','symlink','mv'].includes(nativeCommand)) b=absolute(b);
    const text=String(module.ccall('shell_exec','string',['string','string','string'],[nativeCommand,a,b]));
    const status=Number(module.ccall('shell_status','number',[],[]));
    lines.push({type:status<0?'error':'output',text});
  } catch(error) { lines.push({type:'error',text:error instanceof Error?error.message:String(error)}); }
  return {lines};
}
