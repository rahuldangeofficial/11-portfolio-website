# MIMFS

**A multithreaded in-memory file system for C.**

![Language](https://img.shields.io/badge/language-C11-00599C?style=flat-square)
![Threads](https://img.shields.io/badge/threads-POSIX-4b5563?style=flat-square)
![Dependencies](https://img.shields.io/badge/dependencies-none-4b5563?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-1f6feb?style=flat-square)

MIMFS implements a complete POSIX-style filesystem inside a memory region that the
caller provides. It performs no dynamic allocation of its own, at any point, so the
region may be a static buffer, a shared mapping, a huge page arena or battery-backed
memory. Every operation is safe to call from any number of threads at once with no
external serialisation.

---

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Motivation](#motivation)
- [Building and Installation](#building-and-installation)
  - [Requirements](#requirements)
  - [Make Targets](#make-targets)
  - [Vendoring](#vendoring)
- [API Reference](#api-reference)
  - [Status Codes](#status-codes)
  - [Volume Lifecycle](#volume-lifecycle)
  - [File Operations](#file-operations)
  - [Namespace Operations](#namespace-operations)
  - [Metadata and Directory Streams](#metadata-and-directory-streams)
  - [Path Semantics](#path-semantics)
- [Concurrency Model](#concurrency-model)
  - [Guarantees](#guarantees)
  - [Worked Example](#worked-example)
  - [Lock Granularity](#lock-granularity)
- [Architecture](#architecture)
  - [Region Layout](#region-layout)
  - [Cache Alignment](#cache-alignment)
  - [Lock-Free Allocation](#lock-free-allocation)
  - [Block Map](#block-map)
  - [Directory Format](#directory-format)
  - [Inode Lifetime](#inode-lifetime)
- [Format Limits](#format-limits)
- [Benchmarks](#benchmarks)
- [Testing and Verification](#testing-and-verification)
- [Project Structure](#project-structure)
- [License](#license)

---

## Overview

The implementation is not a demonstration. It provides:

- **Caller-owned storage.** A volume is one memory region supplied by the caller. The
  library never allocates.
- **Position independence.** The region holds byte offsets rather than absolute
  pointers, so it can be mapped at a different address by a different process, or
  relocated wholesale with a single copy, and still mount.
- **Thread safety by construction.** Every entry point may be called concurrently. No
  external locking is required, and there is no global lock on the data path.
- **Lock-free allocation.** Blocks and inodes are claimed with atomic operations against
  per-group bitmaps.
- **POSIX-style semantics.** Hard links, symbolic links with inline storage, sparse
  files, atomic rename, shared file offsets, positioned transfers and directory streams.
- **Files up to 4 TiB.** Twelve direct block pointers plus a three-level indirect map.
- **Reference-counted inode lifetimes.** An unlinked file stays alive and readable while
  a descriptor is still open on it.
- **No dependencies.** C11, the C library and POSIX threads.

## Quick Start

```c
#include <mimfs/mimfs.h>

static unsigned char region[64 * 1024 * 1024];

struct mimfs_volume *volume = mimfs_format(region, sizeof(region), NULL, NULL);

mimfs_create_directory(volume, "/var/log", 0755);

int descriptor = mimfs_open(volume, "/var/log/messages",
                            MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE | MIMFS_OPEN_APPEND,
                            0644);

mimfs_write(volume, descriptor, "started\n", 8);
mimfs_close(volume, descriptor);
```

## Motivation

Three situations call for a filesystem that lives entirely in memory and is addressed
through paths rather than through pointers.

**A component expects a filesystem.** Configuration loaders, template engines, archive
readers and test fixtures are frequently written against open, read, write and readdir.
Giving them a real filesystem is often less work than giving them an abstraction layer,
and it removes the abstraction layer from the shipped product.

**The storage is memory, but the semantics must be a filesystem's.** Addressable
non-volatile memory, a shared mapping between processes, or a RAM disk in an embedded
system all need names, directories, links and atomic rename. They do not need a block
device underneath, and paying for one costs a translation layer and a page cache.

**Tests need a filesystem that cannot leak.** A volume is one memory region. Formatting
it is one call and discards everything. There is no temporary directory to clean up, no
file descriptor to leak into the next test, and no dependence on the state of the host.

## Building and Installation

### Requirements

A C11 compiler and POSIX threads. Nothing else. There is no configuration step and no
generated header.

### Make Targets

| Target | Result |
| --- | --- |
| `make` | Static library at `build/libmimfs.a` |
| `make shared` | Shared library at `build/libmimfs.so` |
| `make tests` | Build and run the test suite |
| `make tests-strict` | The same, under the address and undefined behaviour sanitizers |
| `make benchmark` | Build and run the measurements |
| `make examples` | Build the two example programs |
| `make format-check` | Fail if any source file is not formatted |
| `make analyse` | Run the compiler's static analyser over the library |
| `make coverage` | Run the tests with coverage instrumentation |
| `make install` | Install headers and the library under `PREFIX` |

`make help` lists every target with its description.

### Vendoring

To fold MIMFS into an existing project, add `include/` to the include path and compile
everything under `src/`.

```sh
cc -std=c11 -Iinclude -Isrc -O2 $(find src -name '*.c') your_program.c -lpthread
```

## API Reference

### Status Codes

Every entry point reports success as a non-negative value and failure as a negated
`MIMFS_STATUS_*` code. The numeric values mirror the standard error numbers, so code
that already speaks errno can pass the negated result straight to its reporting layer.
`mimfs_describe_status` turns any code into a stable English description.

### Volume Lifecycle

| Function | Purpose |
| --- | --- |
| `mimfs_format` | Write a fresh volume into a region and mount it |
| `mimfs_mount` | Attach to a volume already present in a region |
| `mimfs_unmount` | Close every descriptor and detach |
| `mimfs_query_volume_statistics` | Geometry and free space accounting |

### File Operations

| Function | Purpose |
| --- | --- |
| `mimfs_open` | Open, optionally creating; returns a descriptor |
| `mimfs_close` | Release a descriptor |
| `mimfs_duplicate_descriptor` | Second descriptor sharing one offset |
| `mimfs_read`, `mimfs_write` | Transfer at the shared offset, advancing it |
| `mimfs_read_at_offset`, `mimfs_write_at_offset` | Transfer at an explicit offset |
| `mimfs_seek` | Reposition the shared offset |
| `mimfs_truncate_by_path`, `mimfs_truncate_by_descriptor` | Set a file's length |
| `mimfs_synchronize` | Publish prior writes to later observers |

### Namespace Operations

| Function | Purpose |
| --- | --- |
| `mimfs_create_directory`, `mimfs_remove_directory` | Directories |
| `mimfs_create_hard_link`, `mimfs_remove_link` | Names |
| `mimfs_create_symbolic_link`, `mimfs_read_symbolic_link` | Symbolic links |
| `mimfs_rename` | Move a name, atomically |

### Metadata and Directory Streams

| Function | Purpose |
| --- | --- |
| `mimfs_get_status`, `mimfs_get_link_status`, `mimfs_get_status_by_descriptor` | Report on an object |
| `mimfs_change_permissions`, `mimfs_change_ownership`, `mimfs_set_times` | Modify metadata |
| `mimfs_open_directory`, `mimfs_read_directory`, `mimfs_rewind_directory`, `mimfs_close_directory` | Enumerate a directory |

### Path Semantics

Paths are absolute. A volume has no working directory, because a working directory is a
property of a thread rather than of a filesystem, and inventing one would make every
operation depend on state shared between threads. Relative symbolic link targets are of
course resolved against the directory holding the link.

## Concurrency Model

### Guarantees

Four guarantees, which are what make external locking unnecessary:

1. **Metadata operations are atomic.** A concurrent observer sees a directory entry
   either fully present or fully absent, never half written.
2. **The shared offset is consumed atomically.** Two threads reading through one
   descriptor receive two disjoint ranges of the file, never the same bytes twice and
   never a skipped range.
3. **Appends do not interleave.** A write to a descriptor opened with
   `MIMFS_OPEN_APPEND` places all of its bytes at the end of the file as one
   indivisible step, with respect to every other append to that file.
4. **Reads never observe a partial write.** Transfers to one file are serialised per
   inode.

Two things are deliberately not guaranteed, matching the behaviour of general purpose
filesystems: a read racing a write to a *different* region of the same file may be
ordered either way, and operations are not transactional across more than one path.

### Worked Example

`examples/mimfs_pipeline.c` is a worked demonstration. Four producer threads and four
consumer threads move two thousand work items through one volume with no mutex, no
condition variable and no work queue in the application. Consumers claim items purely
by racing to create a marker file with `MIMFS_OPEN_EXCLUSIVE`, and report results by
appending to one shared file. The program then asserts that every item was produced,
every item was consumed exactly once, and every result was recorded.

### Lock Granularity

There is no global lock on the data path. What guards what:

| Object | Guarded by |
| --- | --- |
| One inode and its block map | A reader/writer lock inside that inode |
| One block group's free space | Nothing; allocation is lock-free |
| One open file description's offset | A spin lock inside that description |
| Descriptor and description tables | Nothing; slots come from lock-free bitmaps |
| Directory restructuring by rename | One volume-wide lock, taken only by rename |

The reader/writer lock is eight bytes and the spin lock is four, built on C11 atomics.
A platform reader/writer lock is between 56 and 200 bytes depending on the system; one
per inode would have inflated the inode from four cache lines to more than a dozen and
made the inode table several times larger for the same volume. Both locks are correctly
initialised by zeroing, which is what lets a whole inode table be prepared in one pass.

Lock acquisition order is fixed for the entire filesystem and stated in
`src/support/mimfs_synchronization.h`. Path resolution never holds two inode locks at
once: it read-locks one directory, takes a reference on the child, then unlocks before
descending, so a path walk can never participate in a cycle no matter what the caller's
path text is. Rename is the one operation that must hold two directories at once, and it
takes the volume-wide rename lock first, which collapses the whole class of
parent-and-child inversion cycles that moving a directory would otherwise create.

## Architecture

### Region Layout

```
+--------------------------------------+ offset 0, cache line aligned
| volume header                        | geometry, counters, global locks
+--------------------------------------+
| open file description table          | volatile, rebuilt on every mount
+--------------------------------------+
| descriptor table and its bitmaps     | volatile, rebuilt on every mount
+--------------------------------------+
| block group descriptor table         | one cache line per group
+--------------------------------------+
| inode allocation bitmaps             | all groups, back to back
+--------------------------------------+
| block allocation bitmaps             | all groups, back to back
+--------------------------------------+
| inode table                          | all groups, back to back
+--------------------------------------+
| data blocks                          | the remainder
+--------------------------------------+
```

Every region is located by a byte offset from the base of the volume, never by an
absolute pointer. The volume therefore contains no self-references: it can be mapped at
a different address by a different process, or relocated wholesale with one copy, and
still mount.

The metadata of all groups is gathered into shared regions rather than interleaved with
each group's data. Two properties follow: a sequential walk of the inode table, which is
what a whole-tree traversal performs, reads contiguous memory instead of striding across
the volume; and the data area is one unbroken run, so a large file receives one long
contiguous extent.

Persistent state is the geometry, the free counters, the group descriptors, both
bitmaps, the inode table and the data blocks. Volatile state is every lock word, every
reference count, and both open file tables. Mounting rewrites all of the volatile state,
which is why a volume left behind by a process that died mid-operation mounts cleanly
with no descriptor leaked and no lock stuck held.

### Cache Alignment

Alignment here is not decoration. Three structures have their sizes fixed by static
assertion, and the build fails if a field is ever added without adjusting the reserved
area:

- **A directory entry is exactly one cache line, 64 bytes.** A lookup walks these back
  to back, and the fields are ordered by the sequence the comparison uses: a 32-bit name
  hash first, which rejects every candidate but the intended one without touching the
  name at all, then the length, then the name. The name field size, and therefore the
  52-byte name limit, is *derived* from the line size rather than chosen.
- **An inode is exactly four cache lines, 256 bytes.** The division is by access
  pattern, not by meaning: line 0 holds the lock and the metadata almost every
  operation touches, line 1 holds the block map roots, line 2 is reserved so the format
  can grow without moving any inode, line 3 holds the timestamps. A read of a small file
  touches lines 0 and 1 and leaves the timestamps out of cache; a stat touches lines 0
  and 3 and leaves the pointer array out. Two inodes never share a line, so two threads
  mutating two different inodes never contend.
- **A block group descriptor is exactly one cache line.** This is the single most
  important piece of padding in the format. Without it the free counters of sixteen
  groups would share one line, and the lock-free allocator would be slower than a
  locked one.

### Lock-Free Allocation

Claiming a block is three steps, each one atomic operation:

1. Claim one unit of the volume-wide free counter. A compare-and-exchange that refuses
   to go below zero, so the number of threads that get past this point can never exceed
   the number of free blocks.
2. Claim one unit of the chosen group's free counter. This narrows admission to a group
   that certainly has a free block.
3. Claim a bit in that group's bitmap slice. Step two guarantees this succeeds.

Every step is reversible and each failure path undoes the ones before it, so a request
that cannot be satisfied leaves the accounting exactly as it found it. The bitmap scan
rejects a fully allocated 64-bit word in one comparison and locates the first free bit
inside a word with a single count-trailing-zeros instruction, and the claim itself is a
`fetch_or` that both sets the bit and reports whether this thread was the one that set
it.

Locality is the whole reason the group structure exists. An allocation request carries
the number of a block the file already owns, and the search starts in that block's group
at the word just past it, so a file written sequentially receives a run of consecutive
blocks even while other threads allocate concurrently. A request with no locality to
offer starts from a rotor that advances on every use, so unrelated threads begin in
different groups instead of piling onto group zero.

### Block Map

An inode holds fifteen pointer slots:

| Slots | Reach | Indirection |
| --- | --- | --- |
| 0 to 11 | logical blocks 0 to 11 | none |
| 12 | the next 1024 blocks | one level |
| 13 | the next 1048576 blocks | two levels |
| 14 | the next 1073741824 blocks | three levels |

The first 48 KiB of any file costs one array index with no indirect block read at all.
That asymmetry is the point: real trees are dominated by small files, and a small file
must not pay for the machinery a large one needs.

Extent trees were considered and rejected. An extent tree wins on a rotating disk, where
it turns many seeks into one. Here there is no seek to amortise, and the property that
matters instead is the cost of translating one logical index. This map does it in at
most three dependent loads with no search and no comparison; an extent tree would need a
binary search through a variable number of records for the same answer.

Files are sparse. A logical block with no physical block behind it is a hole, reads back
as zeroes and consumes nothing, which is why a file's length and the blocks charged to
it are independent numbers. Seeking four gigabytes past the end of a file and writing one
byte costs one data block and two indirect blocks.

A block that a write covers from its first byte to its last is not cleared before the
copy, because every byte of it is about to be overwritten. Any other newly allocated
block is cleared, otherwise the untouched part would expose whatever the previous owner
of the block left behind. This is a correctness requirement, not an optimisation, and it
is why the allocator takes an initialisation policy from its caller.

### Directory Format

A directory is a file whose content is an array of fixed-size 64-byte entry slots, 64
per block, with a zero inode number marking a slot free.

Fixed slots rather than the packed variable-length records a disk format would use, and
the reason is the access pattern. With fixed slots the address of the next candidate is
known before the current one has been read, so the hardware prefetcher runs ahead of the
scan and 64 candidates are examined per block with no dependent loads. Packed records
would make each candidate's position depend on the length of the one before it, turning
the scan into a chain of dependent loads to save memory this filesystem is not short of.

Removal is O(1) and leaves a free slot rather than compacting, so a churned directory
does not repeatedly rewrite itself. Trailing blocks that become entirely free are
released, so a directory that shrinks for good gives its space back. Every directory
begins with `.` and `..` as ordinary entries, which is what lets the resolver treat them
like any other name and needs no special case.

**Lookup and insertion are linear in the number of entries**, and this is the one place
where the design has a known ceiling rather than a trade-off. A lookup in a directory of
one costs about 140 nanoseconds; in a directory of twenty thousand it costs about
twenty-four microseconds. The constant is very small, because the scan is
prefetcher-friendly and rejects candidates with a single integer comparison, but it is
still O(n). Directories holding more than a few thousand entries will want a hashed
index; the entry already carries the name hash such an index would key on, and the
reserved byte in the entry header is there to flag it. Until then, a workload with very
wide directories should shard them, exactly as one would on a filesystem without a
directory index.

### Inode Lifetime

An inode is reachable in two independent ways and lives until both are gone: its hard
link count, being how many directory entries name it, and its reference count, being how
many descriptors and in-flight path lookups hold it. Removing the last name of a file
that is still open therefore does not destroy it. The name disappears from the namespace
immediately; the blocks stay; both are released when the last descriptor closes.

Each inode also carries a generation number, incremented every time its number is handed
out again. A descriptor remembers the generation it opened, so a stale reference is
refused rather than silently following an inode number into an unrelated file.

## Format Limits

| Property | Value | Why |
| --- | --- | --- |
| Block size | 4096 bytes | Matches the host page size; a block never straddles a page and a full block copy is always a whole number of cache lines |
| Inode size | 256 bytes | Four cache lines exactly |
| Largest file | 4 TiB | 12 direct plus three indirect trees of 1024 pointers |
| Longest name | 52 bytes | Derived: 64-byte entry less the 12-byte header |
| Longest path | 4096 bytes | |
| Hard links per file | 65000 | Below the 16-bit counter maximum, so the overflow check never reasons about wraparound |
| Symbolic link depth | 8 | Turns a link cycle into a clean loop error |
| Inline symbolic link | 60 bytes | Fits where the block pointers go, so a short link costs no block |
| Block group span | 16 MiB target | Compromise between fixed per-group cost and number of independent allocation arenas |
| Smallest volume | About 256 KiB with the defaults | Smaller volumes work with a reduced `maximum_open_file_count` |

Geometry is chosen at format time by a layout engine that searches for the group count
yielding the most data blocks. Only the final group may be short, which lets a volume of
any size use essentially all the memory it was given rather than rounding the data area
down to a multiple of the group span. Every parameter of
`struct mimfs_format_parameters` may be left at zero to take its documented default.

## Benchmarks

`make benchmark` reports five things: sequential transfer, small transfer to isolate
per-call overhead, metadata operation rates, path resolution against directory size, and
parallel scaling. The scaling measurement runs each workload twice, once with a private
directory per thread and once with one shared directory, because those separate two
different costs: whether allocation scales, and what the serialisation on a single hot
directory costs.

Figures depend entirely on the machine's memory bandwidth and core count, so none are
quoted here. Run it on the target.

## Testing and Verification

The test suite is 1749 assertions across five groups: volume geometry and mounting, file
transfers and descriptors, directories, the namespace, and concurrency. It runs clean
under three configurations.

```sh
make tests           # 1749 checks, 0 failures
make tests-strict    # the same, under the address and undefined behaviour sanitizers
```

The thread sanitizer is run separately because it cannot be combined with the address
sanitizer.

```sh
make clean
make OPTIMISATION_FLAGS="-O1 -g -fsanitize=thread" LDFLAGS="-fsanitize=thread" \
     build/mimfs-tests
./build/mimfs-tests   # 0 data races
```

The concurrency group asserts properties that hold regardless of how threads interleave,
rather than asserting a particular interleaving: that accounting closes, meaning every
block and inode taken is given back; that no append reported as successful is missing
from the file; that exactly one thread wins each exclusive creation; and that no record
is ever torn.

The library compiles with no warnings under `-Wall -Wextra -Werror` plus `-Wshadow`,
`-Wcast-qual`, `-Wcast-align`, `-Wconversion`, `-Wsign-conversion`, `-Wpointer-arith`,
`-Wstrict-prototypes`, `-Wmissing-prototypes`, `-Wredundant-decls`, `-Wundef`,
`-Wwrite-strings`, `-Wvla` and `-Wdouble-promotion`.

## Project Structure

```
include/mimfs/          the public interface, the only headers a caller needs
  mimfs.h                 every entry point
  mimfs_types.h           structures, flags and status codes
  mimfs_limits.h          the geometry and limits of the format

src/support/            no knowledge of the filesystem
  mimfs_compiler.h        branch hints, alignment, prefetch, spin-wait
  mimfs_synchronization   the reader/writer lock and the spin lock
  mimfs_bitmap            the lock-free allocation bitmap
  mimfs_clock             the one source of timestamps
  mimfs_name              name validation, hashing and comparison

src/core/               the filesystem, in dependency order
  mimfs_layout.h          every structure, and the format's static assertions
  mimfs_volume            the layout engine, format and mount
  mimfs_block_allocator   data block allocation
  mimfs_inode_allocator   inode allocation and placement
  mimfs_inode             one inode: locking, stamping, lifetime
  mimfs_block_map         logical to physical translation, range release
  mimfs_file_io           byte transfers, holes, length control
  mimfs_directory         entry lookup, insertion, removal, iteration
  mimfs_path_resolver     text to inode, safely, under concurrent change
  mimfs_namespace         create, remove, link, rename, metadata
  mimfs_file_table        descriptors, descriptions, shared offsets

src/api/                the boundary
  mimfs_api.c             argument validation and dispatch
  mimfs_diagnostics.c     status descriptions and version

tests/                  five groups plus a small harness
examples/               an interactive shell and a concurrent pipeline
benchmarks/             throughput and scaling
```

The layering is strict and one-directional. `support` knows nothing of the filesystem.
Each `core` module depends only on modules above it in that list. `api` is the only layer
that validates caller arguments; past it, every module states its preconditions in its
header and relies on them rather than re-checking. `mimfs_layout.h` is the only place
that performs address arithmetic on the volume, so a change to the region layout is
confined to one file.

## License

MIT. See [LICENSE](LICENSE).
