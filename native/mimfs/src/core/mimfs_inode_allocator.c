/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_inode_allocator.c
 * Purpose: Implementation of the lock free inode allocator.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_inode_allocator.h"

#include <string.h>

#include "support/mimfs_bitmap.h"
#include "support/mimfs_clock.h"

/*
 * Slice of the inode bitmap owned by one group. Inode numbering starts at one, so
 * the bit index of an inode is one less than its number; the first inode of a
 * group therefore sits at a bit index that is an exact multiple of the group's
 * inode count, which keeps the slice word aligned.
 */
static _Atomic uint64_t *mimfs_inode_bitmap_slice_of_group(struct mimfs_volume *volume,
                                                           uint64_t first_inode_number)
{
    return mimfs_inode_allocation_bitmap(volume) +
           ((first_inode_number - 1u) / MIMFS_BITMAP_WORD_BIT_COUNT);
}

/*
 * Prepare a freshly claimed inode for use.
 *
 * The generation number is read before the clear and written back incremented
 * afterwards, because it is the one field whose whole purpose is to survive the
 * reuse of the inode number.
 *
 * The lock is reinitialised rather than assumed to be free. It must be free, since
 * the inode was on the free list, but rewriting one word is cheaper than the
 * reasoning required to be sure of it at every future change to the code.
 */
static void mimfs_prepare_allocated_inode(struct mimfs_volume *volume,
                                          uint64_t inode_number,
                                          uint32_t home_group_index)
{
    struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

    const uint32_t previous_generation_number = (*inode).generation_number;

    memset(inode, 0, sizeof(*inode));

    (*inode).generation_number = previous_generation_number + 1u;
    (*inode).home_block_group_index = home_group_index;

    mimfs_reader_writer_lock_initialize(&(*inode).content_lock);
    atomic_store_explicit(&(*inode).open_reference_count, 0u, memory_order_relaxed);

    const int64_t creation_time = mimfs_clock_current_time_nanoseconds();

    (*inode).creation_time_nanoseconds = creation_time;
    (*inode).last_modification_time_nanoseconds = creation_time;

    atomic_store_explicit(&(*inode).last_access_time_nanoseconds, creation_time,
                          memory_order_relaxed);
    (*inode).last_status_change_time_nanoseconds = creation_time;

    /*
     * The inode is fully formed before any other thread can reach it, which it
     * cannot do until the caller inserts a directory entry naming it. The fence
     * makes that ordering explicit rather than incidental.
     */
    mimfs_publish_prior_stores();
}

/* Try to take one inode out of one specific group. */
static uint64_t mimfs_take_inode_from_group(struct mimfs_volume *volume,
                                            uint32_t group_index)
{
    struct mimfs_block_group_descriptor *group =
        mimfs_block_group_descriptor_at(volume, group_index);

    if (!mimfs_atomic_decrement_if_positive(&(*group).free_inode_count))
    {
        return MIMFS_INVALID_INODE_NUMBER;
    }

    const uint64_t first_inode_number = (*group).first_inode_number;
    _Atomic uint64_t *group_bitmap =
        mimfs_inode_bitmap_slice_of_group(volume, first_inode_number);

    const uint64_t search_hint =
        atomic_load_explicit(&(*group).next_inode_search_hint, memory_order_relaxed) %
        (*volume).inodes_per_block_group;

    const uint64_t claimed_local_index = mimfs_bitmap_claim_first_free_bit(
        group_bitmap, (*volume).inodes_per_block_group, search_hint);

    if (MIMFS_UNLIKELY(claimed_local_index == MIMFS_BITMAP_NO_FREE_BIT))
    {
        mimfs_atomic_increment(&(*group).free_inode_count);
        return MIMFS_INVALID_INODE_NUMBER;
    }

    atomic_store_explicit(&(*group).next_inode_search_hint, claimed_local_index + 1u,
                          memory_order_relaxed);

    return first_inode_number + claimed_local_index;
}

/*
 * Decide which group to try first.
 *
 * For a file, the answer is the parent's group, so the two inodes end up adjacent
 * in the inode table. For a directory, the answer is deliberately a different
 * group; stepping by a rotor-driven stride rather than by one keeps two threads
 * creating sibling directories at the same instant from choosing the same target.
 */
static uint32_t
mimfs_choose_first_group_to_try(struct mimfs_volume *volume,
                                uint64_t parent_inode_number,
                                enum mimfs_inode_placement_policy placement_policy)
{
    const uint32_t group_count = (*volume).block_group_count;

    uint32_t parent_group_index = 0u;

