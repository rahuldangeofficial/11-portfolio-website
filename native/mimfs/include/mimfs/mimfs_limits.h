/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    include/mimfs/mimfs_limits.h
 * Purpose: Compile time geometry, limits and tunable constants of the volume
 *          format. Everything that determines the on-memory layout lives here
 *          so that a reader can audit the whole format budget in one place.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#ifndef MIMFS_LIMITS_H
#define MIMFS_LIMITS_H

/* -------------------------------------------------------------------------
 * Machine geometry
 * ------------------------------------------------------------------------- */

/*
 * Size of a single cache line, in bytes. Every structure that is written by
 * more than one thread is padded to a multiple of this value so that two
 * independent objects can never share a line and generate false sharing.
 *
 * 64 bytes is the line size of every mainstream 64 bit CPU family in service
 * today (x86-64, AArch64, POWER9 sector, RISC-V application profiles).
 */
#define MIMFS_CACHE_LINE_SIZE_BYTES          64u

/* -------------------------------------------------------------------------
 * Block geometry
 * ------------------------------------------------------------------------- */

/*
 * Base two logarithm of the block size. Keeping the block size a power of two
 * turns every byte-offset to block-index conversion into a shift and every
 * intra-block offset computation into a mask, which removes integer division
 * from the whole read and write path.
 *
 * 4096 bytes matches the page size of the host and the block size that
 * general purpose filesystems converged on, so a block never straddles a page
 * boundary and a full block copy is always a multiple of a cache line.
 */
#define MIMFS_BLOCK_SIZE_LOG2                12u
#define MIMFS_BLOCK_SIZE_BYTES               (1u << MIMFS_BLOCK_SIZE_LOG2)
#define MIMFS_BLOCK_OFFSET_MASK              ((uint64_t)MIMFS_BLOCK_SIZE_BYTES - 1u)

/* Block number 0 is never handed out; it is the "there is no block" sentinel. */
#define MIMFS_UNALLOCATED_BLOCK_NUMBER       0u

/* -------------------------------------------------------------------------
 * Inode geometry
 * ------------------------------------------------------------------------- */

/*
 * An inode occupies exactly four cache lines. The division of the four lines
 * is documented on the structure definition itself; the important consequence
 * of the size being a power of two multiple of the line size is that two
 * inodes never share a cache line, so two threads mutating two different
 * inodes never contend.
 */
#define MIMFS_INODE_SIZE_BYTES               256u

/*
 * Reserved inode numbers, following the convention that general purpose
 * filesystems use:
 *
 *   0  never valid, used as the "no inode" sentinel
 *   1  reserved for bad block accounting, never reachable from the namespace
 *   2  the root directory
 */
#define MIMFS_INVALID_INODE_NUMBER           0u
#define MIMFS_BAD_BLOCK_INODE_NUMBER         1u
#define MIMFS_ROOT_INODE_NUMBER              2u
#define MIMFS_FIRST_ALLOCATABLE_INODE_NUMBER 3u

/* -------------------------------------------------------------------------
 * Block map geometry
 * ------------------------------------------------------------------------- */

/* Width of one block pointer stored inside an inode or an indirect block. */
#define MIMFS_BLOCK_POINTER_SIZE_BYTES       4u

/* Number of block pointers that address file data without any indirection. */
#define MIMFS_DIRECT_BLOCK_POINTER_COUNT     12u

/* Slot indices of the three indirect roots inside the inode pointer array. */
#define MIMFS_SINGLE_INDIRECT_POINTER_SLOT   12u
#define MIMFS_DOUBLE_INDIRECT_POINTER_SLOT   13u
#define MIMFS_TRIPLE_INDIRECT_POINTER_SLOT   14u

/* Total number of pointer slots inside an inode. */
#define MIMFS_INODE_BLOCK_POINTER_SLOT_COUNT 15u

/* Deepest indirection level that the block map supports. */
#define MIMFS_MAXIMUM_INDIRECTION_LEVEL      3u

/* How many pointers a single indirect block can hold. */
#define MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK                                        \
    ((uint64_t)MIMFS_BLOCK_SIZE_BYTES / MIMFS_BLOCK_POINTER_SIZE_BYTES)

/*
 * Largest logical block index that the block map can express, which is the sum
 * of the reach of the direct pointers and of the three indirect trees.
 */
#define MIMFS_MAXIMUM_LOGICAL_BLOCK_COUNT                                              \
    ((uint64_t)MIMFS_DIRECT_BLOCK_POINTER_COUNT +                                      \
     MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK +                                         \
     MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK *                                         \
         MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK +                                     \
     MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK *                                         \
         MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK *                                     \
         MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK)

/* Largest file size the block map can address, in bytes. */
#define MIMFS_MAXIMUM_FILE_SIZE_BYTES                                                  \
    (MIMFS_MAXIMUM_LOGICAL_BLOCK_COUNT * (uint64_t)MIMFS_BLOCK_SIZE_BYTES)

/* -------------------------------------------------------------------------
 * Namespace limits
 * ------------------------------------------------------------------------- */

