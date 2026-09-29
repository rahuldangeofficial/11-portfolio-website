/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_directory.c
 * Purpose: Implementation of directory content operations.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_directory.h"

#include <string.h>

#include "mimfs_block_map.h"
#include "mimfs_file_io.h"
#include "mimfs_inode.h"

/* Number of entry slots a directory currently has room for. */
static uint64_t mimfs_directory_slot_capacity(const struct mimfs_inode *directory_inode)
{
    return ((*directory_inode).file_size_in_bytes / MIMFS_BLOCK_SIZE_BYTES) *
           MIMFS_DIRECTORY_ENTRIES_PER_BLOCK;
}

/* Number of blocks a directory currently occupies. */
static uint64_t mimfs_directory_block_count(const struct mimfs_inode *directory_inode)
{
    return (*directory_inode).file_size_in_bytes / MIMFS_BLOCK_SIZE_BYTES;
}

/*
 * Locate the array of entry slots that a directory's logical block holds.
 *
 * Returns NULL for a block that is not present. A directory should never contain a
 * hole, since every block of one is allocated the moment the directory grows into
 * it, so NULL here means either a damaged inode or a directory whose length is
 * larger than its map; either way the scan skips the block rather than reading an
 * address it did not verify.
 */
static struct mimfs_directory_entry *mimfs_directory_entry_array_of_block(
    struct mimfs_volume *volume, struct mimfs_inode *directory_inode,
    uint64_t logical_block_index, uint64_t *physical_block_number_out)
{
    uint64_t physical_block_number = MIMFS_UNALLOCATED_BLOCK_NUMBER;

    const int resolve_status = mimfs_block_map_resolve(
        volume, directory_inode, logical_block_index, MIMFS_BLOCK_MAP_LOOKUP_ONLY,
        MIMFS_BLOCK_INITIALIZE_TO_ZERO, MIMFS_BLOCK_RESPECT_RESERVE,
        &physical_block_number);

    if (MIMFS_UNLIKELY(resolve_status != MIMFS_STATUS_SUCCESS ||
                       physical_block_number == MIMFS_UNALLOCATED_BLOCK_NUMBER))
    {
        return NULL;
    }

    if (physical_block_number_out != NULL)
    {
        *physical_block_number_out = physical_block_number;
    }

    return mimfs_block_as_directory_entry_array(volume, physical_block_number);
}

/* True when the slot currently names an inode. */
MIMFS_ALWAYS_INLINE bool
mimfs_directory_entry_is_in_use(const struct mimfs_directory_entry *entry)
{
    return (*entry).inode_number != MIMFS_INVALID_INODE_NUMBER;
}

/*
 * Test one slot against a name.
 *
 * The hash is compared before anything else. That single integer comparison rejects
 * every candidate but the intended one, so the character comparison, which is the
 * only part that touches the second half of the cache line, runs once per lookup
 * rather than once per candidate.
 */
MIMFS_ALWAYS_INLINE bool
mimfs_directory_entry_matches_name(const struct mimfs_directory_entry *entry,
                                   const struct mimfs_name_reference *name)
{
    if (!mimfs_directory_entry_is_in_use(entry))
    {
        return false;
    }

    if ((*entry).name_hash != (*name).hash)
    {
        return false;
    }

    return mimfs_name_matches((*entry).name, (*entry).name_length, name);
}

