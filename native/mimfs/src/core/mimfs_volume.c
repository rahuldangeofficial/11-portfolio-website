/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_volume.c
 * Purpose: Implementation of the volume lifecycle and of the layout engine.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_volume.h"

#include <string.h>

#include "mimfs_directory.h"
#include "mimfs_file_table.h"
#include "support/mimfs_bitmap.h"

/* =========================================================================
 * Power of two arithmetic
 * =========================================================================
 * The group sizes must be powers of two, because the mapping from an object
 * number to its group is on the path of every allocation and has to be a shift.
 * These three helpers are the only place that constraint is enforced.
 */

static uint32_t mimfs_largest_power_of_two_not_greater_than(uint32_t value)
{
    if (value == 0u)
    {
        return 0u;
    }

    return 1u << (31u - (uint32_t)__builtin_clz(value));
}

static uint32_t mimfs_smallest_power_of_two_not_less_than(uint32_t value)
{
    if (value <= 1u)
    {
        return 1u;
    }

    return 1u << (32u - (uint32_t)__builtin_clz(value - 1u));
}

static uint32_t mimfs_logarithm_base_two_of_power_of_two(uint32_t power_of_two)
{
    return (uint32_t)__builtin_ctz(power_of_two);
}

static uint32_t mimfs_clamp_unsigned(uint32_t value, uint32_t lower_bound,
                                     uint32_t upper_bound)
{
    if (value < lower_bound)
    {
        return lower_bound;
    }

    if (value > upper_bound)
    {
        return upper_bound;
    }

    return value;
}

/* =========================================================================
 * Layout engine
 * ========================================================================= */

/*
 * Place every region for a candidate geometry and report the offset at which the
 * data area would begin. Nothing is written; this is pure arithmetic over the
 * candidate, which is what allows the search below to try a hundred candidates
 * and commit only the winner.
 *
 * The regions are emitted in the order given by the map at the top of
 * mimfs_layout.h, each rounded up to a cache line so that no region ever starts
 * part way into a line shared with the region before it. The data area is
 * rounded up to a whole block so that block number zero starts on a block
 * boundary and every block copy stays page aligned.
 */
static void mimfs_place_regions(uint32_t maximum_open_file_count,
                                uint32_t blocks_per_group, uint32_t inodes_per_group,
                                uint32_t group_count,
                                struct mimfs_resolved_geometry *geometry_out)
{
    const uint64_t total_inode_count = (uint64_t)group_count * inodes_per_group;
    const uint64_t total_bitmap_block_count = (uint64_t)group_count * blocks_per_group;

    uint64_t next_offset = mimfs_round_up_to_power_of_two_multiple(
        sizeof(struct mimfs_volume), MIMFS_CACHE_LINE_SIZE_BYTES);

    (*geometry_out).open_file_description_table_offset = next_offset;
    next_offset +=
        (uint64_t)maximum_open_file_count * sizeof(struct mimfs_open_file_description);

    next_offset = mimfs_round_up_to_power_of_two_multiple(next_offset,
                                                          MIMFS_CACHE_LINE_SIZE_BYTES);
    (*geometry_out).descriptor_table_offset = next_offset;
    next_offset +=
        (uint64_t)maximum_open_file_count * sizeof(struct mimfs_descriptor_table_entry);

    next_offset = mimfs_round_up_to_power_of_two_multiple(next_offset,
                                                          MIMFS_CACHE_LINE_SIZE_BYTES);
    /* Two bitmaps share this region: one for descriptor slots, one for open file
     * description slots. See the accessors in mimfs_layout.h. */
    (*geometry_out).descriptor_allocation_bitmap_offset = next_offset;
    next_offset +=
        mimfs_bitmap_byte_count_for_bits(2u * (uint64_t)maximum_open_file_count);

    next_offset = mimfs_round_up_to_power_of_two_multiple(next_offset,
                                                          MIMFS_CACHE_LINE_SIZE_BYTES);
    (*geometry_out).block_group_descriptor_table_offset = next_offset;
    next_offset += (uint64_t)group_count * sizeof(struct mimfs_block_group_descriptor);

    next_offset = mimfs_round_up_to_power_of_two_multiple(next_offset,
                                                          MIMFS_CACHE_LINE_SIZE_BYTES);
    (*geometry_out).inode_allocation_bitmap_offset = next_offset;
    next_offset += mimfs_bitmap_byte_count_for_bits(total_inode_count);