/*
 * Longest single path component, in bytes, excluding the terminating NUL.
 *
 * The value is derived, not chosen: a directory entry is pinned to exactly one
 * cache line so that a directory scan touches one line per candidate and never
 * splits an entry across two lines. Twelve of those bytes carry the entry
 * header, which leaves the remainder for the name.
 */
#define MIMFS_DIRECTORY_ENTRY_SIZE_BYTES        64u
#define MIMFS_DIRECTORY_ENTRY_HEADER_SIZE_BYTES 12u
#define MIMFS_MAXIMUM_FILE_NAME_LENGTH                                                 \
    (MIMFS_DIRECTORY_ENTRY_SIZE_BYTES - MIMFS_DIRECTORY_ENTRY_HEADER_SIZE_BYTES)

/* Number of directory entry slots that fit in one block. */
#define MIMFS_DIRECTORY_ENTRIES_PER_BLOCK                                              \
    ((uint64_t)MIMFS_BLOCK_SIZE_BYTES / MIMFS_DIRECTORY_ENTRY_SIZE_BYTES)

/* Longest complete path accepted by the resolver, including the NUL. */
#define MIMFS_MAXIMUM_PATH_LENGTH                    4096u

/*
 * Upper bound on how many symbolic links a single resolution may traverse.
 * The bound is what turns an unbounded link cycle into a clean loop error.
 */
#define MIMFS_MAXIMUM_SYMBOLIC_LINK_RESOLUTION_DEPTH 8u

/*
 * Highest hard link count an inode may reach. The counter itself is 16 bits
 * wide; the limit is kept below the type maximum so that the overflow check
 * never has to reason about wraparound.
 */
#define MIMFS_MAXIMUM_HARD_LINK_COUNT                65000u

/*
 * Number of bytes of symbolic link target that fit inside the inode pointer
 * array. Targets that fit are stored in the inode itself and cost no data
 * block at all, which is how the overwhelming majority of real links behave.
 */
#define MIMFS_INLINE_SYMBOLIC_LINK_CAPACITY_BYTES                                      \
    (MIMFS_INODE_BLOCK_POINTER_SLOT_COUNT * MIMFS_BLOCK_POINTER_SIZE_BYTES)

/* -------------------------------------------------------------------------
 * Block group geometry
 * ------------------------------------------------------------------------- */

/*
 * Free space is not tracked by one global structure. The volume is cut into
 * block groups, each owning a private slice of the block bitmap, of the inode
 * bitmap and of the inode table, plus private free counters. Two threads that
 * allocate out of two different groups share no cache line, so allocation
 * scales with the number of groups rather than serialising on one hot word.
 */
#define MIMFS_TARGET_BLOCK_GROUP_SPAN_BYTES     (16u * 1024u * 1024u)
#define MIMFS_MAXIMUM_BLOCK_GROUP_COUNT         4096u

/*
 * Blocks and inodes per group are both kept a multiple of the bitmap word
 * width, so every group bitmap is a whole number of 64 bit words and the
 * word-parallel scan never needs a partial-word mask.
 */
#define MIMFS_BITMAP_WORD_BIT_COUNT             64u
#define MIMFS_MINIMUM_BLOCKS_PER_BLOCK_GROUP    64u
#define MIMFS_MAXIMUM_BLOCKS_PER_BLOCK_GROUP    32768u
#define MIMFS_MINIMUM_INODES_PER_BLOCK_GROUP    64u

/* -------------------------------------------------------------------------
 * Format defaults
 * ------------------------------------------------------------------------- */

/*
 * One inode is provisioned per this many bytes of data area. The default
 * matches the ratio that general purpose filesystems use for mixed workloads:
 * small enough that a tree of small files does not exhaust inodes first, large
 * enough that the inode table stays well under two percent of the volume.
 */
#define MIMFS_DEFAULT_BYTES_PER_INODE_RATIO     16384u

/* Default size of the open file tables created at format time. */
#define MIMFS_DEFAULT_MAXIMUM_OPEN_FILE_COUNT   1024u
#define MIMFS_MINIMUM_MAXIMUM_OPEN_FILE_COUNT   8u
#define MIMFS_MAXIMUM_MAXIMUM_OPEN_FILE_COUNT   (1u << 20)

/*
 * Percentage of the data area held back from unprivileged allocation. The
 * reserve keeps a full volume repairable: a directory entry can still be
 * removed, and a truncation can still write back a partial tail block, after
 * ordinary writes have started to fail.
 */
#define MIMFS_DEFAULT_RESERVED_BLOCK_PERCENTAGE 5u
#define MIMFS_MAXIMUM_RESERVED_BLOCK_PERCENTAGE 50u

/* -------------------------------------------------------------------------
 * Contention tuning
 * ------------------------------------------------------------------------- */

/*
 * How many times a lock acquisition spins on the CPU relax instruction before
 * it yields the remainder of its scheduling quantum. Spinning wins when the
 * holder is on another core and about to release; yielding wins when the
 * holder has been descheduled. The threshold is the crossover point measured
 * for critical sections of a few hundred cycles, which is what every critical
 * section in this filesystem is.
 */
#define MIMFS_LOCK_SPIN_ITERATIONS_BEFORE_YIELD 64u

#endif /* MIMFS_LIMITS_H */
