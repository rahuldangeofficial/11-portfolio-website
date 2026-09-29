/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_inode_allocator.h
 * Purpose: Allocation and release of inodes.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * The three step claim is the same as the block allocator's, for the same
 * reasons; see the commentary there. What differs is placement policy, and the
 * difference matters:
 *
 *   * A regular file is placed in the same group as the directory that will name
 *     it. A directory listing that then stats every entry walks one contiguous
 *     stretch of the inode table instead of touching a different page per entry.
 *
 *   * A directory is deliberately placed in a different group from its parent.
 *     Keeping directories together would concentrate the whole tree in the first
 *     group, and with it every block allocated for files in that tree. Spreading
 *     directories is what keeps the per-group arenas useful as a tree grows.
 */

#ifndef MIMFS_INODE_ALLOCATOR_H
#define MIMFS_INODE_ALLOCATOR_H

#include "mimfs_layout.h"

/*
 * Which placement rule to apply. Expressed as intent rather than as a group
 * number so that the policy lives in one module and callers never compute a
 * group.
 */
enum mimfs_inode_placement_policy
{
    /* Keep the new inode close to the parent directory. */
    MIMFS_INODE_PLACE_NEAR_PARENT = 0,

    /* Push the new inode away from the parent, to spread the tree. */
    MIMFS_INODE_PLACE_AWAY_FROM_PARENT = 1
};

/*
 * Allocate one inode and hand it back fully cleared, except for its generation
 * number, which has been advanced.
 *
 * The generation number is what makes a stale reference detectable. A descriptor
 * remembers the generation it opened; if the inode is freed and the same number is
 * later handed to a different file, the generation no longer matches and the
 * descriptor is refused instead of silently operating on the wrong file.
 *
 * parent_inode_number  the directory that will hold the new entry, or
 *                      MIMFS_INVALID_INODE_NUMBER when there is none
 *
 * Returns the inode number, or MIMFS_INVALID_INODE_NUMBER when the volume has no
 * inode left.
 */
uint64_t
mimfs_inode_allocator_allocate(struct mimfs_volume *volume,
                               uint64_t parent_inode_number,
                               enum mimfs_inode_placement_policy placement_policy);

/*
 * Return one inode to the volume.
 *
 * The caller must already have released every block the inode owned and must hold
 * no reference to it. A release of an inode that is already free is detected and
 * ignored.
 */
void mimfs_inode_allocator_release(struct mimfs_volume *volume, uint64_t inode_number);

/*
 * True when the inode number names an inode that is currently allocated. Used by
 * the validity checks of the descriptor layer, which then re-verify under the
 * inode lock; on its own the answer can go stale immediately.
 */
bool mimfs_inode_allocator_inode_is_allocated(struct mimfs_volume *volume,
                                              uint64_t inode_number);

#endif /* MIMFS_INODE_ALLOCATOR_H */
