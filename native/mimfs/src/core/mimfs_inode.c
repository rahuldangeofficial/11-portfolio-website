/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_inode.c
 * Purpose: Implementation of single inode operations.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_inode.h"

#include "mimfs_block_map.h"
#include "mimfs_inode_allocator.h"
#include "support/mimfs_clock.h"

/* =========================================================================
 * Initialisation
 * ========================================================================= */

/*
 * Shared prologue of the three initialisers. The allocator already handed the
 * inode over cleared and stamped, so this only has to install the identity fields
 * that the allocator has no way to know.
 */
static void mimfs_inode_apply_common_initialization(struct mimfs_inode *inode,
                                                    uint32_t file_type_bits,
                                                    uint32_t permission_bits,
                                                    uint32_t owner_user_identifier,
                                                    uint32_t owner_group_identifier)
{
    (*inode).file_mode = (uint16_t)((file_type_bits & MIMFS_FILE_TYPE_MASK) |
                                    (permission_bits & MIMFS_PERMISSION_MASK));
    (*inode).owner_user_identifier = owner_user_identifier;
    (*inode).owner_group_identifier = owner_group_identifier;
    (*inode).hard_link_count = 0u;
    (*inode).file_size_in_bytes = 0u;
    (*inode).allocated_block_count = 0u;
}

void mimfs_inode_initialize_as_regular_file(struct mimfs_inode *inode,
                                            uint32_t permission_bits,
                                            uint32_t owner_user_identifier,
                                            uint32_t owner_group_identifier)
{
    mimfs_inode_apply_common_initialization(inode, MIMFS_FILE_TYPE_REGULAR,
                                            permission_bits, owner_user_identifier,
                                            owner_group_identifier);
}

void mimfs_inode_initialize_as_directory(struct mimfs_inode *inode,
                                         uint32_t permission_bits,
                                         uint32_t owner_user_identifier,
                                         uint32_t owner_group_identifier)
{
    mimfs_inode_apply_common_initialization(inode, MIMFS_FILE_TYPE_DIRECTORY,
                                            permission_bits, owner_user_identifier,
                                            owner_group_identifier);
}

void mimfs_inode_initialize_as_symbolic_link(struct mimfs_inode *inode,
                                             uint32_t owner_user_identifier,
                                             uint32_t owner_group_identifier)
{
    /*
     * A symbolic link carries no meaningful permissions of its own: access is
     * decided by the target it resolves to. It is therefore created fully
     * permissive, which is the established convention and avoids a link that
     * silently shadows an accessible target.
     */
    mimfs_inode_apply_common_initialization(inode, MIMFS_FILE_TYPE_SYMBOLIC_LINK, 0777u,
                                            owner_user_identifier,
                                            owner_group_identifier);
}

/* =========================================================================
 * Timestamps
 * ========================================================================= */

void mimfs_inode_stamp_access_time(struct mimfs_inode *inode)
{
    /* Relaxed: this store races with other readers by design, and no other field
     * depends on it, so there is nothing for stronger ordering to publish. */
    atomic_store_explicit(&(*inode).last_access_time_nanoseconds,
                          mimfs_clock_current_time_nanoseconds(), memory_order_relaxed);
}

void mimfs_inode_stamp_modification_time(struct mimfs_inode *inode)
{
    const int64_t current_time = mimfs_clock_current_time_nanoseconds();

    (*inode).last_modification_time_nanoseconds = current_time;
    (*inode).last_status_change_time_nanoseconds = current_time;
}

void mimfs_inode_stamp_status_change_time(struct mimfs_inode *inode)
{
    (*inode).last_status_change_time_nanoseconds =
        mimfs_clock_current_time_nanoseconds();
}

/* =========================================================================
 * Link counting
 * ========================================================================= */

bool mimfs_inode_increment_hard_link_count(struct mimfs_inode *inode)
{
    if (MIMFS_UNLIKELY((*inode).hard_link_count >= MIMFS_MAXIMUM_HARD_LINK_COUNT))
    {
        return false;
    }

    (*inode).hard_link_count++;
    mimfs_inode_stamp_status_change_time(inode);

    return true;
}

uint32_t mimfs_inode_decrement_hard_link_count(struct mimfs_inode *inode)
{
    if (MIMFS_UNLIKELY((*inode).hard_link_count == 0u))
    {
        return 0u;
    }

    (*inode).hard_link_count--;
    mimfs_inode_stamp_status_change_time(inode);

    return (*inode).hard_link_count;
}

/* =========================================================================
 * Reference counting and destruction
 * ========================================================================= */

void mimfs_inode_acquire_reference(struct mimfs_volume *volume, uint64_t inode_number)
{
    if (MIMFS_UNLIKELY(!mimfs_inode_number_is_in_range(volume, inode_number)))
    {
        return;
    }

    struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

    atomic_fetch_add_explicit(&(*inode).open_reference_count, 1u, memory_order_acq_rel);
}

