/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_directory.h
 * Purpose: The contents of a directory: looking a name up, inserting one,
 *          removing one, and iterating.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * How a directory is stored
 * ---------------------------------------------------------------------------
 * A directory is an ordinary file whose content is an array of fixed size entry
 * slots, sixty four of them per block. A slot whose inode number is zero is free.
 *
 * Fixed size slots rather than the packed variable length records that a disk
 * format would use, and the reason is the access pattern. A lookup compares one
 * precomputed hash per candidate; with fixed slots the address of the next
 * candidate is known before the current one has been read, so the hardware
 * prefetcher runs ahead of the scan and sixty four candidates are examined per
 * block with no dependent loads at all. Packed records would make the position of
 * each candidate depend on the length of the one before it, turning the scan into a
 * chain of dependent loads for the sake of memory this filesystem is not short of.
 *
 * Removal is O(1) and leaves a free slot behind rather than compacting, so a
 * directory that is churned does not repeatedly rewrite itself. Trailing blocks
 * that become entirely free are released, so a directory that shrinks for good does
 * give its space back.
 *
 * Every directory begins with the two entries "." and "..", inserted when it is
 * created. They are ordinary entries in every respect, which is what lets the
 * resolver treat them like any other name.
 *
 * ---------------------------------------------------------------------------
 * Locking
 * ---------------------------------------------------------------------------
 * The caller holds the lock of the directory inode throughout: shared for lookup
 * and iteration, exclusive for insert and remove. Nothing here locks anything.
 */

#ifndef MIMFS_DIRECTORY_H
#define MIMFS_DIRECTORY_H

#include "mimfs_layout.h"
#include "support/mimfs_name.h"

/* Where a lookup found what it was asked for, and what it found. */
struct mimfs_directory_lookup_result
{
    uint64_t inode_number;
    uint32_t entry_type;

    /* Exact location of the entry, so that a caller which is about to modify it
     * does not have to search for it a second time. */
    uint64_t containing_block_number;
    uint32_t entry_index_within_block;
};

/*
 * Find a name.
 *
 * Returns MIMFS_STATUS_SUCCESS with the result filled in, or
 * MIMFS_STATUS_NO_SUCH_ENTRY.
 *
 * Requires at least the shared lock on the directory inode.
 */
int mimfs_directory_lookup(struct mimfs_volume *volume,
                           struct mimfs_inode *directory_inode,
                           const struct mimfs_name_reference *name,
                           struct mimfs_directory_lookup_result *result_out);

/*
 * Add a name.
 *
 * The scan that looks for a free slot also detects a name that is already present,
 * so both questions are answered by one pass over the directory while the lock is
 * held once. That is not merely an optimisation: checking for a duplicate in a
 * separate earlier pass would leave a window in which another thread could insert
 * the same name.
 *
 * Returns MIMFS_STATUS_SUCCESS, MIMFS_STATUS_ALREADY_EXISTS, or
 * MIMFS_STATUS_NO_SPACE_LEFT.
 *
 * Requires the exclusive lock on the directory inode.
 */
int mimfs_directory_insert(struct mimfs_volume *volume,
                           struct mimfs_inode *directory_inode,
                           const struct mimfs_name_reference *name,
                           uint64_t inode_number, uint32_t entry_type);

/*
 * Remove a name, reporting which inode it named.
 *
 * Returns MIMFS_STATUS_SUCCESS or MIMFS_STATUS_NO_SUCH_ENTRY.
 *
 * Requires the exclusive lock on the directory inode.
 */
int mimfs_directory_remove(struct mimfs_volume *volume,
                           struct mimfs_inode *directory_inode,
                           const struct mimfs_name_reference *name,
                           uint64_t *removed_inode_number_out);

/*
 * Repoint an existing name at a different inode, in place.
 *
 * This is what makes replacing an existing destination during a rename atomic: the
 * name never disappears, it simply refers to something else from one store to the
 * next.
 *
 * Returns MIMFS_STATUS_SUCCESS or MIMFS_STATUS_NO_SUCH_ENTRY.
 *
 * Requires the exclusive lock on the directory inode.
 */
int mimfs_directory_repoint(struct mimfs_volume *volume,
                            struct mimfs_inode *directory_inode,
                            const struct mimfs_name_reference *name,
                            uint64_t new_inode_number, uint32_t new_entry_type,
                            uint64_t *previous_inode_number_out);

/*
 * True when the directory holds nothing but "." and "..".
 *
 * Requires at least the shared lock on the directory inode.
 */
bool mimfs_directory_is_empty(struct mimfs_volume *volume,
                              struct mimfs_inode *directory_inode);

/*
 * Produce the entry at or after the cursor, advancing the cursor past it.
 *
 * The cursor is a slot index, which makes it stable in the only way that matters:
 * an entry inserted or removed elsewhere in the directory never causes an entry the
 * stream has not yet reached to be skipped or repeated.
 *
 * Returns 1 when an entry was produced, 0 at the end of the directory.
 *
 * Requires at least the shared lock on the directory inode.
 */
int mimfs_directory_read_next_entry(
    struct mimfs_volume *volume, struct mimfs_inode *directory_inode,
    uint64_t *entry_cursor, struct mimfs_directory_entry_information *entry_out);

/*
 * Give a freshly allocated directory inode its "." and ".." entries.
 *
 * The link count of the new directory is set to one, accounting for its own ".";
 * the caller adds the second link when it inserts the name of this directory into
 * its parent, and raises the parent's own count for the ".." installed here.
 *
 * Requires the exclusive lock on the new directory inode.
 */
int mimfs_directory_install_dot_entries(struct mimfs_volume *volume,
                                        struct mimfs_inode *directory_inode,
                                        uint64_t directory_inode_number,
                                        uint64_t parent_inode_number);

/*
 * Repoint the ".." entry of a directory that has been moved to a new parent.
 *
 * Requires the exclusive lock on the directory inode.
 */
int mimfs_directory_update_parent_entry(struct mimfs_volume *volume,
                                        struct mimfs_inode *directory_inode,
                                        uint64_t new_parent_inode_number);

/*
 * Build the root directory of a freshly formatted volume.
 *
 * The root is the one inode whose number is fixed by the format rather than chosen
 * by the allocator, and the one directory with no parent, so it is created here
 * rather than through the ordinary directory creation path.
 */
int mimfs_directory_create_root(struct mimfs_volume *volume);

#endif /* MIMFS_DIRECTORY_H */