    next_offset = mimfs_round_up_to_power_of_two_multiple(next_offset,
                                                          MIMFS_CACHE_LINE_SIZE_BYTES);
    (*geometry_out).block_allocation_bitmap_offset = next_offset;
    next_offset += mimfs_bitmap_byte_count_for_bits(total_bitmap_block_count);

    next_offset = mimfs_round_up_to_power_of_two_multiple(next_offset,
                                                          MIMFS_CACHE_LINE_SIZE_BYTES);
    (*geometry_out).inode_table_offset = next_offset;
    next_offset += total_inode_count * sizeof(struct mimfs_inode);

    (*geometry_out).data_block_region_offset =
        mimfs_round_up_to_power_of_two_multiple(next_offset, MIMFS_BLOCK_SIZE_BYTES);
}

/*
 * Choose the span of a block group, in blocks.
 *
 * The span is a compromise between two costs. Large groups keep the fixed cost
 * of a group, its descriptor and the rounding of its two bitmaps, negligible,
 * and they let one file receive a long contiguous run. Small groups give more
 * independent allocation arenas, which is what lets many threads allocate at the
 * same time without touching the same cache line.
 *
 * The target span is therefore a fixed number of bytes rather than a fixed
 * fraction of the volume, so that a large volume gets more arenas rather than
 * bigger ones. On a volume too small for even one target-sized group, the span
 * shrinks so that the volume still ends up with a handful of groups.
 */
static uint32_t mimfs_choose_blocks_per_group(uint64_t usable_size_in_bytes,
                                              uint32_t requested_blocks_per_group)
{
    if (requested_blocks_per_group != 0u)
    {
        const uint32_t clamped_request = mimfs_clamp_unsigned(
            requested_blocks_per_group, MIMFS_MINIMUM_BLOCKS_PER_BLOCK_GROUP,
            MIMFS_MAXIMUM_BLOCKS_PER_BLOCK_GROUP);

        return mimfs_largest_power_of_two_not_greater_than(clamped_request);
    }

    const uint64_t target_span_in_blocks =
        MIMFS_TARGET_BLOCK_GROUP_SPAN_BYTES / MIMFS_BLOCK_SIZE_BYTES;

    /*
     * Aim for at least four groups on a small volume: dividing by four rather
     * than by one is what keeps a modest volume from collapsing to a single
     * allocation arena and serialising every allocating thread on it.
     */
    const uint64_t span_that_fits_four_groups =
        usable_size_in_bytes / (4u * MIMFS_BLOCK_SIZE_BYTES);

    const uint64_t chosen_span =
        mimfs_minimum_of(target_span_in_blocks, span_that_fits_four_groups);

    const uint32_t clamped_span = mimfs_clamp_unsigned(
        (uint32_t)mimfs_minimum_of(chosen_span, UINT32_MAX),
        MIMFS_MINIMUM_BLOCKS_PER_BLOCK_GROUP, MIMFS_MAXIMUM_BLOCKS_PER_BLOCK_GROUP);

    return mimfs_largest_power_of_two_not_greater_than(clamped_span);
}

/*
 * Choose how many inodes a group provisions, from the requested ratio of data
 * bytes per inode. Rounded up to a power of two, and never below the bitmap word
 * width so that a group bitmap is always a whole number of words.
 */
static uint32_t mimfs_choose_inodes_per_group(uint32_t blocks_per_group,
                                              uint64_t bytes_per_inode_ratio)
{
    const uint64_t data_bytes_in_group =
        (uint64_t)blocks_per_group * MIMFS_BLOCK_SIZE_BYTES;

    uint64_t inode_count = data_bytes_in_group / bytes_per_inode_ratio;

    if (inode_count < MIMFS_MINIMUM_INODES_PER_BLOCK_GROUP)
    {
        inode_count = MIMFS_MINIMUM_INODES_PER_BLOCK_GROUP;
    }

    /*
     * An inode occupies a quarter of the space of a block, so provisioning more
     * than four inodes per block of data would spend more memory on the inode
     * table than could ever be described by it.
     */
    const uint64_t sane_upper_bound = (uint64_t)blocks_per_group * 4u;

    if (inode_count > sane_upper_bound)
    {
        inode_count = sane_upper_bound;
    }

    return mimfs_smallest_power_of_two_not_less_than((uint32_t)inode_count);
}