    if (mimfs_inode_number_is_in_range(volume, parent_inode_number))
    {
        parent_group_index = mimfs_group_index_of_inode(volume, parent_inode_number);
    }

    if (placement_policy == MIMFS_INODE_PLACE_NEAR_PARENT || group_count == 1u)
    {
        return parent_group_index;
    }

    const uint64_t rotor_value = atomic_fetch_add_explicit(&(*volume).allocation_rotor,
                                                           1u, memory_order_relaxed);

    /* Any stride coprime with the group count would do; one plus the rotor is the
     * cheapest such choice and never lands back on the parent. */
    const uint32_t stride = 1u + (uint32_t)(rotor_value % (group_count - 1u));

    return (parent_group_index + stride) % group_count;
}

uint64_t
mimfs_inode_allocator_allocate(struct mimfs_volume *volume,
                               uint64_t parent_inode_number,
                               enum mimfs_inode_placement_policy placement_policy)
{
    const uint32_t group_count = (*volume).block_group_count;

    if (!mimfs_atomic_decrement_if_positive(&(*volume).free_inode_count))
    {
        return MIMFS_INVALID_INODE_NUMBER;
    }

    const uint32_t first_group_to_try =
        mimfs_choose_first_group_to_try(volume, parent_inode_number, placement_policy);

    for (uint32_t attempt_index = 0u; attempt_index < group_count; attempt_index++)
    {
        const uint32_t group_index = (first_group_to_try + attempt_index) % group_count;

        const uint64_t allocated_inode_number =
            mimfs_take_inode_from_group(volume, group_index);

        if (MIMFS_LIKELY(allocated_inode_number != MIMFS_INVALID_INODE_NUMBER))
        {
            mimfs_prepare_allocated_inode(volume, allocated_inode_number, group_index);
            return allocated_inode_number;
        }
    }

    mimfs_atomic_increment(&(*volume).free_inode_count);

    return MIMFS_INVALID_INODE_NUMBER;
}

void mimfs_inode_allocator_release(struct mimfs_volume *volume, uint64_t inode_number)
{
    /*
     * The two reserved numbers are never on the free list, so a release of either
     * is a defect in the caller rather than a condition to handle. Refusing here
     * keeps the reserved numbers reserved for the lifetime of the volume.
     */
    if (MIMFS_UNLIKELY(!mimfs_inode_number_is_in_range(volume, inode_number) ||
                       inode_number == MIMFS_ROOT_INODE_NUMBER))
    {
        return;
    }

    const uint32_t group_index = mimfs_group_index_of_inode(volume, inode_number);
    struct mimfs_block_group_descriptor *group =
        mimfs_block_group_descriptor_at(volume, group_index);

    const uint64_t first_inode_number = (*group).first_inode_number;
    _Atomic uint64_t *group_bitmap =
        mimfs_inode_bitmap_slice_of_group(volume, first_inode_number);

    struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

    /*
     * Wipe the metadata before the bit is cleared, not after. The bit is what
     * publishes the inode as reusable, so every store that has to be in place
     * before another thread can claim it must happen first. The file mode is
     * cleared in particular, so that an inode observed through a stale directory
     * entry reads as having no type at all rather than as a plausible file.
     */
    (*inode).file_mode = 0u;
    (*inode).hard_link_count = 0u;
    (*inode).file_size_in_bytes = 0u;
    (*inode).allocated_block_count = 0u;

    memset(&(*inode).block_map_root, 0, sizeof((*inode).block_map_root));

    mimfs_publish_prior_stores();

    if (MIMFS_UNLIKELY(
            !mimfs_bitmap_release_bit(group_bitmap, inode_number - first_inode_number)))
    {
        /* Already free: a double release. Leave the counters alone so that the
         * same inode cannot be handed to two files. */
        return;
    }

    mimfs_atomic_increment(&(*group).free_inode_count);
    mimfs_atomic_increment(&(*volume).free_inode_count);
}

bool mimfs_inode_allocator_inode_is_allocated(struct mimfs_volume *volume,
                                              uint64_t inode_number)
{
    if (!mimfs_inode_number_is_in_range(volume, inode_number))
    {
        return false;
    }

    const uint32_t group_index = mimfs_group_index_of_inode(volume, inode_number);
    struct mimfs_block_group_descriptor *group =
        mimfs_block_group_descriptor_at(volume, group_index);

    _Atomic uint64_t *group_bitmap =
        mimfs_inode_bitmap_slice_of_group(volume, (*group).first_inode_number);

    return mimfs_bitmap_bit_is_set(group_bitmap,
                                   inode_number - (*group).first_inode_number);
}
