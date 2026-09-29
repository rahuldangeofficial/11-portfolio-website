/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_file_io.c
 * Purpose: Implementation of byte level transfer and length control.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_file_io.h"

#include <string.h>

#include "mimfs_block_map.h"
#include "mimfs_inode.h"

/* Logical block holding a byte offset, and the position of that byte inside it.
 * Both are a shift and a mask; the block size is a power of two for exactly this
 * reason. */
#define MIMFS_LOGICAL_BLOCK_INDEX_OF_OFFSET(offset)                                    \
    ((uint64_t)(offset) >> MIMFS_BLOCK_SIZE_LOG2)
#define MIMFS_OFFSET_WITHIN_BLOCK(offset) ((uint64_t)(offset) & MIMFS_BLOCK_OFFSET_MASK)

long mimfs_file_read_at_offset(struct mimfs_volume *volume, struct mimfs_inode *inode,
                               uint64_t file_offset, void *destination_buffer,
                               size_t byte_count)
{
    if (MIMFS_UNLIKELY(destination_buffer == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (byte_count == 0u)
    {
        return 0;
    }

    const uint64_t file_size = (*inode).file_size_in_bytes;

    /* Nothing at or past the end of the file, which is end of file rather than an
     * error. */
    if (file_offset >= file_size)
    {
        return 0;
    }

    const uint64_t bytes_available_to_read = file_size - file_offset;
    uint64_t bytes_remaining =
        mimfs_minimum_of((uint64_t)byte_count, bytes_available_to_read);

    unsigned char *destination_cursor = (unsigned char *)destination_buffer;
    uint64_t current_offset = file_offset;
    uint64_t total_bytes_read = 0u;

    while (bytes_remaining > 0u)
    {
        const uint64_t logical_block_index =
            MIMFS_LOGICAL_BLOCK_INDEX_OF_OFFSET(current_offset);
        const uint64_t offset_within_block = MIMFS_OFFSET_WITHIN_BLOCK(current_offset);

        const uint64_t bytes_in_this_block = mimfs_minimum_of(
            bytes_remaining, (uint64_t)MIMFS_BLOCK_SIZE_BYTES - offset_within_block);

        uint64_t physical_block_number = MIMFS_UNALLOCATED_BLOCK_NUMBER;

        const int resolve_status = mimfs_block_map_resolve(
            volume, inode, logical_block_index, MIMFS_BLOCK_MAP_LOOKUP_ONLY,
            MIMFS_BLOCK_INITIALIZE_TO_ZERO, MIMFS_BLOCK_RESPECT_RESERVE,
            &physical_block_number);

        if (MIMFS_UNLIKELY(resolve_status != MIMFS_STATUS_SUCCESS))
        {
            /* Only reachable when the file's length claims a block beyond what the
             * map can address, which means the inode is damaged. */
            return total_bytes_read > 0 ? (long)total_bytes_read : resolve_status;
        }

        if (MIMFS_LIKELY(physical_block_number != MIMFS_UNALLOCATED_BLOCK_NUMBER))
        {
            const unsigned char *block_bytes =
                mimfs_block_address(volume, physical_block_number);

            memcpy(destination_cursor, block_bytes + offset_within_block,
                   (size_t)bytes_in_this_block);
        }
        else
        {
            /* A hole. It has no block, and it reads as zeroes. */
            memset(destination_cursor, 0, (size_t)bytes_in_this_block);
        }

        destination_cursor += bytes_in_this_block;
        current_offset += bytes_in_this_block;
        total_bytes_read += bytes_in_this_block;
        bytes_remaining -= bytes_in_this_block;
    }

    mimfs_inode_stamp_access_time(inode);

    return (long)total_bytes_read;
}

long mimfs_file_write_at_offset(struct mimfs_volume *volume, struct mimfs_inode *inode,
                                uint64_t file_offset, const void *source_buffer,
                                size_t byte_count)
{
    if (MIMFS_UNLIKELY(source_buffer == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (byte_count == 0u)
    {
        return 0;
    }

    /* A write whose first byte is already beyond the addressable range cannot be
     * shortened into something valid, so it is refused outright. */
    if (MIMFS_UNLIKELY(file_offset >= MIMFS_MAXIMUM_FILE_SIZE_BYTES))
    {
        return -MIMFS_STATUS_FILE_TOO_LARGE;
    }

    uint64_t bytes_remaining = mimfs_minimum_of(
        (uint64_t)byte_count, MIMFS_MAXIMUM_FILE_SIZE_BYTES - file_offset);

    const unsigned char *source_cursor = (const unsigned char *)source_buffer;
    uint64_t current_offset = file_offset;
    uint64_t total_bytes_written = 0u;

    while (bytes_remaining > 0u)
    {
        const uint64_t logical_block_index =
            MIMFS_LOGICAL_BLOCK_INDEX_OF_OFFSET(current_offset);
        const uint64_t offset_within_block = MIMFS_OFFSET_WITHIN_BLOCK(current_offset);

        const uint64_t bytes_in_this_block = mimfs_minimum_of(
            bytes_remaining, (uint64_t)MIMFS_BLOCK_SIZE_BYTES - offset_within_block);

        /*
         * A block that this write covers from its first byte to its last needs no
         * clearing, because every byte of it is about to be overwritten. Any other
         * block does, otherwise the part this write does not touch would expose the
         * contents left behind by whichever file owned the block before.
         *
         * This is the reason the allocator takes an initialisation policy at all: it
         * turns a full-block write from two passes over the block into one.
         */
        const bool write_covers_whole_block =
            offset_within_block == 0u && bytes_in_this_block == MIMFS_BLOCK_SIZE_BYTES;

        const enum mimfs_block_initialization_policy initialization_policy =
            write_covers_whole_block ? MIMFS_BLOCK_LEAVE_UNINITIALIZED
                                     : MIMFS_BLOCK_INITIALIZE_TO_ZERO;

        uint64_t physical_block_number = MIMFS_UNALLOCATED_BLOCK_NUMBER;

        const int resolve_status = mimfs_block_map_resolve(
            volume, inode, logical_block_index, MIMFS_BLOCK_MAP_LOOKUP_AND_ALLOCATE,
            initialization_policy, MIMFS_BLOCK_RESPECT_RESERVE, &physical_block_number);

        if (MIMFS_UNLIKELY(resolve_status != MIMFS_STATUS_SUCCESS))
        {
            /*
             * Out of space, or past the end of the addressable range. Whatever was
             * already placed is real and must be reported, so the length is committed
             * before the partial count is returned.
             */
            break;
        }

        unsigned char *block_bytes = mimfs_block_address(volume, physical_block_number);

        memcpy(block_bytes + offset_within_block, source_cursor,
               (size_t)bytes_in_this_block);

        source_cursor += bytes_in_this_block;
        current_offset += bytes_in_this_block;
        total_bytes_written += bytes_in_this_block;
        bytes_remaining -= bytes_in_this_block;
    }

    if (total_bytes_written == 0u)
    {
        return -MIMFS_STATUS_NO_SPACE_LEFT;
    }

    /*
     * A write only ever extends a file; writing into the middle of one leaves its
     * length alone. Taking the maximum expresses both cases without a branch on
     * which one applies.
     */
    (*inode).file_size_in_bytes = mimfs_maximum_of((*inode).file_size_in_bytes,
                                                   file_offset + total_bytes_written);

    mimfs_inode_stamp_modification_time(inode);

    return (long)total_bytes_written;
}

int mimfs_file_set_size(struct mimfs_volume *volume, struct mimfs_inode *inode,
                        uint64_t new_size_in_bytes)
{
    if (MIMFS_UNLIKELY(new_size_in_bytes > MIMFS_MAXIMUM_FILE_SIZE_BYTES))
    {
        return -MIMFS_STATUS_FILE_TOO_LARGE;
    }

    const uint64_t previous_size_in_bytes = (*inode).file_size_in_bytes;

    if (new_size_in_bytes == previous_size_in_bytes)
    {
        return MIMFS_STATUS_SUCCESS;
    }

    if (new_size_in_bytes < previous_size_in_bytes)
    {
        /*
         * The block that now contains the end of the file is kept, so the first block
         * to release is the one after it. Rounding up is what expresses that: a new
         * length of one byte keeps block zero and releases from block one.
         */
        const uint64_t first_logical_block_to_release =
            mimfs_divide_rounding_up(new_size_in_bytes, MIMFS_BLOCK_SIZE_BYTES);

        const uint64_t released_block_count =
            mimfs_block_map_release_from(volume, inode, first_logical_block_to_release);

        (*inode).allocated_block_count -=
            mimfs_minimum_of(released_block_count, (*inode).allocated_block_count);

        /*
         * Clear whatever remains in the final surviving block past the new end.
         *
         * Without this, growing the file again would expose the bytes that used to
         * follow the truncation point, which are no longer part of the file and which
         * the caller has every right to expect to read as zeroes.
         */
        const uint64_t bytes_used_in_final_block =
            MIMFS_OFFSET_WITHIN_BLOCK(new_size_in_bytes);

        if (bytes_used_in_final_block != 0u)
        {
            uint64_t final_block_number = MIMFS_UNALLOCATED_BLOCK_NUMBER;

            const int resolve_status = mimfs_block_map_resolve(
                volume, inode, MIMFS_LOGICAL_BLOCK_INDEX_OF_OFFSET(new_size_in_bytes),
                MIMFS_BLOCK_MAP_LOOKUP_ONLY, MIMFS_BLOCK_INITIALIZE_TO_ZERO,
                MIMFS_BLOCK_RESPECT_RESERVE, &final_block_number);

            if (resolve_status == MIMFS_STATUS_SUCCESS &&
                final_block_number != MIMFS_UNALLOCATED_BLOCK_NUMBER)
            {
                unsigned char *block_bytes =
                    mimfs_block_address(volume, final_block_number);

                memset(block_bytes + bytes_used_in_final_block, 0,
                       (size_t)(MIMFS_BLOCK_SIZE_BYTES - bytes_used_in_final_block));
            }
        }
    }

    /*
     * Growing needs no work beyond the length itself. The new span is a hole: it
     * reads as zeroes, and it will consume blocks only where something is written.
     */
    (*inode).file_size_in_bytes = new_size_in_bytes;

    mimfs_inode_stamp_modification_time(inode);

    return MIMFS_STATUS_SUCCESS;
}

int mimfs_file_append_cleared_block(struct mimfs_volume *volume,
                                    struct mimfs_inode *inode,
                                    uint64_t *appended_block_number_out)
{
    const uint64_t logical_block_index =
        mimfs_divide_rounding_up((*inode).file_size_in_bytes, MIMFS_BLOCK_SIZE_BYTES);

    uint64_t physical_block_number = MIMFS_UNALLOCATED_BLOCK_NUMBER;

    /*
     * Directory growth may draw on the reserve. A directory that cannot accept an
     * entry on a full volume is a directory that cannot be repaired, and leaving room
     * for that is the whole reason the reserve exists.
     */
    const int resolve_status = mimfs_block_map_resolve(
        volume, inode, logical_block_index, MIMFS_BLOCK_MAP_LOOKUP_AND_ALLOCATE,
        MIMFS_BLOCK_INITIALIZE_TO_ZERO, MIMFS_BLOCK_MAY_USE_RESERVE,
        &physical_block_number);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    (*inode).file_size_in_bytes =
        (logical_block_index + 1u) * (uint64_t)MIMFS_BLOCK_SIZE_BYTES;

    mimfs_inode_stamp_modification_time(inode);

    *appended_block_number_out = physical_block_number;

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Symbolic link target storage
 * ========================================================================= */

int mimfs_file_store_symbolic_link_target(struct mimfs_volume *volume,
                                          struct mimfs_inode *inode,
                                          const char *target_text, size_t target_length)
{
    if (MIMFS_UNLIKELY(target_text == NULL || target_length == 0u))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (MIMFS_UNLIKELY(target_length >= MIMFS_MAXIMUM_PATH_LENGTH))
    {
        return -MIMFS_STATUS_NAME_TOO_LONG;
    }

    /*
     * The overwhelming majority of real link targets are short, so the common case
     * is stored where the block pointers would go and costs no block, no allocation
     * and no indirection on every later resolution of the link.
     */
    if (target_length <= MIMFS_INLINE_SYMBOLIC_LINK_CAPACITY_BYTES)
    {
        memset(&(*inode).block_map_root, 0, sizeof((*inode).block_map_root));
        memcpy((*inode).block_map_root.inline_symbolic_link_target, target_text,
               target_length);

        (*inode).file_size_in_bytes = target_length;
        (*inode).allocated_block_count = 0u;

        mimfs_inode_stamp_modification_time(inode);

        return MIMFS_STATUS_SUCCESS;
    }

    const long bytes_written =
        mimfs_file_write_at_offset(volume, inode, 0u, target_text, target_length);

    if (bytes_written < 0)
    {
        return (int)bytes_written;
    }

    if ((size_t)bytes_written != target_length)
    {
        return -MIMFS_STATUS_NO_SPACE_LEFT;
    }

    return MIMFS_STATUS_SUCCESS;
}

long mimfs_file_load_symbolic_link_target(struct mimfs_volume *volume,
                                          struct mimfs_inode *inode,
                                          char *destination_buffer,
                                          size_t buffer_size_in_bytes)
{
    if (MIMFS_UNLIKELY(destination_buffer == NULL || buffer_size_in_bytes == 0u))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    const uint64_t target_length = (*inode).file_size_in_bytes;

    const uint64_t bytes_to_copy =
        mimfs_minimum_of(target_length, (uint64_t)buffer_size_in_bytes);

    if (mimfs_inode_uses_inline_symbolic_link_storage(inode))
    {
        memcpy(destination_buffer, (*inode).block_map_root.inline_symbolic_link_target,
               (size_t)bytes_to_copy);

        return (long)bytes_to_copy;
    }

    return mimfs_file_read_at_offset(volume, inode, 0u, destination_buffer,
                                     (size_t)bytes_to_copy);
}