int mimfs_volume_resolve_geometry(uint64_t aligned_region_size_in_bytes,
                                  uint64_t alignment_padding_bytes,
                                  const struct mimfs_format_parameters *parameters,
                                  struct mimfs_resolved_geometry *geometry_out)
{
    if (MIMFS_UNLIKELY(geometry_out == NULL))
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    struct mimfs_format_parameters effective_parameters;
    memset(&effective_parameters, 0, sizeof(effective_parameters));

    if (parameters != NULL)
    {
        effective_parameters = *parameters;
    }

    /* Apply the documented default for every field left at zero. */
    if (effective_parameters.bytes_per_inode_ratio == 0u)
    {
        effective_parameters.bytes_per_inode_ratio =
            MIMFS_DEFAULT_BYTES_PER_INODE_RATIO;
    }

    if (effective_parameters.maximum_open_file_count == 0u)
    {
        effective_parameters.maximum_open_file_count =
            MIMFS_DEFAULT_MAXIMUM_OPEN_FILE_COUNT;
    }

    if (effective_parameters.reserved_block_percentage >
        MIMFS_MAXIMUM_RESERVED_BLOCK_PERCENTAGE)
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    /*
     * A ratio below one block per inode would provision inodes that no data
     * could ever be attached to, and an unbounded ratio would overflow the inode
     * count arithmetic. Both ends are clamped rather than rejected so that a
     * caller can pass a rough number and still get a working volume.
     */
    if (effective_parameters.bytes_per_inode_ratio < MIMFS_BLOCK_SIZE_BYTES)
    {
        effective_parameters.bytes_per_inode_ratio = MIMFS_BLOCK_SIZE_BYTES;
    }

    const uint32_t maximum_open_file_count =
        (uint32_t)mimfs_round_up_to_power_of_two_multiple(
            mimfs_clamp_unsigned(effective_parameters.maximum_open_file_count,
                                 MIMFS_MINIMUM_MAXIMUM_OPEN_FILE_COUNT,
                                 MIMFS_MAXIMUM_MAXIMUM_OPEN_FILE_COUNT),
            MIMFS_BITMAP_WORD_BIT_COUNT);

    const uint32_t blocks_per_group = mimfs_choose_blocks_per_group(
        aligned_region_size_in_bytes, effective_parameters.blocks_per_block_group);

    const uint32_t inodes_per_group = mimfs_choose_inodes_per_group(
        blocks_per_group, effective_parameters.bytes_per_inode_ratio);

    /*
     * Search for the group count that yields the most data blocks.
     *
     * Adding a group costs a descriptor, two bitmap slices and a slice of inode
     * table, all of which come out of the data area; it pays for itself only as
     * long as the extra group span it unlocks is actually backed by memory. The
     * search therefore walks the candidates in order and keeps the best, rather
     * than solving for the group count analytically, because the rounding of
     * every region boundary makes the closed form both fragile and unreadable
     * for a gain of a few microseconds once per volume.
     */
    struct mimfs_resolved_geometry best_geometry;
    memset(&best_geometry, 0, sizeof(best_geometry));

    uint64_t best_total_block_count = 0u;
    uint32_t best_group_count = 0u;

    for (uint32_t candidate_group_count = 1u;
         candidate_group_count <= MIMFS_MAXIMUM_BLOCK_GROUP_COUNT;
         candidate_group_count++)
    {
        struct mimfs_resolved_geometry candidate_geometry;
        memset(&candidate_geometry, 0, sizeof(candidate_geometry));

        mimfs_place_regions(maximum_open_file_count, blocks_per_group, inodes_per_group,
                            candidate_group_count, &candidate_geometry);

        if (candidate_geometry.data_block_region_offset >= aligned_region_size_in_bytes)
        {
            /* The metadata of this many groups already fills the region. */
            break;
        }

        const uint64_t data_bytes_available =
            aligned_region_size_in_bytes - candidate_geometry.data_block_region_offset;

        const uint64_t group_span_capacity =
            (uint64_t)candidate_group_count * blocks_per_group;

        const uint64_t candidate_total_block_count = mimfs_minimum_of(
            group_span_capacity, data_bytes_available / MIMFS_BLOCK_SIZE_BYTES);

        /*
         * Every group must own at least one block. A candidate whose final group
         * would be empty describes a volume with a group that can never be
         * allocated from, so it is rejected outright.
         */
        const uint64_t blocks_before_final_group =
            (uint64_t)(candidate_group_count - 1u) * blocks_per_group;

        if (candidate_total_block_count <= blocks_before_final_group)
        {
            break;
        }

        if (candidate_total_block_count >= best_total_block_count)
        {
            best_total_block_count = candidate_total_block_count;
            best_group_count = candidate_group_count;
            best_geometry = candidate_geometry;
            best_geometry.block_count_in_final_group =
                (uint32_t)(candidate_total_block_count - blocks_before_final_group);
        }
    }

