/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_block_allocator.h
 * Purpose: Allocation and release of data blocks.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * How a block is claimed without a lock
 * ---------------------------------------------------------------------------
 * Three steps, each of which is a single atomic operation, in this order:
 *
 *   1. Claim one unit of the volume-wide free counter. This is the global
 *      admission gate. Because the claim is a compare and exchange that refuses
 *      to go below zero, the number of threads that get past this point can
 *      never exceed the number of free blocks.
 *   2. Claim one unit of the free counter of a chosen group. This narrows the
 *      admission to a group that certainly has a free block.
 *   3. Claim a bit in that group's slice of the block bitmap. Step two
 *      guarantees this succeeds.
 *
 * Every step is reversible, and the failure path of each undoes the ones before
 * it, so a request that cannot be satisfied leaves the accounting exactly as it
 * found it. No lock is taken at any point, and two threads allocating from two
 * different groups touch no common cache line at all.
 *
 * ---------------------------------------------------------------------------
 * Where a block comes from
 * ---------------------------------------------------------------------------
 * Locality is not incidental, it is the whole reason the group structure exists.
 * A request carries the number of a block the file already owns, and the search
 * starts in that block's group, at the bitmap word just past it. A file written
 * from beginning to end therefore receives a run of consecutive blocks even while
 * other threads are allocating concurrently, which is what makes a sequential
 * read of that file walk memory forwards instead of jumping.
 *
 * A request with no locality to offer starts from a rotor that advances on every
 * use, so unrelated threads begin in different groups rather than all piling into
 * group zero and contending on one counter.
 */

#ifndef MIMFS_BLOCK_ALLOCATOR_H
#define MIMFS_BLOCK_ALLOCATOR_H

#include "mimfs_layout.h"

/*
 * Whether a freshly allocated block must be handed back cleared.
 *
 * Clearing costs a full block of stores, so it is not done unconditionally. It
 * is required in exactly two situations, and the caller is the only one that can
 * tell them apart:
 *
 *   * The block will become part of the block map, where a stale non zero word
 *     would be read as a pointer to a block belonging to another file.
 *   * The block will be only partly written, so the untouched part would
 *     otherwise expose whatever the previous owner of the block left behind.
 *
 * When the caller is about to overwrite every byte of the block, clearing it
 * first would write the whole block twice, and skipping it is not merely an
 * optimisation but the correct behaviour.
 */
enum mimfs_block_initialization_policy
{
    MIMFS_BLOCK_INITIALIZE_TO_ZERO = 0,
    MIMFS_BLOCK_LEAVE_UNINITIALIZED = 1
};

/*
 * Whether the request may dip into the reserve.
 *
 * Ordinary file data may not. Metadata that is needed to make the volume smaller
 * again may: a directory entry has to be insertable even on a full volume,
 * otherwise the volume cannot be cleaned up and the reserve would have no
 * purpose.
 */
enum mimfs_block_reserve_policy
{
    MIMFS_BLOCK_RESPECT_RESERVE = 0,
    MIMFS_BLOCK_MAY_USE_RESERVE = 1
};

/*
 * Allocate one block.
 *
 * locality_hint_block_number  a block the same file already owns, or
 *                             MIMFS_UNALLOCATED_BLOCK_NUMBER when there is none
 *
 * Returns the block number, or MIMFS_UNALLOCATED_BLOCK_NUMBER when the volume is
 * out of space.
 */
uint64_t mimfs_block_allocator_allocate(
    struct mimfs_volume *volume, uint64_t locality_hint_block_number,
    enum mimfs_block_initialization_policy initialization_policy,
    enum mimfs_block_reserve_policy reserve_policy);

/*
 * Return one block to the volume. Releasing a block that is already free is a
 * caller defect; it is detected and ignored rather than being allowed to corrupt
 * the free counters, so that the damage of such a bug stops at one leaked block.
 */
void mimfs_block_allocator_release(struct mimfs_volume *volume, uint64_t block_number);

/* Blocks a caller could still consume, honouring the reserve. */
uint64_t mimfs_block_allocator_available_block_count(struct mimfs_volume *volume);

#endif /* MIMFS_BLOCK_ALLOCATOR_H */
