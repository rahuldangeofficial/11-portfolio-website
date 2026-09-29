/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_block_map.c
 * Purpose: Implementation of logical to physical block translation and of range
 *          release.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_block_map.h"

/*
 * Number of logical blocks reachable through one pointer at each depth.
 *
 * Indexed by indirection level: a level zero pointer reaches one block, a level one
 * pointer reaches a whole indirect block worth of them, and so on. Tabulating the
 * powers removes the only multiplication chain from the release walk and makes the
 * arithmetic of the walk readable as a single expression.
 */
static const uint64_t
    mimfs_logical_blocks_reached_per_pointer[MIMFS_MAXIMUM_INDIRECTION_LEVEL + 1u] = {
        1u, MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK,
        MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK
            *MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK,
        MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK *
            MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK *
            MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK
    };

int mimfs_block_map_decompose_logical_index(
    uint64_t logical_block_index, struct mimfs_block_map_position *position_out)
{
    /* The direct slots come first and cost no arithmetic beyond one comparison. */
    if (MIMFS_LIKELY(logical_block_index < MIMFS_DIRECT_BLOCK_POINTER_COUNT))
    {
        (*position_out).root_pointer_slot = (uint32_t)logical_block_index;
        (*position_out).indirection_level = 0u;

        return MIMFS_STATUS_SUCCESS;
    }

    uint64_t remaining_index = logical_block_index - MIMFS_DIRECT_BLOCK_POINTER_COUNT;

    /*
     * Walk the three indirect trees in order, subtracting the reach of each until
     * the index falls inside one. Then peel the per-level indices off the remainder
     * from the deepest level upwards, which is the same digit extraction a base
     * conversion performs, with the pointer count as the base.
     */
    for (uint32_t indirection_level = 1u;
         indirection_level <= MIMFS_MAXIMUM_INDIRECTION_LEVEL; indirection_level++)
    {
        const uint64_t reach_of_this_tree =
            *(mimfs_logical_blocks_reached_per_pointer + indirection_level);

        if (remaining_index < reach_of_this_tree)
        {
            (*position_out).root_pointer_slot =
                MIMFS_SINGLE_INDIRECT_POINTER_SLOT + indirection_level - 1u;
            (*position_out).indirection_level = indirection_level;

            for (uint32_t depth = indirection_level; depth > 0u; depth--)
            {
                *((*position_out).index_at_level + (depth - 1u)) =
                    (uint32_t)(remaining_index %
                               MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK);
                remaining_index /= MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK;
            }

            return MIMFS_STATUS_SUCCESS;
        }

        remaining_index -= reach_of_this_tree;
    }

    return MIMFS_STATUS_FILE_TOO_LARGE;
}

/*
 * Pick the block that a new allocation should try to land next to.
 *
 * The preference order is deliberate. The neighbouring entry of the same table is
 * best, because it makes a sequentially grown file contiguous. The file's first
 * block is next best, because it keeps the file inside one group. The group the
 * inode itself lives in is the last resort, which at least keeps a file's data in
 * the same arena as its metadata.
 */
static uint64_t mimfs_choose_locality_hint(struct mimfs_volume *volume,
                                           const struct mimfs_inode *inode,
                                           const uint32_t *table_base,
                                           uint32_t index_in_table)
{
    if (index_in_table > 0u)
    {
        const uint32_t neighbouring_block_number =
            *(table_base + (index_in_table - 1u));

        if (neighbouring_block_number != MIMFS_UNALLOCATED_BLOCK_NUMBER)
        {
            return neighbouring_block_number;
        }
    }

    const uint32_t first_block_of_file =
        *((*inode).block_map_root.block_pointer_slot + 0);

    if (first_block_of_file != MIMFS_UNALLOCATED_BLOCK_NUMBER)
    {
        return first_block_of_file;
    }

    const uint32_t home_group_index = (*inode).home_block_group_index;

    if (home_group_index < (*volume).block_group_count)
    {
        return (*mimfs_block_group_descriptor_at(volume, home_group_index))
            .first_block_number;
    }

    return MIMFS_UNALLOCATED_BLOCK_NUMBER;
}

int mimfs_block_map_resolve(
    struct mimfs_volume *volume, struct mimfs_inode *inode,
    uint64_t logical_block_index, enum mimfs_block_map_lookup_mode lookup_mode,
    enum mimfs_block_initialization_policy data_block_initialization_policy,
    enum mimfs_block_reserve_policy reserve_policy, uint64_t *physical_block_number_out)
{
    struct mimfs_block_map_position position;

    const int decomposition_status =
        mimfs_block_map_decompose_logical_index(logical_block_index, &position);

    if (MIMFS_UNLIKELY(decomposition_status != MIMFS_STATUS_SUCCESS))
    {
        return -decomposition_status;
    }