    /*
     * Two data blocks is the true floor: one for the root directory and one
     * spare, because block number zero is never handed out.
     */
    if (best_group_count == 0u || best_total_block_count < 2u)
    {
        return MIMFS_STATUS_NO_SPACE_LEFT;
    }

    *geometry_out = best_geometry;

    (*geometry_out).usable_size_in_bytes = aligned_region_size_in_bytes;
    (*geometry_out).alignment_padding_bytes = alignment_padding_bytes;
    (*geometry_out).maximum_open_file_count = maximum_open_file_count;
    (*geometry_out).block_group_count = best_group_count;
    (*geometry_out).blocks_per_block_group = blocks_per_group;
    (*geometry_out).blocks_per_block_group_log2 =
        mimfs_logarithm_base_two_of_power_of_two(blocks_per_group);
    (*geometry_out).inodes_per_block_group = inodes_per_group;
    (*geometry_out).inodes_per_block_group_log2 =
        mimfs_logarithm_base_two_of_power_of_two(inodes_per_group);
    (*geometry_out).total_block_count = best_total_block_count;
    (*geometry_out).total_inode_count = (uint64_t)best_group_count * inodes_per_group;
    (*geometry_out).reserved_block_count =
        (best_total_block_count * effective_parameters.reserved_block_percentage) /
        100u;