/* Write a name into a slot, publishing the inode number last. */
static void mimfs_directory_entry_populate(struct mimfs_directory_entry *entry,
                                           const struct mimfs_name_reference *name,
                                           uint64_t inode_number, uint32_t entry_type)
{
    (*entry).name_hash = (*name).hash;
    (*entry).name_length = (uint16_t)(*name).length;
    (*entry).entry_type = (uint8_t)entry_type;
    (*entry).reserved_byte = 0u;

    memcpy((*entry).name, (*name).characters, (*name).length);

    /* Clear the unused tail so that two entries with the same name but different
     * lengths never differ only in bytes past the length, which would make a memory
     * dump of the directory misleading. */
    if ((*name).length < MIMFS_MAXIMUM_FILE_NAME_LENGTH)
    {
        memset((*entry).name + (*name).length, 0,
               MIMFS_MAXIMUM_FILE_NAME_LENGTH - (*name).length);
    }

    /*
     * The inode number is what makes the slot live, so it is stored last and after a
     * release fence. A concurrent reader holding the shared lock cannot be inside the
     * directory at this moment, since insertion holds the exclusive lock, but the
     * ordering also matters for a reader that acquires the lock immediately
     * afterwards and must see a fully formed name.
     */
    mimfs_publish_prior_stores();

    (*entry).inode_number = (uint32_t)inode_number;
}

/* =========================================================================
 * Lookup
 * ========================================================================= */

int mimfs_directory_lookup(struct mimfs_volume *volume,
                           struct mimfs_inode *directory_inode,
                           const struct mimfs_name_reference *name,
                           struct mimfs_directory_lookup_result *result_out)
{
    const uint64_t block_count = mimfs_directory_block_count(directory_inode);

    for (uint64_t logical_block_index = 0u; logical_block_index < block_count;
         logical_block_index++)
    {
        uint64_t physical_block_number = MIMFS_UNALLOCATED_BLOCK_NUMBER;

        struct mimfs_directory_entry *entry_array =
            mimfs_directory_entry_array_of_block(
                volume, directory_inode, logical_block_index, &physical_block_number);

        if (MIMFS_UNLIKELY(entry_array == NULL))
        {
            continue;
        }

        for (uint64_t entry_index = 0u; entry_index < MIMFS_DIRECTORY_ENTRIES_PER_BLOCK;
             entry_index++)
        {
            struct mimfs_directory_entry *entry = entry_array + entry_index;

            if (mimfs_directory_entry_matches_name(entry, name))
            {
                (*result_out).inode_number = (*entry).inode_number;
                (*result_out).entry_type = (*entry).entry_type;
                (*result_out).containing_block_number = physical_block_number;
                (*result_out).entry_index_within_block = (uint32_t)entry_index;

                return MIMFS_STATUS_SUCCESS;
            }
        }
    }

    return MIMFS_STATUS_NO_SUCH_ENTRY;
}

/* =========================================================================
 * Insertion
 * ========================================================================= */

