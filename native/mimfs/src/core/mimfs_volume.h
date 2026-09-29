/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_volume.h
 * Purpose: Volume lifecycle: deciding the geometry, writing a fresh volume,
 *          attaching to an existing one, and reporting accounting.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * This module is the only one that decides where anything lives. Every other
 * module reads the region offsets it produced and never computes one.
 */

#ifndef MIMFS_VOLUME_H
#define MIMFS_VOLUME_H

#include "mimfs_layout.h"

/*
 * Fully resolved geometry of a volume. Produced by the layout engine, then
 * copied into the volume header. Keeping it as a separate value means the whole
 * geometry decision can be computed and validated before a single byte of the
 * caller's memory is modified, so a format that turns out to be impossible
 * leaves the region untouched.
 */
struct mimfs_resolved_geometry
{
    uint64_t usable_size_in_bytes;
    uint64_t alignment_padding_bytes;

    uint32_t maximum_open_file_count;
    uint32_t block_group_count;
    uint32_t blocks_per_block_group;
    uint32_t blocks_per_block_group_log2;
    uint32_t inodes_per_block_group;
    uint32_t inodes_per_block_group_log2;
    uint32_t block_count_in_final_group;

    uint64_t total_block_count;
    uint64_t total_inode_count;
    uint64_t reserved_block_count;

    uint64_t open_file_description_table_offset;
    uint64_t descriptor_table_offset;
    uint64_t descriptor_allocation_bitmap_offset;
    uint64_t block_group_descriptor_table_offset;
    uint64_t inode_allocation_bitmap_offset;
    uint64_t block_allocation_bitmap_offset;
    uint64_t inode_table_offset;
    uint64_t data_block_region_offset;
};

/*
 * Decide the geometry for a region of the given size.
 *
 * Returns MIMFS_STATUS_SUCCESS, or a positive status code when no viable
 * geometry exists, which happens when the region cannot hold the fixed tables
 * plus one group with at least one data block.
 */
int mimfs_volume_resolve_geometry(uint64_t aligned_region_size_in_bytes,
                                  uint64_t alignment_padding_bytes,
                                  const struct mimfs_format_parameters *parameters,
                                  struct mimfs_resolved_geometry *geometry_out);

/* Write a fresh volume over the region and mount it. */
struct mimfs_volume *
mimfs_volume_format(void *memory_region_base, size_t memory_region_size_in_bytes,
                    const struct mimfs_format_parameters *parameters, int *status_out);

/* Attach to a volume already present in the region. */
struct mimfs_volume *mimfs_volume_mount(void *memory_region_base,
                                        size_t memory_region_size_in_bytes,
                                        int *status_out);

/* Detach, closing everything still open. */
int mimfs_volume_unmount(struct mimfs_volume *volume);

/* Fill in whole volume accounting. */
int mimfs_volume_query_statistics(struct mimfs_volume *volume,
                                  struct mimfs_volume_statistics *statistics_out);

/*
 * Blocks that ordinary allocation may still consume, meaning free blocks less
 * the reserve. Reported to callers and used by the allocator to decide whether
 * a request has to be refused.
 */
uint64_t mimfs_volume_available_block_count(struct mimfs_volume *volume);

#endif /* MIMFS_VOLUME_H */