    /*
     * The inode table must have room for the two reserved numbers plus at least
     * one usable inode, otherwise a freshly formatted volume could not even hold
     * a single file.
     */
    if ((*geometry_out).total_inode_count < MIMFS_FIRST_ALLOCATABLE_INODE_NUMBER)
    {
        return MIMFS_STATUS_NO_SPACE_LEFT;
    }

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Writing the geometry into the volume header
 * ========================================================================= */

static void mimfs_publish_geometry(struct mimfs_volume *volume,
                                   const struct mimfs_resolved_geometry *geometry)
{
    (*volume).magic_number = MIMFS_VOLUME_MAGIC_NUMBER;
    (*volume).format_revision = MIMFS_FORMAT_REVISION;
    (*volume).volume_size_in_bytes = (*geometry).usable_size_in_bytes;
    (*volume).alignment_padding_bytes = (*geometry).alignment_padding_bytes;
    (*volume).block_size_in_bytes = MIMFS_BLOCK_SIZE_BYTES;
    (*volume).inode_size_in_bytes = MIMFS_INODE_SIZE_BYTES;
    (*volume).total_block_count = (*geometry).total_block_count;
    (*volume).total_inode_count = (*geometry).total_inode_count;
    (*volume).block_group_count = (*geometry).block_group_count;
    (*volume).blocks_per_block_group = (*geometry).blocks_per_block_group;
    (*volume).inodes_per_block_group = (*geometry).inodes_per_block_group;
    (*volume).maximum_open_file_count = (*geometry).maximum_open_file_count;
    (*volume).blocks_per_block_group_log2 = (*geometry).blocks_per_block_group_log2;
    (*volume).inodes_per_block_group_log2 = (*geometry).inodes_per_block_group_log2;
    (*volume).block_count_in_final_group = (*geometry).block_count_in_final_group;
    (*volume).reserved_geometry_field = 0u;
    (*volume).reserved_block_count = (*geometry).reserved_block_count;

    (*volume).open_file_description_table_offset =
        (*geometry).open_file_description_table_offset;
    (*volume).descriptor_table_offset = (*geometry).descriptor_table_offset;
    (*volume).descriptor_allocation_bitmap_offset =
        (*geometry).descriptor_allocation_bitmap_offset;
    (*volume).block_group_descriptor_table_offset =
        (*geometry).block_group_descriptor_table_offset;
    (*volume).inode_allocation_bitmap_offset =
        (*geometry).inode_allocation_bitmap_offset;
    (*volume).block_allocation_bitmap_offset =
        (*geometry).block_allocation_bitmap_offset;
    (*volume).inode_table_offset = (*geometry).inode_table_offset;
    (*volume).data_block_region_offset = (*geometry).data_block_region_offset;
}

/*
 * Fill in the group descriptors and the two allocation bitmaps of a fresh
 * volume.
 *
 * The final group is the only one that may be short. The blocks its span covers
 * but the volume does not actually contain are marked allocated in the bitmap
 * rather than tracked as a separate limit, which keeps one invariant true
 * everywhere else in the filesystem: a clear bit in the block bitmap is always a
 * block that exists.
 */
static void mimfs_initialize_allocation_state(struct mimfs_volume *volume)
{
    const uint32_t group_count = (*volume).block_group_count;
    const uint32_t blocks_per_group = (*volume).blocks_per_block_group;
    const uint32_t inodes_per_group = (*volume).inodes_per_block_group;

    _Atomic uint64_t *block_bitmap = mimfs_block_allocation_bitmap(volume);
    _Atomic uint64_t *inode_bitmap = mimfs_inode_allocation_bitmap(volume);

    mimfs_bitmap_clear_all_bits(block_bitmap, (uint64_t)group_count * blocks_per_group);
    mimfs_bitmap_clear_all_bits(inode_bitmap, (*volume).total_inode_count);

    uint64_t total_free_block_count = 0u;

    for (uint32_t group_index = 0u; group_index < group_count; group_index++)
    {
        struct mimfs_block_group_descriptor *group =
            mimfs_block_group_descriptor_at(volume, group_index);

        const uint64_t first_block_number = (uint64_t)group_index * blocks_per_group;
        const bool group_is_final = (group_index + 1u) == group_count;

        const uint32_t block_count_in_group =
            group_is_final ? (*volume).block_count_in_final_group : blocks_per_group;

        (*group).block_count_in_group = block_count_in_group;
        (*group).inode_count_in_group = inodes_per_group;
        (*group).first_block_number = first_block_number;
        (*group).first_inode_number = (uint64_t)group_index * inodes_per_group + 1u;

        /* Fence off the span this group covers but the volume does not have. */
        for (uint32_t block_offset = block_count_in_group;
             block_offset < blocks_per_group; block_offset++)
        {
            (void)mimfs_bitmap_claim_specific_bit(block_bitmap,
                                                  first_block_number + block_offset);
        }

        uint64_t free_block_count_in_group = block_count_in_group;

        /*
         * Block number zero is the "no block" sentinel of the block map, so it
         * must never be handed out. Spending one block to make a null pointer
         * expressible removes a parallel "is this pointer valid" flag from every
         * block map entry in the volume.
         */
        if (group_index == 0u)
        {
            (void)mimfs_bitmap_claim_specific_bit(block_bitmap,
                                                  MIMFS_UNALLOCATED_BLOCK_NUMBER);
            free_block_count_in_group -= 1u;
        }

        atomic_store_explicit(&(*group).free_block_count, free_block_count_in_group,
                              memory_order_relaxed);
        atomic_store_explicit(&(*group).next_block_search_hint, first_block_number,
                              memory_order_relaxed);

        uint64_t free_inode_count_in_group = inodes_per_group;

        /*
         * Inode one is reserved by convention for bad block accounting and inode
         * two is the root directory. Both live in group zero and are claimed here
         * so that no allocation can ever produce either number.
         */
        if (group_index == 0u)
        {
            (void)mimfs_bitmap_claim_specific_bit(inode_bitmap,
                                                  MIMFS_BAD_BLOCK_INODE_NUMBER - 1u);
            (void)mimfs_bitmap_claim_specific_bit(inode_bitmap,
                                                  MIMFS_ROOT_INODE_NUMBER - 1u);
            free_inode_count_in_group -= 2u;
        }

        atomic_store_explicit(&(*group).free_inode_count, free_inode_count_in_group,
                              memory_order_relaxed);
        atomic_store_explicit(&(*group).next_inode_search_hint,
                              (uint64_t)group_index * inodes_per_group,
                              memory_order_relaxed);

        total_free_block_count += free_block_count_in_group;

        memset((*group).reserved_padding, 0, sizeof((*group).reserved_padding));
    }

    atomic_store_explicit(&(*volume).free_block_count, total_free_block_count,
                          memory_order_relaxed);
    atomic_store_explicit(&(*volume).free_inode_count, (*volume).total_inode_count - 2u,
                          memory_order_relaxed);
    atomic_store_explicit(&(*volume).allocation_rotor, 0u, memory_order_relaxed);
}

/*
 * Rebuild every piece of state that must not survive a mount.
 *
 * This is what makes a volume left behind by a process that was killed mid
 * operation safe to mount: no lock can still be held, because every lock word is
 * rewritten; no inode can be pinned by a descriptor that no longer exists,
 * because every reference count is rewritten; and no descriptor can be inherited,
 * because the descriptor table is rewritten.
 */
static void mimfs_initialize_volatile_state(struct mimfs_volume *volume)
{
    mimfs_reader_writer_lock_initialize(&(*volume).rename_serialization_lock);
    atomic_store_explicit(&(*volume).open_descriptor_count, 0u, memory_order_relaxed);

    for (uint64_t inode_number = 1u; inode_number <= (*volume).total_inode_count;
         inode_number++)
    {
        struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

        mimfs_reader_writer_lock_initialize(&(*inode).content_lock);
        atomic_store_explicit(&(*inode).open_reference_count, 0u, memory_order_relaxed);
    }

    const uint32_t maximum_open_file_count = (*volume).maximum_open_file_count;

    /* Both halves of the shared region: descriptor slots and description slots. */
    mimfs_bitmap_clear_all_bits(mimfs_descriptor_allocation_bitmap(volume),
                                2u * (uint64_t)maximum_open_file_count);

    for (uint32_t description_index = 0u; description_index < maximum_open_file_count;
         description_index++)
    {
        struct mimfs_open_file_description *description =
            mimfs_open_file_description_at(volume, description_index);

        memset(description, 0, sizeof(*description));
        mimfs_spin_lock_initialize(&(*description).offset_lock);
    }

    for (uint32_t descriptor = 0u; descriptor < maximum_open_file_count; descriptor++)
    {
        struct mimfs_descriptor_table_entry *entry =
            mimfs_descriptor_table_entry_at(volume, descriptor);

        atomic_store_explicit(&(*entry).open_file_description_index, -1,
                              memory_order_relaxed);
        (*entry).descriptor_flags = 0u;
    }
}

/* =========================================================================
 * Format
 * ========================================================================= */

struct mimfs_volume *
mimfs_volume_format(void *memory_region_base, size_t memory_region_size_in_bytes,
                    const struct mimfs_format_parameters *parameters, int *status_out)
{
    if (status_out != NULL)
    {
        *status_out = MIMFS_STATUS_SUCCESS;
    }

    if (MIMFS_UNLIKELY(memory_region_base == NULL))
    {
        if (status_out != NULL)
        {
            *status_out = MIMFS_STATUS_INVALID_ARGUMENT;
        }

        return NULL;
    }

    /*
     * Align the base upwards. The volume header holds atomics that must be
     * naturally aligned, and the whole point of the cache line padding inside it
     * is lost if the structure itself starts part way into a line.
     */
    const uintptr_t requested_base = (uintptr_t)memory_region_base;
    const uintptr_t aligned_base =
        mimfs_align_address_upwards(requested_base, MIMFS_CACHE_LINE_SIZE_BYTES);
    const uint64_t alignment_padding_bytes = (uint64_t)(aligned_base - requested_base);

    if (MIMFS_UNLIKELY(memory_region_size_in_bytes <= alignment_padding_bytes))
    {
        if (status_out != NULL)
        {
            *status_out = MIMFS_STATUS_NO_SPACE_LEFT;
        }

        return NULL;
    }

    const uint64_t usable_size_in_bytes =
        (uint64_t)memory_region_size_in_bytes - alignment_padding_bytes;

    struct mimfs_resolved_geometry geometry;
    const int geometry_status = mimfs_volume_resolve_geometry(
        usable_size_in_bytes, alignment_padding_bytes, parameters, &geometry);

    if (geometry_status != MIMFS_STATUS_SUCCESS)
    {
        if (status_out != NULL)
        {
            *status_out = geometry_status;
        }

        return NULL;
    }

    struct mimfs_volume *volume = (struct mimfs_volume *)aligned_base;

    /*
     * Zero the header and the inode table. The header must be clean because a
     * stale magic number in a region that is being reformatted would let a
     * concurrent mount attach to a half written volume. The inode table must be
     * clean because an allocated inode is expected to have been zero before the
     * allocator handed it out.
     */
    memset(volume, 0, sizeof(*volume));
    mimfs_publish_geometry(volume, &geometry);

    memset(mimfs_volume_region_at(volume, geometry.inode_table_offset), 0,
           geometry.total_inode_count * sizeof(struct mimfs_inode));

    mimfs_initialize_allocation_state(volume);
    mimfs_initialize_volatile_state(volume);

    /* From here on the ordinary code paths may run against this volume. */
    atomic_store_explicit(&(*volume).mount_state, MIMFS_MOUNT_STATE_MOUNTED,
                          memory_order_release);

    const int root_status = mimfs_directory_create_root(volume);

    if (root_status != MIMFS_STATUS_SUCCESS)
    {
        atomic_store_explicit(&(*volume).mount_state, MIMFS_MOUNT_STATE_UNMOUNTED,
                              memory_order_release);
        (*volume).magic_number = 0u;

        if (status_out != NULL)
        {
            *status_out = root_status;
        }

        return NULL;
    }

    return volume;
}

/* =========================================================================
 * Mount
 * ========================================================================= */

/*
 * Verify that the region really holds a volume this build can operate on.
 *
 * Everything checked here is a geometry field that the rest of the filesystem
 * treats as an axiom. A volume that fails any of these checks is refused rather
 * than repaired, because every one of these fields is used to compute an address,
 * and a wrong address is not a recoverable error.
 */
static int mimfs_validate_mounted_geometry(struct mimfs_volume *volume,
                                           uint64_t usable_size_in_bytes)
{
    if ((*volume).magic_number != MIMFS_VOLUME_MAGIC_NUMBER)
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if ((*volume).format_revision != MIMFS_FORMAT_REVISION)
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if ((*volume).block_size_in_bytes != MIMFS_BLOCK_SIZE_BYTES ||
        (*volume).inode_size_in_bytes != MIMFS_INODE_SIZE_BYTES)
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    if ((*volume).block_group_count == 0u ||
        (*volume).block_group_count > MIMFS_MAXIMUM_BLOCK_GROUP_COUNT)
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    if (!mimfs_value_is_power_of_two((*volume).blocks_per_block_group) ||
        !mimfs_value_is_power_of_two((*volume).inodes_per_block_group))
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    if ((1u << (*volume).blocks_per_block_group_log2) !=
            (*volume).blocks_per_block_group ||
        (1u << (*volume).inodes_per_block_group_log2) !=
            (*volume).inodes_per_block_group)
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    if ((*volume).total_inode_count !=
        (uint64_t)(*volume).block_group_count * (*volume).inodes_per_block_group)
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    if ((*volume).total_block_count < 2u ||
        (*volume).total_block_count >
            (uint64_t)(*volume).block_group_count * (*volume).blocks_per_block_group)
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    /* Every region, and the data area in particular, must lie inside the memory
     * the caller actually handed over. */
    const uint64_t end_of_data_region =
        (*volume).data_block_region_offset +
        (*volume).total_block_count * MIMFS_BLOCK_SIZE_BYTES;

    if (end_of_data_region > usable_size_in_bytes)
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    return MIMFS_STATUS_SUCCESS;
}

struct mimfs_volume *mimfs_volume_mount(void *memory_region_base,
                                        size_t memory_region_size_in_bytes,
                                        int *status_out)
{
    if (status_out != NULL)
    {
        *status_out = MIMFS_STATUS_SUCCESS;
    }

    if (MIMFS_UNLIKELY(memory_region_base == NULL))
    {
        if (status_out != NULL)
        {
            *status_out = MIMFS_STATUS_INVALID_ARGUMENT;
        }

        return NULL;
    }

    const uintptr_t requested_base = (uintptr_t)memory_region_base;
    const uintptr_t aligned_base =
        mimfs_align_address_upwards(requested_base, MIMFS_CACHE_LINE_SIZE_BYTES);
    const uint64_t alignment_padding_bytes = (uint64_t)(aligned_base - requested_base);

    if (MIMFS_UNLIKELY(memory_region_size_in_bytes <=
                       alignment_padding_bytes + sizeof(struct mimfs_volume)))
    {
        if (status_out != NULL)
        {
            *status_out = MIMFS_STATUS_INVALID_ARGUMENT;
        }

        return NULL;
    }

    struct mimfs_volume *volume = (struct mimfs_volume *)aligned_base;

    const uint64_t usable_size_in_bytes =
        (uint64_t)memory_region_size_in_bytes - alignment_padding_bytes;

    const int validation_status =
        mimfs_validate_mounted_geometry(volume, usable_size_in_bytes);

    if (validation_status != MIMFS_STATUS_SUCCESS)
    {
        if (status_out != NULL)
        {
            *status_out = validation_status;
        }

        return NULL;
    }

    /* The persistent state is kept exactly as found; only the volatile state is
     * rebuilt. */
    mimfs_initialize_volatile_state(volume);

    atomic_store_explicit(&(*volume).mount_state, MIMFS_MOUNT_STATE_MOUNTED,
                          memory_order_release);

    /* The root directory is the one persistent object whose shape the rest of the
     * filesystem assumes without checking, so it is verified once, here. */
    struct mimfs_inode *root_inode = mimfs_inode_at(volume, MIMFS_ROOT_INODE_NUMBER);

    if (!MIMFS_MODE_IS_DIRECTORY((*root_inode).file_mode) ||
        (*root_inode).hard_link_count < 2u)
    {
        atomic_store_explicit(&(*volume).mount_state, MIMFS_MOUNT_STATE_UNMOUNTED,
                              memory_order_release);

        if (status_out != NULL)
        {
            *status_out = MIMFS_STATUS_CORRUPTED_VOLUME;
        }

        return NULL;
    }

    return volume;
}

/* =========================================================================
 * Unmount and accounting
 * ========================================================================= */

int mimfs_volume_unmount(struct mimfs_volume *volume)
{
    if (!mimfs_volume_is_usable(volume))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    /*
     * Closing the descriptors first is what gives an unlinked but still open
     * inode its chance to be destroyed, so that the volume left in memory has no
     * inode that is unreachable from the namespace yet still holds blocks.
     */
    mimfs_file_table_close_all_descriptors(volume);

    atomic_store_explicit(&(*volume).mount_state, MIMFS_MOUNT_STATE_UNMOUNTED,
                          memory_order_release);

    return MIMFS_STATUS_SUCCESS;
}

uint64_t mimfs_volume_available_block_count(struct mimfs_volume *volume)
{
    const uint64_t free_block_count =
        atomic_load_explicit(&(*volume).free_block_count, memory_order_relaxed);
    const uint64_t reserved_block_count = (*volume).reserved_block_count;

    return free_block_count > reserved_block_count
               ? free_block_count - reserved_block_count
               : 0u;
}

int mimfs_volume_query_statistics(struct mimfs_volume *volume,
                                  struct mimfs_volume_statistics *statistics_out)
{
    if (!mimfs_volume_is_usable(volume) || statistics_out == NULL)
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    memset(statistics_out, 0, sizeof(*statistics_out));

