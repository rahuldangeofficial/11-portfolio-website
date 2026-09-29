/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_block_allocator.c
 * Purpose: Implementation of the lock free data block allocator.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_block_allocator.h"

#include <string.h>

#include "support/mimfs_bitmap.h"

/*
 * Slice of the block bitmap owned by one group.
 *
 * A group's first block number is a multiple of the group span, and the span is a
 * multiple of the bitmap word width, so the slice always begins on a word
 * boundary. That is what allows a group to be handed to the bitmap engine as an
 * independent bitmap of its own, with local bit indices, instead of needing a
 * range limited variant of every bitmap operation.
 */
static _Atomic uint64_t *mimfs_block_bitmap_slice_of_group(struct mimfs_volume *volume,
                                                           uint64_t first_block_number)
{
    return mimfs_block_allocation_bitmap(volume) +
           (first_block_number / MIMFS_BITMAP_WORD_BIT_COUNT);
}

/*
 * Try to take one block out of one specific group.
 *
 * Returns the block number, or MIMFS_UNALLOCATED_BLOCK_NUMBER when the group had
 * nothing to give. The caller has already passed the volume-wide admission gate,
 * so a failure here means only that this particular group is full.
 */
static uint64_t mimfs_take_block_from_group(struct mimfs_volume *volume,
                                            uint32_t group_index,
                                            uint64_t local_search_hint)
{
    struct mimfs_block_group_descriptor *group =
        mimfs_block_group_descriptor_at(volume, group_index);

    /*
     * Reserve the block in the group's counter before touching its bitmap. Doing
     * it in this order is what makes the bitmap scan below guaranteed to find a
     * free bit, which in turn is what allows the scan to run with no lock and no
     * retry against other groups.
     */
    if (!mimfs_atomic_decrement_if_positive(&(*group).free_block_count))
    {
        return MIMFS_UNALLOCATED_BLOCK_NUMBER;
    }

    const uint64_t first_block_number = (*group).first_block_number;
    _Atomic uint64_t *group_bitmap =
        mimfs_block_bitmap_slice_of_group(volume, first_block_number);

    const uint64_t claimed_local_index = mimfs_bitmap_claim_first_free_bit(
        group_bitmap, (*volume).blocks_per_block_group, local_search_hint);

    if (MIMFS_UNLIKELY(claimed_local_index == MIMFS_BITMAP_NO_FREE_BIT))
    {
        /*
         * The counter promised a block the bitmap does not have. That is only
         * possible on a volume whose accounting has been damaged, so the
         * reservation is handed straight back rather than trusted.
         */
        mimfs_atomic_increment(&(*group).free_block_count);
        return MIMFS_UNALLOCATED_BLOCK_NUMBER;
    }

    /*
     * Move the hint one past the block just taken. The store is unsynchronised on
     * purpose: a hint that another thread overwrites costs at most a few wasted
     * word comparisons on the next search, and paying for a read modify write
     * here would put a contended atomic back into the very path the group
     * structure exists to keep uncontended.
     */
    atomic_store_explicit(&(*group).next_block_search_hint, claimed_local_index + 1u,
                          memory_order_relaxed);

    return first_block_number + claimed_local_index;
}

uint64_t mimfs_block_allocator_available_block_count(struct mimfs_volume *volume)
{
    const uint64_t free_block_count =
        atomic_load_explicit(&(*volume).free_block_count, memory_order_relaxed);
    const uint64_t reserved_block_count = (*volume).reserved_block_count;

    return free_block_count > reserved_block_count
               ? free_block_count - reserved_block_count
               : 0u;
}

uint64_t mimfs_block_allocator_allocate(
    struct mimfs_volume *volume, uint64_t locality_hint_block_number,
    enum mimfs_block_initialization_policy initialization_policy,
    enum mimfs_block_reserve_policy reserve_policy)
{
    const uint32_t group_count = (*volume).block_group_count;

    /*
     * The reserve is a threshold rather than a second counter, so this test is
     * approximate under concurrency: a handful of threads may cross it together
     * and dip a few blocks into the reserve. That is the correct trade. Making it
     * exact would require a second volume-wide atomic on the allocation path, and
     * the reserve exists to leave room for cleanup, a purpose a few blocks of
     * slack does not defeat.
     */
    if (reserve_policy == MIMFS_BLOCK_RESPECT_RESERVE &&
        mimfs_block_allocator_available_block_count(volume) == 0u)
    {
        return MIMFS_UNALLOCATED_BLOCK_NUMBER;
    }

