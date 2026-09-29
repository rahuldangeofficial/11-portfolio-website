# MIMFS WebAssembly build

The shell runs the actual MIMFS C filesystem, not a JavaScript filesystem emulator.

Source: https://github.com/rahuldangeofficial/MIMFS/tree/9599286060a4852e8117ea1e443aff202d4eb918
The vendored `mimfs/src` and `mimfs/include` trees were compared on 2026-09-29 with
`/Users/rahuldangeofficial/rahuldange/MIMFS`; their contents match exactly.
The MIT license is retained in `mimfs/LICENSE`. Upstream C source is unmodified.
The browser-specific adapter is `bridge/mimfs_browser.c`.

## Rebuild

Use an activated official Emscripten 4.0.23 SDK and set `EMSDK` to its directory:

```
EMSDK=/path/to/emsdk yarn wasm:build
yarn test
yarn build
yarn preview --host 127.0.0.1
```

The prebuilt `.mjs`/`.wasm` files in `public/wasm` are included in ordinary Yarn
builds; visitors and normal site builds do not need a compiler. The temporary SDK
used for the initial build is not a permanent project dependency. Rebuilding C
requires installing/activating that SDK again if its temporary folder was removed.

## Runtime

A dedicated browser worker owns an 8 MiB caller-supplied region inside WebAssembly
memory. All file operations call the C library. The visualizer reads its real
allocation bitmap and inode table after commands complete, displaying the first
48 data blocks and up to 128 live inodes; it is not a map of the entire volume.

The threaded build uses four real POSIX threads sharing the WebAssembly heap.
The concurrency command appends 100 records per thread to one shared file. Tests
verify all 400 unique records. On non-isolated hosts the same C filesystem runs
in a single-threaded build; the concurrency command reports unavailable instead
of simulating threads. No filesystem data is sent to a server or saved to disk.
Refresh, Format, unmounting the demo, or fatal runtime recovery resets the volume.

Vite development and preview servers send COOP/COEP headers. For a deployed host,
serve the document and worker assets with:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

The static files must be served from the same origin, with `.wasm` using
`application/wasm` and `.mjs` using a JavaScript MIME type. Browser secure-context
rules also apply (HTTPS or localhost). This does not grant access to the visitor's
real operating-system filesystem. LLFPL remains separately labelled as a
JavaScript simulation; this port changes MIMFS only.
