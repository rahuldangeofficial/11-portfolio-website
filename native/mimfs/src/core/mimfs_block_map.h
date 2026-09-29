/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_block_map.h
 * Purpose: Translation from a logical block index inside a file to the physical
 *          block that holds it, and release of the blocks behind a range.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * The shape of the map
 * ---------------------------------------------------------------------------
 * An inode holds fifteen pointer slots. The first twelve address blocks directly.
 * The last three are the roots of trees of increasing depth:
 *
 *   slots 0 to 11   logical blocks 0 to 11              no indirection
 *   slot 12         the next 1024 blocks                one level
 *   slot 13         the next 1048576 blocks             two levels
 *   slot 14         the next 1073741824 blocks          three levels
 *
 * The reach is four terabytes, and the cost of the first 48 kilobytes of any file
 * is one array index with no indirect block read at all. That asymmetry is the
 * entire point: real trees are dominated by small files, and a small file must not
 * pay for the machinery that a large one needs.
 *
 * ---------------------------------------------------------------------------
 * Why not extents
 * ---------------------------------------------------------------------------
 * An extent tree wins on a rotating disk, where it turns many seeks into one. Here
 * the backing store is memory, so there is no seek to amortise, and the property
 * that matters instead is the cost of translating one logical index. This map does
 * it in at most three dependent loads, each from a block the allocator has already
 * placed near the others, with no search and no comparison. An extent tree would
 * need a binary search through a variable number of records for the same answer.
 *
 * ---------------------------------------------------------------------------
 * Locking
 * ---------------------------------------------------------------------------
 * Every function here requires the caller to hold the inode lock: shared is enough
 * for a pure lookup, exclusive is required for anything that may allocate or free.
 * Nothing in this module takes a lock of its own.
 */

#ifndef MIMFS_BLOCK_MAP_H
#define MIMFS_BLOCK_MAP_H

#include "mimfs_block_allocator.h"
#include "mimfs_layout.h"

/*
 * Whether a lookup may create the blocks it needs.
 *
 * A read never allocates: a logical block with no physical block behind it is a
 * hole, and a hole reads as zeroes. A write allocates both the data block and any
 * indirect block on the path to it.
 */
enum mimfs_block_map_lookup_mode
{
    MIMFS_BLOCK_MAP_LOOKUP_ONLY = 0,
    MIMFS_BLOCK_MAP_LOOKUP_AND_ALLOCATE = 1
};

/*
 * Position of one logical block within the map: which root slot leads to it, how
 * deep the tree below that slot is, and the index to take at each level.
 *
 * Decomposing the index into this explicit form is what allows one loop to walk
 * every depth, instead of three near-identical special cases for the single,
 * double and triple indirect trees.
 */
struct mimfs_block_map_position
{
    uint32_t root_pointer_slot;
    uint32_t indirection_level;
    uint32_t index_at_level[MIMFS_MAXIMUM_INDIRECTION_LEVEL];
};

/*
 * Work out where a logical block index lives in the map.
 *
 * Returns MIMFS_STATUS_SUCCESS, or MIMFS_STATUS_FILE_TOO_LARGE when the index is
 * beyond what the map can express.
 */
int mimfs_block_map_decompose_logical_index(
    uint64_t logical_block_index, struct mimfs_block_map_position *position_out);

/*
 * Find, and optionally create, the physical block holding a logical block.
 *
 * On success, *physical_block_number_out is the block number, or
 * MIMFS_UNALLOCATED_BLOCK_NUMBER when the block is a hole and the mode was lookup
 * only.
 *
 * data_block_initialization_policy applies to the data block itself; indirect
 * blocks are always cleared, because an uncleared one would be read as a table of
 * pointers to blocks belonging to other files.
 *
 * Requires the inode lock: shared for lookup only, exclusive when allocating.
 */
int mimfs_block_map_resolve(
    struct mimfs_volume *volume, struct mimfs_inode *inode,
    uint64_t logical_block_index, enum mimfs_block_map_lookup_mode lookup_mode,
    enum mimfs_block_initialization_policy data_block_initialization_policy,
    enum mimfs_block_reserve_policy reserve_policy,
    uint64_t *physical_block_number_out);

/*
 * Release every block at or after first_logical_block_index_to_release, including
 * any indirect block that becomes empty as a result.
 *
 * Returns the number of blocks released, data and indirect together, which the
 * caller subtracts from the inode's charge.
 *
 * Requires the exclusive inode lock.
 */
uint64_t mimfs_block_map_release_from(struct mimfs_volume *volume,
                                      struct mimfs_inode *inode,
                                      uint64_t first_logical_block_index_to_release);

/*
 * Release the whole map. Equivalent to releasing from index zero, and provided
 * separately because destruction and truncation to zero are the common cases and
 * this form makes the intent obvious at the call site.
 *
 * Requires the exclusive inode lock.
 */
uint64_t mimfs_block_map_release_all_blocks(struct mimfs_volume *volume,
                                            struct mimfs_inode *inode);

#endif /* MIMFS_BLOCK_MAP_H */