    /* Volume-wide admission gate. */
    if (!mimfs_atomic_decrement_if_positive(&(*volume).free_block_count))
    {
        return MIMFS_UNALLOCATED_BLOCK_NUMBER;
    }

    uint32_t first_group_to_try;
    uint64_t local_search_hint;

    if (mimfs_block_number_is_in_range(volume, locality_hint_block_number))
    {
        /*
         * Continue immediately after the block the file already owns, so that a
         * sequentially written file collects a contiguous run.
         */
        first_group_to_try =
            mimfs_group_index_of_block(volume, locality_hint_block_number);
        local_search_hint = (locality_hint_block_number &
                             ((uint64_t)(*volume).blocks_per_block_group - 1u)) +
                            1u;
    }
    else
    {
        /*
         * No locality to preserve. Spread instead: the rotor hands consecutive
         * callers different starting groups, which keeps unrelated threads off
         * each other's counters.
         */
        const uint64_t rotor_value = atomic_fetch_add_explicit(
            &(*volume).allocation_rotor, 1u, memory_order_relaxed);

        first_group_to_try = (uint32_t)(rotor_value % group_count);
        local_search_hint = 0u;
    }

    for (uint32_t attempt_index = 0u; attempt_index < group_count; attempt_index++)
    {
        const uint32_t group_index = (first_group_to_try + attempt_index) % group_count;

        /* Only the first group tried inherits the fine grained hint; the
         * fallbacks start at their own remembered position. */
        uint64_t hint_for_this_group = local_search_hint;

        if (attempt_index != 0u)
        {
            struct mimfs_block_group_descriptor *group =
                mimfs_block_group_descriptor_at(volume, group_index);

            hint_for_this_group = atomic_load_explicit(&(*group).next_block_search_hint,
                                                       memory_order_relaxed);
        }

        const uint64_t allocated_block_number =
            mimfs_take_block_from_group(volume, group_index, hint_for_this_group);

        if (MIMFS_LIKELY(allocated_block_number != MIMFS_UNALLOCATED_BLOCK_NUMBER))
        {
            if (initialization_policy == MIMFS_BLOCK_INITIALIZE_TO_ZERO)
            {
                memset(mimfs_block_address(volume, allocated_block_number), 0,
                       MIMFS_BLOCK_SIZE_BYTES);
            }

            return allocated_block_number;
        }
    }

    /* Every group refused. Undo the volume-wide reservation. */
    mimfs_atomic_increment(&(*volume).free_block_count);

    return MIMFS_UNALLOCATED_BLOCK_NUMBER;
}

void mimfs_block_allocator_release(struct mimfs_volume *volume, uint64_t block_number)
{
    if (MIMFS_UNLIKELY(!mimfs_block_number_is_in_range(volume, block_number)))
    {
        return;
    }

    const uint32_t group_index = mimfs_group_index_of_block(volume, block_number);
    struct mimfs_block_group_descriptor *group =
        mimfs_block_group_descriptor_at(volume, group_index);

    const uint64_t first_block_number = (*group).first_block_number;
    _Atomic uint64_t *group_bitmap =
        mimfs_block_bitmap_slice_of_group(volume, first_block_number);

    /*
     * The bitmap is the authority on whether this block was really allocated. A
     * double release is caught here, by the bit already being clear, and the free
     * counters are left untouched in that case: inflating them would hand the same
     * block to two files, which is unrecoverable, whereas leaking the release is
     * merely a lost block.
     */
    if (MIMFS_UNLIKELY(
            !mimfs_bitmap_release_bit(group_bitmap, block_number - first_block_number)))
    {
        return;
    }

    /*
     * Point the group's next search at the block just freed, so that a released
     * block is the first candidate handed out again. Reusing a hot block keeps the
     * working set of a busy directory or a rewritten file inside the cache.
     */
    atomic_store_explicit(&(*group).next_block_search_hint,
                          block_number - first_block_number, memory_order_relaxed);

    mimfs_atomic_increment(&(*group).free_block_count);
    mimfs_atomic_increment(&(*volume).free_block_count);
}
