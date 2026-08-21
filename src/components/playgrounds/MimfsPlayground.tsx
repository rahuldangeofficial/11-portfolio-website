import React, { useState, useEffect, useRef, useCallback } from 'react';
import { IconLayers } from '../Icons';

interface MimfsInode {
  id: number;
  type: 'dir' | 'file' | 'symlink';
  mode: number;
  size: number;
  linkCount: number;
  directBlocks: number[];
  indirectBlock: number | null;
  target?: string;
  data?: string;
  createdAt: number;
}

interface DirEntry {
  name: string;
  inodeId: number;
}

export const MimfsPlayground: React.FC = React.memo(() => {
  const [history, setHistory] = useState<Array<{ text: string; type: 'cmd' | 'output' | 'error' | 'success' }>>([]);
  const [inputVal, setInputVal] = useState<string>('');
  const [activeView, setActiveView] = useState<'terminal' | 'block_map'>('terminal');

  const [inodes, setInodes] = useState<Record<number, MimfsInode>>({
    0: {
      id: 0,
      type: 'dir',
      mode: 0o755,
      size: 128,
      linkCount: 2,
      directBlocks: [4],
      indirectBlock: null,
      createdAt: Date.now()
    }
  });

  const [dirs, setDirs] = useState<Record<number, DirEntry[]>>({
    0: []
  });

  const [blockAllocation, setBlockAllocation] = useState<Array<{ id: number; type: 'super' | 'inode_bm' | 'block_bm' | 'inode_tbl' | 'data_dir' | 'data_file' | 'free'; ownerInode?: number }>>([]);
  const terminalEndRef = useRef<HTMLDivElement | null>(null);

  const initVolume = useCallback(() => {
    const blocks = [];
    blocks.push({ id: 0, type: 'super' as const });
    blocks.push({ id: 1, type: 'inode_bm' as const });
    blocks.push({ id: 2, type: 'block_bm' as const });
    blocks.push({ id: 3, type: 'inode_tbl' as const });
    blocks.push({ id: 4, type: 'data_dir' as const, ownerInode: 0 });

    for (let i = 5; i < 48; i++) {
      blocks.push({ id: i, type: 'free' as const });
    }

    setBlockAllocation(blocks);
  }, []);

  useEffect(() => {
    initVolume();
    setHistory([
      { type: 'output', text: 'MIMFS v1.0: In-Memory File System' },
      { type: 'output', text: 'Volume: 64 MiB | 4096 B blocks | Lock-free bitmap allocation' },
      { type: 'output', text: "Type 'help' for available commands." }
    ]);
  }, [initVolume]);

  useEffect(() => {
    if (terminalEndRef.current) {
      const container = terminalEndRef.current.parentElement;
      if (container) {
        container.scrollTop = container.scrollHeight;
      }
    }
  }, [history]);

  const runCommand = (cmdLine: string) => {
    if (!cmdLine.trim()) return;

    const trimmed = cmdLine.trim();
    const parts = trimmed.split(/\s+/);
    const cmd = parts[0].toLowerCase();
    const args = parts.slice(1);

    const newHistory = [...history, { type: 'cmd' as const, text: `$ ${trimmed}` }];

    try {
      switch (cmd) {
        case 'help': {
          newHistory.push({
            type: 'output',
            text: `Available MIMFS POSIX Commands:
  format [sizeMB]      - Mount & format a fresh caller-owned memory region
  ls [-la]             - List directory contents with inodes, mode, and sizes
  mkdir <path>         - Create a directory
  touch <path>         - Create an empty regular file
  write <path> <text>  - Write data to file (allocates direct/indirect blocks)
  cat <path>           - Read file contents
  stat <path>          - Display detailed inode metadata, link counts, and block map
  ln <target> <link>   - Create POSIX hard link (increments ref count)
  ln -s <target> <link> - Create symbolic link
  mv <src> <dst>       - Atomic file or directory rename
  rm <path>            - Unlink file/directory (frees blocks when ref count hits 0)
  tree                 - Display recursive directory hierarchy
  hexdump <path>       - Inspect raw hex byte dump of allocated blocks
  concurrency-sim [N]  - Simulate N parallel POSIX threads creating files lock-free
  df -h                - Display volume memory consumption and block usage
  clear                - Clear terminal console`
          });
          break;
        }

        case 'clear': {
          setHistory([]);
          return;
        }

        case 'df': {
          const usedBlocks = blockAllocation.filter(b => b.type !== 'free').length;
          newHistory.push({
            type: 'output',
            text: `Filesystem     Size   Used  Avail Use% Mounted on
mimfs_vol0     64M   ${(usedBlocks * 4)}K   ${64 * 1024 - usedBlocks * 4}K   ${Math.round((usedBlocks / 48) * 100)}% /`
          });
          break;
        }

        case 'format': {
          const mb = parseInt(args[0], 10) || 64;
          setInodes({
            0: {
              id: 0,
              type: 'dir',
              mode: 0o755,
              size: 128,
              linkCount: 2,
              directBlocks: [4],
              indirectBlock: null,
              createdAt: Date.now()
            }
          });
          setDirs({ 0: [] });
          initVolume();
          newHistory.push({
            type: 'success',
            text: `Formatted ${mb} MiB volume. Block size: 4096 B, Inodes: 1024.`
          });
          break;
        }

        case 'mkdir': {
          if (!args[0]) throw new Error('Usage: mkdir <path>');
          const path = args[0].replace(/^\//, '');
          const nextInodeId = Object.keys(inodes).length;

          setInodes(prev => ({
            ...prev,
            [nextInodeId]: {
              id: nextInodeId,
              type: 'dir',
              mode: 0o755,
              size: 128,
              linkCount: 2,
              directBlocks: [nextInodeId + 4],
              indirectBlock: null,
              createdAt: Date.now()
            }
          }));

          setDirs(prev => ({
            ...prev,
            0: [...(prev[0] || []), { name: path, inodeId: nextInodeId }],
            [nextInodeId]: []
          }));

          setBlockAllocation(prev => {
            const next = [...prev];
            const freeIdx = next.findIndex(b => b.type === 'free');
            if (freeIdx !== -1) {
              next[freeIdx] = { id: freeIdx, type: 'data_dir', ownerInode: nextInodeId };
            }
            return next;
          });

          newHistory.push({ type: 'success', text: `Directory created: /${path} [Inode: #${nextInodeId}]` });
          break;
        }

        case 'touch':
        case 'write': {
          if (!args[0]) throw new Error(`Usage: ${cmd} <path> [content]`);
          const fileName = args[0].replace(/^\//, '');
          const content = args.slice(1).join(' ') || (cmd === 'touch' ? '' : 'hello world');

          const existing = (dirs[0] || []).find(e => e.name === fileName);
          if (existing) {
            setInodes(prev => ({
              ...prev,
              [existing.inodeId]: {
                ...prev[existing.inodeId],
                data: content,
                size: content.length
              }
            }));
            newHistory.push({
              type: 'success',
              text: `Updated /${fileName} (${content.length} bytes written across direct blocks).`
            });
          } else {
            const nextInodeId = Object.keys(inodes).length;
            const blockId = nextInodeId + 4;

            setInodes(prev => ({
              ...prev,
              [nextInodeId]: {
                id: nextInodeId,
                type: 'file',
                mode: 0o644,
                size: content.length,
                linkCount: 1,
                directBlocks: [blockId],
                indirectBlock: null,
                data: content,
                createdAt: Date.now()
              }
            }));

            setDirs(prev => ({
              ...prev,
              0: [...(prev[0] || []), { name: fileName, inodeId: nextInodeId }]
            }));

            setBlockAllocation(prev => {
              const next = [...prev];
              const freeIdx = next.findIndex(b => b.type === 'free');
              if (freeIdx !== -1) {
                next[freeIdx] = { id: freeIdx, type: 'data_file', ownerInode: nextInodeId };
              }
              return next;
            });

            newHistory.push({
              type: 'success',
              text: `Created /${fileName} [Inode: #${nextInodeId}, Size: ${content.length} bytes, Block: #${blockId}]`
            });
          }
          break;
        }

        case 'cat': {
          if (!args[0]) throw new Error('Usage: cat <path>');
          const fileName = args[0].replace(/^\//, '');
          const entry = (dirs[0] || []).find(e => e.name === fileName);
          if (!entry) throw new Error(`cat: ${fileName}: No such file or directory`);

          const inode = inodes[entry.inodeId];
          if (inode.type === 'dir') throw new Error(`cat: ${fileName}: Is a directory`);
          newHistory.push({ type: 'output', text: inode.data || '(empty file)' });
          break;
        }

        case 'hexdump': {
          if (!args[0]) throw new Error('Usage: hexdump <path>');
          const fileName = args[0].replace(/^\//, '');
          const entry = (dirs[0] || []).find(e => e.name === fileName);
          if (!entry) throw new Error(`hexdump: ${fileName}: No such file or directory`);

          const inode = inodes[entry.inodeId];
          const text = inode.data || '';
          let dumpStr = `00000000  `;
          for (let i = 0; i < Math.min(text.length, 32); i++) {
            dumpStr += `${text.charCodeAt(i).toString(16).padStart(2, '0')} `;
            if (i === 7 || i === 15 || i === 23) dumpStr += ' ';
          }
          dumpStr += ` |${text.substring(0, 32)}|`;
          newHistory.push({ type: 'output', text: dumpStr });
          break;
        }

        case 'ls': {
          const isLong = args.includes('-la') || args.includes('-l') || args.includes('-a');
          const entries = dirs[0] || [];

          if (entries.length === 0) {
            newHistory.push({ type: 'output', text: 'total 0 (empty directory)' });
          } else if (isLong) {
            let output = 'total ' + entries.length * 4 + '\n';
            output += 'drwxr-xr-x  2 root root  128 Feb 22 01:00 .\n';
            output += 'drwxr-xr-x  2 root root  128 Feb 22 01:00 ..\n';
            entries.forEach(e => {
              const node = inodes[e.inodeId];
              const typeChar = node.type === 'dir' ? 'd' : node.type === 'symlink' ? 'l' : '-';
              const modeStr = `${typeChar}rwxr-xr-x`;
              const links = node.linkCount;
              const size = node.size.toString().padStart(6, ' ');
              const targetStr = node.target ? ` -> ${node.target}` : '';
              output += `${modeStr}  ${links} root root ${size} Feb 22 01:00 ${e.name}${targetStr}\n`;
            });
            newHistory.push({ type: 'output', text: output.trim() });
          } else {
            const listStr = entries.map(e => (inodes[e.inodeId]?.type === 'dir' ? `${e.name}/` : e.name)).join('  ');
            newHistory.push({ type: 'output', text: listStr });
          }
          break;
        }

        case 'stat': {
          if (!args[0]) throw new Error('Usage: stat <path>');
          const fileName = args[0].replace(/^\//, '');
          const entry = (dirs[0] || []).find(e => e.name === fileName);
          if (!entry) throw new Error(`stat: cannot stat '${fileName}': No such file or directory`);

          const node = inodes[entry.inodeId];
          newHistory.push({
            type: 'output',
            text: `  File: /${fileName}
  Size: ${node.size}        Blocks: ${node.directBlocks.length}        IO Block: 4096   ${node.type === 'dir' ? 'directory' : 'regular file'}
Device: mem00      Inode: ${node.id}         Links: ${node.linkCount}
Access: (0${node.mode.toString(8)}/${node.type === 'dir' ? 'drwxr-xr-x' : node.type === 'symlink' ? 'lrwxrwxrwx' : '-rw-r--r--'})  Uid: ( 0/root)   Gid: ( 0/root)
Direct Blocks: [${node.directBlocks.join(', ')}]
Allocation: Bitmap`
          });
          break;
        }

        case 'ln': {
          if (args[0] === '-s') {
            const target = args[1];
            const linkName = args[2]?.replace(/^\//, '');
            if (!target || !linkName) throw new Error('Usage: ln -s <target> <link>');

            const nextInodeId = Object.keys(inodes).length;
            setInodes(prev => ({
              ...prev,
              [nextInodeId]: {
                id: nextInodeId,
                type: 'symlink',
                mode: 0o777,
                size: target.length,
                linkCount: 1,
                directBlocks: [],
                indirectBlock: null,
                target: target,
                createdAt: Date.now()
              }
            }));
            setDirs(prev => ({
              ...prev,
              0: [...(prev[0] || []), { name: linkName, inodeId: nextInodeId }]
            }));
            newHistory.push({ type: 'success', text: `Symlink: /${linkName} -> ${target}` });
          } else {
            const target = args[0]?.replace(/^\//, '');
            const linkName = args[1]?.replace(/^\//, '');
            if (!target || !linkName) throw new Error('Usage: ln <target> <link>');

            const entry = (dirs[0] || []).find(e => e.name === target);
            if (!entry) throw new Error(`ln: '${target}': No such file or directory`);

            setInodes(prev => ({
              ...prev,
              [entry.inodeId]: {
                ...prev[entry.inodeId],
                linkCount: prev[entry.inodeId].linkCount + 1
              }
            }));

            setDirs(prev => ({
              ...prev,
              0: [...(prev[0] || []), { name: linkName, inodeId: entry.inodeId }]
            }));

            newHistory.push({
              type: 'success',
              text: `Hard link created: /${linkName} pointing to Inode #${entry.inodeId} (Ref count: ${inodes[entry.inodeId].linkCount + 1})`
            });
          }
          break;
        }

        case 'mv': {
          const src = args[0]?.replace(/^\//, '');
          const dst = args[1]?.replace(/^\//, '');
          if (!src || !dst) throw new Error('Usage: mv <src> <dst>');

          const entryIdx = (dirs[0] || []).findIndex(e => e.name === src);
          if (entryIdx === -1) throw new Error(`mv: cannot stat '${src}': No such file or directory`);

          setDirs(prev => {
            const nextRoot = [...(prev[0] || [])];
            nextRoot[entryIdx] = { ...nextRoot[entryIdx], name: dst };
            return { ...prev, 0: nextRoot };
          });

          newHistory.push({ type: 'success', text: `Atomic rename: /${src} -> /${dst}` });
          break;
        }

        case 'rm': {
          const path = args[0]?.replace(/^\//, '');
          if (!path) throw new Error('Usage: rm <path>');

          const entry = (dirs[0] || []).find(e => e.name === path);
          if (!entry) throw new Error(`rm: cannot remove '${path}': No such file or directory`);

          const node = inodes[entry.inodeId];

          setDirs(prev => ({
            ...prev,
            0: (prev[0] || []).filter(e => e.name !== path)
          }));

          if (node.linkCount <= 1) {
            // Last reference: free inode and its blocks
            setInodes(prev => {
              const next = { ...prev };
              delete next[entry.inodeId];
              return next;
            });

            setBlockAllocation(prev =>
              prev.map(b =>
                b.ownerInode === entry.inodeId
                  ? { ...b, type: 'free' as const, ownerInode: undefined }
                  : b
              )
            );

            newHistory.push({ type: 'success', text: `Removed: /${path} (inode #${entry.inodeId} freed, ${node.directBlocks.length} block(s) released)` });
          } else {
            // Decrement link count
            setInodes(prev => ({
              ...prev,
              [entry.inodeId]: {
                ...prev[entry.inodeId],
                linkCount: prev[entry.inodeId].linkCount - 1
              }
            }));

            newHistory.push({ type: 'success', text: `Unlinked: /${path} (inode #${entry.inodeId} ref count: ${node.linkCount - 1})` });
          }
          break;
        }

        case 'tree': {
          let treeStr = '/\n';
          const entries = dirs[0] || [];
          entries.forEach((e, idx) => {
            const isLast = idx === entries.length - 1;
            const prefix = isLast ? '└── ' : '├── ';
            const node = inodes[e.inodeId];
            treeStr += `${prefix}${e.name}${node?.type === 'dir' ? '/' : ''}\n`;
          });
          newHistory.push({ type: 'output', text: treeStr.trim() });
          break;
        }

        case 'concurrency-sim': {
          const threads = Math.min(parseInt(args[0], 10) || 4, 8);
          const baseInode = Object.keys(inodes).length;
          newHistory.push({
            type: 'output',
            text: `[pthread] Spawning ${threads} worker threads...`
          });

          for (let t = 0; t < threads; t++) {
            const tFile = `thread_${t}.log`;
            const tInode = baseInode + t;
            const tContent = `Worker ${t}: write completed at block offset ${tInode + 4}.`;

            setInodes(prev => ({
              ...prev,
              [tInode]: {
                id: tInode,
                type: 'file',
                mode: 0o644,
                size: tContent.length,
                linkCount: 1,
                directBlocks: [tInode + 4],
                indirectBlock: null,
                data: tContent,
                createdAt: Date.now()
              }
            }));

            setDirs(prev => ({
              ...prev,
              0: [...(prev[0] || []), { name: tFile, inodeId: tInode }]
            }));

            setBlockAllocation(prev => {
              const next = [...prev];
              const freeIdx = next.findIndex(b => b.type === 'free');
              if (freeIdx !== -1) {
                next[freeIdx] = { id: freeIdx, type: 'data_file', ownerInode: tInode };
              }
              return next;
            });
          }

          newHistory.push({
            type: 'success',
            text: `[pthread] ${threads} threads completed. ${threads} files created via atomic bitmap allocation.`
          });
          break;
        }

        default:
          throw new Error(`Unknown command: '${cmd}'. Type 'help' for command manual.`);
      }
    } catch (err: unknown) {
      const errMsg = err instanceof Error ? err.message : String(err);
      newHistory.push({ type: 'error', text: errMsg });
    }

    setHistory(newHistory);
    setInputVal('');
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      runCommand(inputVal);
    }
  };

  const executePresetAction = (preset: string) => {
    switch (preset) {
      case 'format_demo':
        runCommand('format 64');
        break;
      case 'dir_tree':
        runCommand('mkdir var');
        runCommand('mkdir etc');
        runCommand('write var/system.log Kernel boot sequence loaded.');
        runCommand('tree');
        break;
      case 'concurrency':
        runCommand('concurrency-sim 4');
        runCommand('ls -la');
        break;
      case 'links':
        runCommand('write master.cfg version=2.4.0');
        runCommand('ln master.cfg config_link.cfg');
        runCommand('ln -s /master.cfg symlink_to_master');
        runCommand('stat master.cfg');
        break;
      case 'hexdump_demo':
        runCommand('hexdump master.cfg');
        break;
      default:
        break;
    }
  };

  return (
    <div className="border border-[var(--border-default)] rounded-lg overflow-hidden bg-[#0d1117] text-[#e6edf3] font-mono text-xs sm:text-[13px] shadow-lg">
      
      {/* ── Titlebar ── */}
      <div className="bg-[#161b22] px-4 py-2.5 border-b border-[#30363d] flex flex-wrap items-center justify-between gap-3">
        <div className="flex items-center gap-3">
          <div className="flex items-center gap-1.5 border-r border-[#30363d] pr-3">
            <div className="w-2.5 h-2.5 rounded-full bg-[#ff5f56]" />
            <div className="w-2.5 h-2.5 rounded-full bg-[#ffbd2e]" />
            <div className="w-2.5 h-2.5 rounded-full bg-[#27c93f]" />
          </div>
          <span className="font-bold text-white tracking-wide">MIMFS Shell</span>
          <span className="px-2 py-0.5 rounded text-[10px] bg-[#21262d] text-emerald-400 border border-[#30363d] flex items-center gap-1.5">
            <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
            IN-MEMORY FS
          </span>
        </div>

        {/* View Toggle Tabs */}
        <div className="flex bg-[#21262d] p-0.5 rounded border border-[#30363d]">
          <button
            onClick={() => setActiveView('terminal')}
            className={`px-2.5 py-1 text-xs rounded transition-colors ${
              activeView === 'terminal'
                ? 'bg-emerald-700 text-white font-bold'
                : 'text-gray-400 hover:text-white'
            }`}
          >
            UNIX Terminal
          </button>
          <button
            onClick={() => setActiveView('block_map')}
            className={`px-2.5 py-1 text-xs rounded transition-colors ${
              activeView === 'block_map'
                ? 'bg-emerald-700 text-white font-bold'
                : 'text-gray-400 hover:text-white'
            }`}
          >
            Block Visualizer
          </button>
        </div>
      </div>

      {/* Quick Demo Action Buttons */}
      <div className="px-4 py-1.5 bg-[#161b22]/70 border-b border-[#30363d] flex items-center gap-2 flex-wrap text-[11px]">
        <span className="text-gray-400">Demos:</span>
        <button
          onClick={() => executePresetAction('format_demo')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Format Volume
        </button>
        <button
          onClick={() => executePresetAction('dir_tree')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Directories
        </button>
        <button
          onClick={() => executePresetAction('concurrency')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Concurrency
        </button>
        <button
          onClick={() => executePresetAction('links')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Hard / Symlinks
        </button>
        <button
          onClick={() => executePresetAction('hexdump_demo')}
          className="px-2 py-0.5 rounded bg-[#21262d] text-gray-300 hover:text-white border border-[#30363d]"
        >
          Hexdump
        </button>
      </div>

      {/* ── Terminal Console ── */}
      {activeView === 'terminal' && (
        <div className="p-4 bg-[#090d13] text-[#e6edf3] font-mono text-xs sm:text-[13px] h-[320px] sm:h-[350px] lg:h-[400px] flex flex-col justify-between">
          <div className="overflow-y-auto dark-scrollbar space-y-1 pr-2 flex-1">
            {history.map((h, i) => (
              <div
                key={i}
                className={`whitespace-pre-wrap leading-relaxed ${
                  h.type === 'cmd'
                    ? 'text-emerald-400 font-bold'
                    : h.type === 'error'
                    ? 'text-red-400'
                    : h.type === 'success'
                    ? 'text-emerald-400 font-semibold'
                    : 'text-gray-300'
                }`}
              >
                {h.text}
              </div>
            ))}
            <div ref={terminalEndRef} />
          </div>

          <div className="flex items-center gap-2 pt-2 border-t border-[#21262d] mt-2">
            <span className="text-emerald-400 font-bold">$</span>
            <input
              type="text"
              value={inputVal}
              onChange={(e) => setInputVal(e.target.value)}
              onKeyDown={handleKeyDown}
              placeholder="Type command (e.g. ls -la, mkdir var, concurrency-sim, df -h)..."
              className="w-full bg-transparent text-[#e6edf3] font-mono text-xs sm:text-[13px] focus:outline-none placeholder:text-gray-600"
              
            />
          </div>
        </div>
      )}

      {/* ── Storage Block Map View ── */}
      {activeView === 'block_map' && (
        <div className="p-4 bg-[#090d13] h-[320px] sm:h-[350px] lg:h-[400px] overflow-y-auto dark-scrollbar space-y-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              <IconLayers size={15} className="text-emerald-400" />
              <span className="text-xs font-bold text-gray-200">
                Memory Layout (4096 B blocks)
              </span>
            </div>
            <span className="text-[11px] text-gray-400">
              {Object.keys(inodes).length} inodes allocated
            </span>
          </div>

          {/* Block Matrix Grid */}
          <div className="grid grid-cols-6 sm:grid-cols-8 md:grid-cols-12 gap-1.5 p-3 rounded bg-[#161b22] border border-[#30363d]">
            {blockAllocation.map((block) => {
              let bg = 'bg-[#21262d] text-gray-500 border border-[#30363d]';
              let label = `Block #${block.id}`;

              if (block.type === 'super') {
                bg = 'bg-purple-900/80 text-purple-200 border border-purple-600 font-bold';
                label = 'SUPERBLOCK';
              } else if (block.type === 'inode_bm') {
                bg = 'bg-emerald-900/80 text-emerald-200 border border-emerald-600 font-bold';
                label = 'INODE_BM';
              } else if (block.type === 'block_bm') {
                bg = 'bg-indigo-900/80 text-indigo-200 border border-indigo-600 font-bold';
                label = 'BLOCK_BM';
              } else if (block.type === 'inode_tbl') {
                bg = 'bg-amber-900/80 text-amber-200 border border-amber-600 font-bold';
                label = 'INODE_TBL';
              } else if (block.type === 'data_dir') {
                bg = 'bg-emerald-900/80 text-emerald-200 border border-emerald-500 font-bold';
                label = `DIR_INODE_${block.ownerInode ?? '0'}`;
              } else if (block.type === 'data_file') {
                bg = 'bg-cyan-900/80 text-cyan-200 border border-cyan-500 font-bold';
                label = `FILE_INODE_${block.ownerInode ?? '0'}`;
              }

              return (
                <div
                  key={block.id}
                  className={`h-9 rounded flex flex-col items-center justify-center p-1 text-[9px] font-mono transition-transform hover:scale-105 cursor-pointer select-none ${bg}`}
                  title={`${label} (Block ID: ${block.id})`}
                >
                  <span>#{block.id}</span>
                  <span className="text-[7px] truncate max-w-full opacity-90">{block.type.toUpperCase()}</span>
                </div>
              );
            })}
          </div>

          {/* Inode Table Info */}
          <div className="space-y-2 pt-2 border-t border-[#21262d]">
            <h4 className="text-xs font-bold text-gray-300">Inode Table</h4>
            <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-2 text-[11px]">
              {Object.values(inodes).map(node => (
                <div key={node.id} className="p-2 rounded bg-[#161b22] border border-[#30363d]">
                  <div className="flex items-center justify-between font-bold text-emerald-400">
                    <span>Inode #{node.id}</span>
                    <span className="uppercase text-[9px] px-1 rounded bg-[#21262d] text-gray-300">
                      {node.type}
                    </span>
                  </div>
                  <div className="text-[10px] text-gray-400 mt-1 space-y-0.5">
                    <div>Size: {node.size} bytes | Links: {node.linkCount}</div>
                    <div>Blocks: [{node.directBlocks.join(', ') || 'inline'}]</div>
                    {node.target && <div className="text-cyan-300 truncate">Target: {node.target}</div>}
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>
      )}
    </div>
  );
});

MimfsPlayground.displayName = 'MimfsPlayground';
export default MimfsPlayground;