bool mimfs_inode_destroy_if_orphaned(struct mimfs_volume *volume, uint64_t inode_number)
{
    if (MIMFS_UNLIKELY(!mimfs_inode_number_is_in_range(volume, inode_number)))
    {
        return false;
    }

    struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

    bool inode_was_destroyed = false;

    mimfs_inode_lock_exclusive(inode);

    /*
     * The three conditions, and why each is needed:
     *
     *   the inode is still live       another thread may have destroyed it already,
     *                                and a second destruction would release its
     *                                blocks twice
     *   no name refers to it         a named inode is reachable and must survive
     *   no reference refers to it    an open descriptor or a path lookup in flight
     *                                still needs it
     *
     * All three are read under the exclusive hold, so no thread can add a name or a
     * reference between the test and the destruction.
     */
    if (mimfs_inode_is_live(inode) && (*inode).hard_link_count == 0u &&
        atomic_load_explicit(&(*inode).open_reference_count, memory_order_acquire) ==
            0u)
    {
        /*
         * A short symbolic link keeps its target text where a file keeps its block
         * pointers, so walking the block map of one would free unrelated blocks.
         */
        if (!mimfs_inode_uses_inline_symbolic_link_storage(inode))
        {
            mimfs_block_map_release_all_blocks(volume, inode);
        }

        (*inode).file_size_in_bytes = 0u;
        (*inode).allocated_block_count = 0u;

        /*
         * Clearing the mode marks the inode as destroyed while the lock is still
         * held, which is what makes a second concurrent attempt a no-op instead of
         * a double release.
         */
        (*inode).file_mode = 0u;

        inode_was_destroyed = true;
    }

    mimfs_inode_unlock_exclusive(inode);

    /*
     * The inode number is returned to the allocator only after the lock has been
     * dropped. Publishing the number while still holding its lock would let another
     * thread claim it, initialise it, and take its lock, all while this thread is
     * about to release a lock it no longer owns.
     */
    if (inode_was_destroyed)
    {
        mimfs_inode_allocator_release(volume, inode_number);
    }

    return inode_was_destroyed;
}

void mimfs_inode_release_reference(struct mimfs_volume *volume, uint64_t inode_number)
{
    if (MIMFS_UNLIKELY(!mimfs_inode_number_is_in_range(volume, inode_number)))
    {
        return;
    }

    struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

    const uint32_t reference_count_before_release = atomic_fetch_sub_explicit(
        &(*inode).open_reference_count, 1u, memory_order_acq_rel);

    /*
     * Only the thread that removed the final reference goes on to consider
     * destruction. Every other thread returns immediately, so the common case of
     * dropping one of several references costs a single atomic decrement.
     */
    if (reference_count_before_release == 1u)
    {
        (void)mimfs_inode_destroy_if_orphaned(volume, inode_number);
    }
}

/* =========================================================================
 * Reporting
 * ========================================================================= */

uint32_t mimfs_inode_entry_type_tag(const struct mimfs_inode *inode)
{
    if (mimfs_inode_is_directory(inode))
    {
        return MIMFS_ENTRY_TYPE_DIRECTORY;
    }

    if (mimfs_inode_is_symbolic_link(inode))
    {
        return MIMFS_ENTRY_TYPE_SYMBOLIC_LINK;
    }

    if (mimfs_inode_is_regular_file(inode))
    {
        return MIMFS_ENTRY_TYPE_REGULAR;
    }

    return MIMFS_ENTRY_TYPE_UNKNOWN;
}

void mimfs_inode_capture_status(struct mimfs_volume *volume, uint64_t inode_number,
                                const struct mimfs_inode *inode,
                                struct mimfs_status_information *status_out)
{
    (*status_out).inode_number = (uint32_t)inode_number;
    (*status_out).file_mode = (*inode).file_mode;
    (*status_out).hard_link_count = (*inode).hard_link_count;
    (*status_out).owner_user_identifier = (*inode).owner_user_identifier;
    (*status_out).owner_group_identifier = (*inode).owner_group_identifier;
    (*status_out).file_size_in_bytes = (*inode).file_size_in_bytes;
    (*status_out).allocated_block_count = (*inode).allocated_block_count;
    (*status_out).block_size_in_bytes = (*volume).block_size_in_bytes;
    (*status_out).last_access_time_nanoseconds = atomic_load_explicit(
        &(*inode).last_access_time_nanoseconds, memory_order_relaxed);
    (*status_out).last_modification_time_nanoseconds =
        (*inode).last_modification_time_nanoseconds;
    (*status_out).last_status_change_time_nanoseconds =
        (*inode).last_status_change_time_nanoseconds;
    (*status_out).creation_time_nanoseconds = (*inode).creation_time_nanoseconds;
}