    uint32_t *table_base = (*inode).block_map_root.block_pointer_slot;
    uint32_t index_in_table = position.root_pointer_slot;

    /*
     * Descend one indirection level per iteration. The loop body is identical at
     * every depth, which is the payoff of decomposing the index up front: a single
     * piece of code covers the direct case, where it runs zero times, and the triple
     * indirect case, where it runs three times.
     */
    for (uint32_t level_index = 0u; level_index < position.indirection_level;
         level_index++)
    {
        uint32_t *slot_pointer = table_base + index_in_table;
        uint32_t indirect_block_number = *slot_pointer;

        if (indirect_block_number == MIMFS_UNALLOCATED_BLOCK_NUMBER)
        {
            if (lookup_mode == MIMFS_BLOCK_MAP_LOOKUP_ONLY)
            {
                /* A missing table means every block below it is a hole. */
                *physical_block_number_out = MIMFS_UNALLOCATED_BLOCK_NUMBER;
                return MIMFS_STATUS_SUCCESS;
            }

            const uint64_t locality_hint =
                mimfs_choose_locality_hint(volume, inode, table_base, index_in_table);

            /*
             * An indirect block is always cleared. Every word of it is read as a
             * block pointer, so a single stale non zero word would attach a block
             * owned by another file to this one.
             */
            const uint64_t allocated_block_number = mimfs_block_allocator_allocate(
                volume, locality_hint, MIMFS_BLOCK_INITIALIZE_TO_ZERO, reserve_policy);

            if (MIMFS_UNLIKELY(allocated_block_number ==
                               MIMFS_UNALLOCATED_BLOCK_NUMBER))
            {
                return -MIMFS_STATUS_NO_SPACE_LEFT;
            }

            *slot_pointer = (uint32_t)allocated_block_number;
            (*inode).allocated_block_count++;
            indirect_block_number = (uint32_t)allocated_block_number;
        }

        table_base = mimfs_block_as_pointer_array(volume, indirect_block_number);
        index_in_table = *(position.index_at_level + level_index);

        /*
         * The next iteration, or the data block lookup below, reads from this table.
         * Issuing the prefetch now overlaps the fetch of the line with the bounds
         * arithmetic that follows.
         */
        MIMFS_PREFETCH_FOR_READ(table_base + index_in_table);
    }

    uint32_t *data_slot_pointer = table_base + index_in_table;
    uint32_t data_block_number = *data_slot_pointer;

    if (data_block_number == MIMFS_UNALLOCATED_BLOCK_NUMBER)
    {
        if (lookup_mode == MIMFS_BLOCK_MAP_LOOKUP_ONLY)
        {
            *physical_block_number_out = MIMFS_UNALLOCATED_BLOCK_NUMBER;
            return MIMFS_STATUS_SUCCESS;
        }

        const uint64_t locality_hint =
            mimfs_choose_locality_hint(volume, inode, table_base, index_in_table);

        const uint64_t allocated_block_number = mimfs_block_allocator_allocate(
            volume, locality_hint, data_block_initialization_policy, reserve_policy);

        if (MIMFS_UNLIKELY(allocated_block_number == MIMFS_UNALLOCATED_BLOCK_NUMBER))
        {
            return -MIMFS_STATUS_NO_SPACE_LEFT;
        }

        *data_slot_pointer = (uint32_t)allocated_block_number;
        (*inode).allocated_block_count++;
        data_block_number = (uint32_t)allocated_block_number;
    }