int mimfs_directory_insert(struct mimfs_volume *volume,
                           struct mimfs_inode *directory_inode,
                           const struct mimfs_name_reference *name,
                           uint64_t inode_number, uint32_t entry_type)
{
    const uint64_t block_count = mimfs_directory_block_count(directory_inode);

    struct mimfs_directory_entry *first_free_slot = NULL;

    /*
     * One pass answers both questions: is the name already here, and where is the
     * first free slot. The whole directory has to be examined either way, because a
     * duplicate could sit past the first free slot.
     */
    for (uint64_t logical_block_index = 0u; logical_block_index < block_count;
         logical_block_index++)
    {
        struct mimfs_directory_entry *entry_array =
            mimfs_directory_entry_array_of_block(volume, directory_inode,
                                                 logical_block_index, NULL);

        if (MIMFS_UNLIKELY(entry_array == NULL))
        {
            continue;
        }

        for (uint64_t entry_index = 0u; entry_index < MIMFS_DIRECTORY_ENTRIES_PER_BLOCK;
             entry_index++)
        {
            struct mimfs_directory_entry *entry = entry_array + entry_index;

            if (!mimfs_directory_entry_is_in_use(entry))
            {
                if (first_free_slot == NULL)
                {
                    first_free_slot = entry;
                }

                continue;
            }

            if (mimfs_directory_entry_matches_name(entry, name))
            {
                return MIMFS_STATUS_ALREADY_EXISTS;
            }
        }
    }

    if (first_free_slot == NULL)
    {
        /* Every slot is taken; the directory needs another block of them. */
        uint64_t appended_block_number = MIMFS_UNALLOCATED_BLOCK_NUMBER;

        const int append_status = mimfs_file_append_cleared_block(
            volume, directory_inode, &appended_block_number);

        if (append_status != MIMFS_STATUS_SUCCESS)
        {
            return MIMFS_STATUS_NO_SPACE_LEFT;
        }

        first_free_slot =
            mimfs_block_as_directory_entry_array(volume, appended_block_number);
    }

    mimfs_directory_entry_populate(first_free_slot, name, inode_number, entry_type);

    mimfs_inode_stamp_modification_time(directory_inode);

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Removal
 * ========================================================================= */

/*
 * Give back the blocks at the end of a directory that hold nothing but free slots.
 *
 * Only trailing blocks are considered, because a free slot in the middle is still
 * useful to the next insertion, and because moving entries to close a gap would
 * invalidate the cursor of every directory stream currently open on this directory.
 */
static void
mimfs_directory_release_trailing_empty_blocks(struct mimfs_volume *volume,
                                              struct mimfs_inode *directory_inode)
{
    uint64_t block_count = mimfs_directory_block_count(directory_inode);

    /*
     * The first block is never released. It holds "." and ".." and therefore can
     * never be empty, and keeping it unconditionally means a directory always has at
     * least one block, which removes a special case from every other function here.
     */
    while (block_count > 1u)
    {
        const uint64_t last_block_index = block_count - 1u;

        struct mimfs_directory_entry *entry_array =
            mimfs_directory_entry_array_of_block(volume, directory_inode,
                                                 last_block_index, NULL);

        if (entry_array == NULL)
        {
            return;
        }

        bool block_holds_a_live_entry = false;

        for (uint64_t entry_index = 0u; entry_index < MIMFS_DIRECTORY_ENTRIES_PER_BLOCK;
             entry_index++)
        {
            if (mimfs_directory_entry_is_in_use(entry_array + entry_index))
            {
                block_holds_a_live_entry = true;
                break;
            }
        }

        if (block_holds_a_live_entry)
        {
            return;
        }

        const uint64_t released_block_count =
            mimfs_block_map_release_from(volume, directory_inode, last_block_index);

        (*directory_inode).allocated_block_count -= mimfs_minimum_of(
            released_block_count, (*directory_inode).allocated_block_count);

        block_count = last_block_index;
        (*directory_inode).file_size_in_bytes =
            block_count * (uint64_t)MIMFS_BLOCK_SIZE_BYTES;
    }
}

int mimfs_directory_remove(struct mimfs_volume *volume,
                           struct mimfs_inode *directory_inode,
                           const struct mimfs_name_reference *name,
                           uint64_t *removed_inode_number_out)
{
    struct mimfs_directory_lookup_result lookup_result;

    const int lookup_status =
        mimfs_directory_lookup(volume, directory_inode, name, &lookup_result);

    if (lookup_status != MIMFS_STATUS_SUCCESS)
    {
        return lookup_status;
    }

    struct mimfs_directory_entry *entry_array = mimfs_block_as_directory_entry_array(
        volume, lookup_result.containing_block_number);

    struct mimfs_directory_entry *entry =
        entry_array + lookup_result.entry_index_within_block;

    /*
     * Zeroing the inode number is the removal, and it happens in one store. Everything
     * else is tidying: a reader that saw the old value is holding the shared lock and
     * cannot be running now, and a reader that arrives afterwards sees a free slot.
     */
    (*entry).inode_number = MIMFS_INVALID_INODE_NUMBER;
    (*entry).name_hash = 0u;
    (*entry).name_length = 0u;
    (*entry).entry_type = MIMFS_ENTRY_TYPE_UNKNOWN;

    memset((*entry).name, 0, MIMFS_MAXIMUM_FILE_NAME_LENGTH);

    mimfs_directory_release_trailing_empty_blocks(volume, directory_inode);

    mimfs_inode_stamp_modification_time(directory_inode);

    if (removed_inode_number_out != NULL)
    {
        *removed_inode_number_out = lookup_result.inode_number;
    }

    return MIMFS_STATUS_SUCCESS;
}

int mimfs_directory_repoint(struct mimfs_volume *volume,
                            struct mimfs_inode *directory_inode,
                            const struct mimfs_name_reference *name,
                            uint64_t new_inode_number, uint32_t new_entry_type,
                            uint64_t *previous_inode_number_out)
{
    struct mimfs_directory_lookup_result lookup_result;

    const int lookup_status =
        mimfs_directory_lookup(volume, directory_inode, name, &lookup_result);

    if (lookup_status != MIMFS_STATUS_SUCCESS)
    {
        return lookup_status;
    }

    struct mimfs_directory_entry *entry_array = mimfs_block_as_directory_entry_array(
        volume, lookup_result.containing_block_number);

    struct mimfs_directory_entry *entry =
        entry_array + lookup_result.entry_index_within_block;

    if (previous_inode_number_out != NULL)
    {
        *previous_inode_number_out = (*entry).inode_number;
    }

    /* The name and its hash are unchanged, so only the target has to move, and it
     * moves in a single store. That is what makes the replacement atomic. */
    (*entry).entry_type = (uint8_t)new_entry_type;
    (*entry).inode_number = (uint32_t)new_inode_number;

    mimfs_inode_stamp_modification_time(directory_inode);

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Inspection and iteration
 * ========================================================================= */

bool mimfs_directory_is_empty(struct mimfs_volume *volume,
                              struct mimfs_inode *directory_inode)
{
    const uint64_t block_count = mimfs_directory_block_count(directory_inode);

    for (uint64_t logical_block_index = 0u; logical_block_index < block_count;
         logical_block_index++)
    {
        struct mimfs_directory_entry *entry_array =
            mimfs_directory_entry_array_of_block(volume, directory_inode,
                                                 logical_block_index, NULL);

        if (entry_array == NULL)
        {
            continue;
        }

        for (uint64_t entry_index = 0u; entry_index < MIMFS_DIRECTORY_ENTRIES_PER_BLOCK;
             entry_index++)
        {
            struct mimfs_directory_entry *entry = entry_array + entry_index;

            if (!mimfs_directory_entry_is_in_use(entry))
            {
                continue;
            }

            /* "." and ".." are present in every directory and say nothing about
             * whether it holds anything. */
            if (mimfs_name_is_current_directory((*entry).name, (*entry).name_length) ||
                mimfs_name_is_parent_directory((*entry).name, (*entry).name_length))
            {
                continue;
            }

            return false;
        }
    }

    return true;
}

int mimfs_directory_read_next_entry(struct mimfs_volume *volume,
                                    struct mimfs_inode *directory_inode,
                                    uint64_t *entry_cursor,
                                    struct mimfs_directory_entry_information *entry_out)
{
    const uint64_t slot_capacity = mimfs_directory_slot_capacity(directory_inode);

    while (*entry_cursor < slot_capacity)
    {
        const uint64_t logical_block_index =
            *entry_cursor / MIMFS_DIRECTORY_ENTRIES_PER_BLOCK;
        const uint64_t entry_index = *entry_cursor % MIMFS_DIRECTORY_ENTRIES_PER_BLOCK;

        struct mimfs_directory_entry *entry_array =
            mimfs_directory_entry_array_of_block(volume, directory_inode,
                                                 logical_block_index, NULL);

        if (MIMFS_UNLIKELY(entry_array == NULL))
        {
            /* Skip the whole absent block rather than one slot at a time. */
            *entry_cursor =
                (logical_block_index + 1u) * MIMFS_DIRECTORY_ENTRIES_PER_BLOCK;
            continue;
        }

        struct mimfs_directory_entry *entry = entry_array + entry_index;

        (*entry_cursor)++;

        if (!mimfs_directory_entry_is_in_use(entry))
        {
            continue;
        }

        const uint32_t name_length = (uint32_t)mimfs_minimum_of(
            (*entry).name_length, MIMFS_MAXIMUM_FILE_NAME_LENGTH);

        (*entry_out).inode_number = (*entry).inode_number;
        (*entry_out).entry_type = (*entry).entry_type;
        (*entry_out).name_length = name_length;

        memcpy((*entry_out).name, (*entry).name, name_length);

        /* The stored name carries no terminator; the reported one does, so that a
         * caller can hand it straight to any string function. */
        *((*entry_out).name + name_length) = '\0';

        return 1;
    }

    return 0;
}

/* =========================================================================
 * Creation
 * ========================================================================= */

int mimfs_directory_install_dot_entries(struct mimfs_volume *volume,
                                        struct mimfs_inode *directory_inode,
                                        uint64_t directory_inode_number,
                                        uint64_t parent_inode_number)
{
    struct mimfs_name_reference current_directory_name;
    struct mimfs_name_reference parent_directory_name;

    if (!mimfs_name_reference_initialize(&current_directory_name, ".", 1u) ||
        !mimfs_name_reference_initialize(&parent_directory_name, "..", 2u))
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    const int current_directory_status =
        mimfs_directory_insert(volume, directory_inode, &current_directory_name,
                               directory_inode_number, MIMFS_ENTRY_TYPE_DIRECTORY);

    if (current_directory_status != MIMFS_STATUS_SUCCESS)
    {
        return current_directory_status;
    }

    const int parent_directory_status =
        mimfs_directory_insert(volume, directory_inode, &parent_directory_name,
                               parent_inode_number, MIMFS_ENTRY_TYPE_DIRECTORY);

    if (parent_directory_status != MIMFS_STATUS_SUCCESS)
    {
        return parent_directory_status;
    }

    /*
     * One link, for the "." just inserted. The caller adds the second when it inserts
     * this directory's name into its parent. Splitting the accounting this way keeps
     * every link increment next to the entry that caused it.
     */
    (*directory_inode).hard_link_count = 1u;

    return MIMFS_STATUS_SUCCESS;
}

int mimfs_directory_update_parent_entry(struct mimfs_volume *volume,
                                        struct mimfs_inode *directory_inode,
                                        uint64_t new_parent_inode_number)
{
    struct mimfs_name_reference parent_directory_name;

    if (!mimfs_name_reference_initialize(&parent_directory_name, "..", 2u))
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_directory_repoint(volume, directory_inode, &parent_directory_name,
                                   new_parent_inode_number, MIMFS_ENTRY_TYPE_DIRECTORY,
                                   NULL);
}

int mimfs_directory_create_root(struct mimfs_volume *volume)
{
    struct mimfs_inode *root_inode = mimfs_inode_at(volume, MIMFS_ROOT_INODE_NUMBER);

    mimfs_inode_initialize_as_directory(root_inode, MIMFS_PERMISSION_DEFAULT_DIRECTORY,
                                        0u, 0u);

    const int install_status = mimfs_directory_install_dot_entries(
        volume, root_inode, MIMFS_ROOT_INODE_NUMBER, MIMFS_ROOT_INODE_NUMBER);

    if (install_status != MIMFS_STATUS_SUCCESS)
    {
        return install_status;
    }

    /*
     * The root is its own parent, so its ".." names it as well. That second reference
     * takes its link count to two, which is the same count every other empty
     * directory has and lets the root be treated uniformly everywhere else.
     */
    (*root_inode).hard_link_count = 2u;

    return MIMFS_STATUS_SUCCESS;
}