    (*statistics_out).block_size_in_bytes = (*volume).block_size_in_bytes;
    (*statistics_out).inode_size_in_bytes = (*volume).inode_size_in_bytes;
    (*statistics_out).total_block_count = (*volume).total_block_count;
    (*statistics_out).free_block_count =
        atomic_load_explicit(&(*volume).free_block_count, memory_order_relaxed);
    (*statistics_out).available_block_count =
        mimfs_volume_available_block_count(volume);
    (*statistics_out).total_inode_count = (*volume).total_inode_count;
    (*statistics_out).free_inode_count =
        atomic_load_explicit(&(*volume).free_inode_count, memory_order_relaxed);
    (*statistics_out).block_group_count = (*volume).block_group_count;
    (*statistics_out).blocks_per_block_group = (*volume).blocks_per_block_group;
    (*statistics_out).inodes_per_block_group = (*volume).inodes_per_block_group;
    (*statistics_out).maximum_open_file_count = (*volume).maximum_open_file_count;
    (*statistics_out).open_file_count = (uint32_t)atomic_load_explicit(
        &(*volume).open_descriptor_count, memory_order_relaxed);
    (*statistics_out).maximum_file_size_in_bytes = MIMFS_MAXIMUM_FILE_SIZE_BYTES;
    (*statistics_out).maximum_name_length = MIMFS_MAXIMUM_FILE_NAME_LENGTH;

    return MIMFS_STATUS_SUCCESS;
}