    *physical_block_number_out = data_block_number;

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Release
 * ========================================================================= */

/*
 * Release a whole subtree unconditionally, including the table blocks that form
 * it.
 *
 * The recursion is bounded by the depth of the map, which is three, so the stack
 * cost is a constant that needs no explicit limit.
 */
static uint64_t mimfs_release_entire_subtree(struct mimfs_volume *volume,
                                             uint32_t subtree_root_block_number,
                                             uint32_t indirection_level)
{
    if (subtree_root_block_number == MIMFS_UNALLOCATED_BLOCK_NUMBER)
    {
        return 0u;
    }

    uint64_t released_block_count = 0u;

    if (indirection_level > 0u)
    {
        uint32_t *table =
            mimfs_block_as_pointer_array(volume, subtree_root_block_number);

        for (uint64_t entry_index = 0u;
             entry_index < MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK; entry_index++)
        {
            uint32_t *entry_pointer = table + entry_index;

            released_block_count += mimfs_release_entire_subtree(
                volume, *entry_pointer, indirection_level - 1u);
            *entry_pointer = MIMFS_UNALLOCATED_BLOCK_NUMBER;
        }
    }

    mimfs_block_allocator_release(volume, subtree_root_block_number);

    return released_block_count + 1u;
}

/*
 * Release the part of a subtree that lies at or after a threshold logical index.
 *
 * The three cases the function distinguishes are what keeps a partial truncation
 * cheap. A subtree entirely at or after the threshold is released wholesale with no
 * per-block decisions. A subtree entirely before it is skipped without being
 * entered at all. Only the single subtree that straddles the threshold is descended
 * into, and that happens at most once per level.
 */
static uint64_t mimfs_release_subtree_from(struct mimfs_volume *volume,
                                           uint32_t *slot_pointer,
                                           uint32_t indirection_level,
                                           uint64_t subtree_base_logical_index,
                                           uint64_t first_logical_index_to_release)
{
    const uint32_t subtree_root_block_number = *slot_pointer;

    if (subtree_root_block_number == MIMFS_UNALLOCATED_BLOCK_NUMBER)
    {
        return 0u;
    }

    if (subtree_base_logical_index >= first_logical_index_to_release)
    {
        const uint64_t released_block_count = mimfs_release_entire_subtree(
            volume, subtree_root_block_number, indirection_level);

        *slot_pointer = MIMFS_UNALLOCATED_BLOCK_NUMBER;

        return released_block_count;
    }

    /* A data block before the threshold survives untouched. */
    if (indirection_level == 0u)
    {
        return 0u;
    }

    uint32_t *table = mimfs_block_as_pointer_array(volume, subtree_root_block_number);

    const uint64_t logical_blocks_per_child =
        *(mimfs_logical_blocks_reached_per_pointer + (indirection_level - 1u));

    uint64_t released_block_count = 0u;
    bool table_became_empty = true;

    for (uint64_t entry_index = 0u;
         entry_index < MIMFS_BLOCK_POINTERS_PER_INDIRECT_BLOCK; entry_index++)
    {
        uint32_t *entry_pointer = table + entry_index;

        const uint64_t child_base_logical_index =
            subtree_base_logical_index + entry_index * logical_blocks_per_child;

        /* Skip the children that lie wholly before the threshold. */
        if (child_base_logical_index + logical_blocks_per_child <=
            first_logical_index_to_release)
        {
            if (*entry_pointer != MIMFS_UNALLOCATED_BLOCK_NUMBER)
            {
                table_became_empty = false;
            }

            continue;
        }

        released_block_count += mimfs_release_subtree_from(
            volume, entry_pointer, indirection_level - 1u, child_base_logical_index,
            first_logical_index_to_release);

        if (*entry_pointer != MIMFS_UNALLOCATED_BLOCK_NUMBER)
        {
            table_became_empty = false;
        }
    }

    /*
     * A table with no entries left holds nothing worth a block, so it goes too. That
     * is what keeps a file that is grown and truncated repeatedly from accumulating
     * empty indirect blocks it will never use again.
     */
    if (table_became_empty)
    {
        mimfs_block_allocator_release(volume, subtree_root_block_number);
        *slot_pointer = MIMFS_UNALLOCATED_BLOCK_NUMBER;
        released_block_count++;
    }

    return released_block_count;
}

uint64_t mimfs_block_map_release_from(struct mimfs_volume *volume,
                                      struct mimfs_inode *inode,
                                      uint64_t first_logical_block_index_to_release)
{
    uint32_t *root_pointer_array = (*inode).block_map_root.block_pointer_slot;

    uint64_t released_block_count = 0u;

    for (uint32_t direct_slot = 0u; direct_slot < MIMFS_DIRECT_BLOCK_POINTER_COUNT;
         direct_slot++)
    {
        released_block_count += mimfs_release_subtree_from(
            volume, root_pointer_array + direct_slot, 0u, direct_slot,
            first_logical_block_index_to_release);
    }

    uint64_t tree_base_logical_index = MIMFS_DIRECT_BLOCK_POINTER_COUNT;

    for (uint32_t indirection_level = 1u;
         indirection_level <= MIMFS_MAXIMUM_INDIRECTION_LEVEL; indirection_level++)
    {
        const uint32_t root_slot =
            MIMFS_SINGLE_INDIRECT_POINTER_SLOT + indirection_level - 1u;

        released_block_count += mimfs_release_subtree_from(
            volume, root_pointer_array + root_slot, indirection_level,
            tree_base_logical_index, first_logical_block_index_to_release);

        tree_base_logical_index +=
            *(mimfs_logical_blocks_reached_per_pointer + indirection_level);
    }

    return released_block_count;
}

uint64_t mimfs_block_map_release_all_blocks(struct mimfs_volume *volume,
                                            struct mimfs_inode *inode)
{
    return mimfs_block_map_release_from(volume, inode, 0u);
}
