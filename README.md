# rahuldange.com

[![tests](https://img.shields.io/github/actions/workflow/status/rahuldangeofficial/11-portfolio-website/ci.yml?branch=main&style=flat-square&label=tests)](https://github.com/rahuldangeofficial/11-portfolio-website/actions/workflows/ci.yml)

My personal site, and a live bench where my C projects run in the browser as WebAssembly.

**Live: [rahuldange.com](https://rahuldange.com)**

## What runs here

- **MIMFS shell.** The actual [MIMFS](https://github.com/rahuldangeofficial/MIMFS) C11 filesystem, compiled with
  Emscripten from unmodified upstream source. It runs in a dedicated Web Worker on an 8 MiB caller-supplied region
  of WebAssembly memory, and the visualizer reads the real allocation bitmap and inode table after each command.
  The threaded build runs four real POSIX threads that each append 100 records to one shared file.
- **LLFPL playground.** The [LLFPL](https://github.com/rahuldangeofficial/LLFPL) interpreter compiled to
  WebAssembly. A small bridge exposes its real counters and read-only arena samples without changing the
  evaluator, the parser or the session layout.
- **The site itself.** React, TypeScript, Vite and Tailwind CSS, prerendered to static HTML so content is visible
  before JavaScript loads.

No filesystem data leaves the browser: every command runs locally against the in-memory volume.

## Tests

`yarn test` runs Node's built-in test runner against the real WebAssembly builds:

- MIMFS: volume creation, UTF-8 transfers, hard links, symbolic links, atomic rename, sparse files, directory
  errors and cycles, allocation accounting, and that the visualizer reads the actual allocated inodes and blocks.
- Threads: four WebAssembly pthreads append 400 records to one file, and all 400 arrive complete and unique.
- LLFPL: malformed syntax, out-of-bounds access and oversized allocation are rejected; fresh sessions are
  enforced; UTF-8 and NUL validation happen before input reaches C.
- The shell never passes a malformed command into the runtime.

## Run locally

```sh
yarn install
yarn dev          # development server
yarn test         # tests against the WebAssembly builds
yarn build        # production build + prerender
yarn preview      # serve the production build
```

The prebuilt `.wasm`/`.mjs` files in `public/wasm` are committed, so a normal build needs no C compiler.
Rebuilding them needs the Emscripten SDK; see [native/README.md](native/README.md).

## Deployment note

The threaded MIMFS build uses `SharedArrayBuffer`, which browsers only enable on cross-origin-isolated pages.
Serve the site with:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

The Vite dev and preview servers already send these. On a host without them, the demo falls back to a
single-threaded build and reports the concurrency command as unavailable rather than simulating threads.

## Layout

```
src/components/   page sections, playgrounds and the filesystem visualizer
src/lib/          MIMFS and LLFPL loaders and their Web Workers
native/           vendored C sources (MIT-licensed upstream) and the browser bridges
public/wasm/      prebuilt WebAssembly modules
scripts/          WebAssembly build and prerender scripts
tests/            Node test-runner suites
```
