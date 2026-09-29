/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_namespace.c
 * Purpose: Implementation of the namespace operations.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_namespace.h"

#include <string.h>

#include "mimfs_directory.h"
#include "mimfs_file_io.h"
#include "mimfs_inode.h"
#include "mimfs_inode_allocator.h"
#include "mimfs_path_resolver.h"
#include "support/mimfs_clock.h"

/*
 * The two names that every directory contains and that no operation may create,
 * remove or move. Rejecting them in one place keeps the check from being forgotten in
 * any of the five operations that need it.
 */
static bool
mimfs_name_is_reserved_directory_entry(const struct mimfs_name_reference *name)
{
    return mimfs_name_is_current_directory((*name).characters, (*name).length) ||
           mimfs_name_is_parent_directory((*name).characters, (*name).length);
}

/*
 * Resolve a path to its parent directory and its final component, rejecting the
 * component names no operation may touch.
 *
 * Every create, remove and rename operation begins with exactly this, so it is
 * factored out rather than repeated six times with six chances to omit a check.
 * On success a reference is held on the parent.
 */
static int mimfs_resolve_parent_and_name(struct mimfs_volume *volume, const char *path,
                                         struct mimfs_path_resolution *resolution_out,
                                         struct mimfs_name_reference *name_out)
{
    const int resolve_status =
        mimfs_path_resolve(volume, path, MIMFS_RESOLVE_TO_PARENT, resolution_out);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return resolve_status;
    }

    if (!mimfs_path_resolution_final_name(resolution_out, name_out))
    {
        mimfs_path_resolution_release(volume, resolution_out);
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (mimfs_name_is_reserved_directory_entry(name_out))
    {
        mimfs_path_resolution_release(volume, resolution_out);
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Creating a directory
 * ========================================================================= */

int mimfs_namespace_create_directory(struct mimfs_volume *volume, const char *path,
                                     uint32_t permission_bits)
{
    struct mimfs_path_resolution parent_resolution;
    struct mimfs_name_reference new_directory_name;

    const int resolve_status = mimfs_resolve_parent_and_name(
        volume, path, &parent_resolution, &new_directory_name);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    const uint64_t parent_inode_number = parent_resolution.inode_number;

    /*
     * A directory is placed away from its parent on purpose. Keeping the whole tree
     * in one group would funnel every file created under it into that group's blocks
     * as well, which is precisely the concentration the group structure exists to
     * avoid.
     */
    const uint64_t new_directory_inode_number = mimfs_inode_allocator_allocate(
        volume, parent_inode_number, MIMFS_INODE_PLACE_AWAY_FROM_PARENT);

    if (new_directory_inode_number == MIMFS_INVALID_INODE_NUMBER)
    {
        mimfs_path_resolution_release(volume, &parent_resolution);
        return -MIMFS_STATUS_NO_SPACE_LEFT;
    }

    struct mimfs_inode *new_directory_inode =
        mimfs_inode_at(volume, new_directory_inode_number);

    /*
     * The new directory is given its shape while it is still unreachable, and its lock
     * is released before the parent's is taken. Holding a child's lock while reaching
     * for its parent's would invert the ordering rule this whole layer depends on,
     * even though nothing could contend on this particular child yet.
     */
    mimfs_inode_lock_exclusive(new_directory_inode);

    mimfs_inode_initialize_as_directory(
        new_directory_inode, permission_bits & MIMFS_PERMISSION_MASK, 0u, 0u);

    int construction_status = mimfs_directory_install_dot_entries(
        volume, new_directory_inode, new_directory_inode_number, parent_inode_number);

    if (construction_status == MIMFS_STATUS_SUCCESS)
    {
        /*
         * Two links: the "." just installed, and the name about to be inserted into
         * the parent. Charging the second one now, before it exists, is safe because
         * nothing can observe this inode until the insertion succeeds, and it means the
         * count is already correct at the instant the name becomes visible.
         */
        (*new_directory_inode).hard_link_count = 2u;
    }

    mimfs_inode_unlock_exclusive(new_directory_inode);

    if (construction_status == MIMFS_STATUS_SUCCESS)
    {
        struct mimfs_inode *parent_inode = mimfs_inode_at(volume, parent_inode_number);

        mimfs_inode_lock_exclusive(parent_inode);

        if (!mimfs_inode_is_directory(parent_inode))
        {
            construction_status = MIMFS_STATUS_NOT_A_DIRECTORY;
        }
        else
        {
            construction_status = mimfs_directory_insert(
                volume, parent_inode, &new_directory_name, new_directory_inode_number,
                MIMFS_ENTRY_TYPE_DIRECTORY);

            if (construction_status == MIMFS_STATUS_SUCCESS)
            {
                /* The ".." of the new directory is a reference to the parent. */
                (void)mimfs_inode_increment_hard_link_count(parent_inode);
            }
        }

        mimfs_inode_unlock_exclusive(parent_inode);
    }

    mimfs_path_resolution_release(volume, &parent_resolution);

    if (construction_status != MIMFS_STATUS_SUCCESS)
    {
        /* Unwind: drop the links charged in advance so that the inode is recognised as
         * an orphan and destroyed along with the block its entries occupied. */
        mimfs_inode_lock_exclusive(new_directory_inode);
        (*new_directory_inode).hard_link_count = 0u;
        mimfs_inode_unlock_exclusive(new_directory_inode);

        (void)mimfs_inode_destroy_if_orphaned(volume, new_directory_inode_number);

        return -construction_status;
    }

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Removing a directory
 * ========================================================================= */

int mimfs_namespace_remove_directory(struct mimfs_volume *volume, const char *path)
{
    struct mimfs_path_resolution parent_resolution;
    struct mimfs_name_reference directory_name;

    const int resolve_status = mimfs_resolve_parent_and_name(
        volume, path, &parent_resolution, &directory_name);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    const uint64_t parent_inode_number = parent_resolution.inode_number;
    struct mimfs_inode *parent_inode = mimfs_inode_at(volume, parent_inode_number);

    uint64_t directory_inode_number = MIMFS_INVALID_INODE_NUMBER;
    int removal_status = MIMFS_STATUS_SUCCESS;

    mimfs_inode_lock_exclusive(parent_inode);

    struct mimfs_directory_lookup_result lookup_result;

    removal_status =
        mimfs_directory_lookup(volume, parent_inode, &directory_name, &lookup_result);

    if (removal_status == MIMFS_STATUS_SUCCESS)
    {
        directory_inode_number = lookup_result.inode_number;

        struct mimfs_inode *directory_inode =
            mimfs_inode_at(volume, directory_inode_number);

        /* Parent first, then child. */
        mimfs_inode_lock_exclusive(directory_inode);

        if (!mimfs_inode_is_directory(directory_inode))
        {
            removal_status = MIMFS_STATUS_NOT_A_DIRECTORY;
        }
        else if (!mimfs_directory_is_empty(volume, directory_inode))
        {
            removal_status = MIMFS_STATUS_DIRECTORY_NOT_EMPTY;
        }
        else
        {
            removal_status =
                mimfs_directory_remove(volume, parent_inode, &directory_name, NULL);

            if (removal_status == MIMFS_STATUS_SUCCESS)
            {
                /*
                 * Both of the directory's links go at once: the name in the parent that
                 * was just removed, and its own ".", which cannot outlive it.
                 */
                (*directory_inode).hard_link_count = 0u;

                /* The parent loses the reference held by the removed directory's "..".
                 */
                (void)mimfs_inode_decrement_hard_link_count(parent_inode);
            }
        }

        mimfs_inode_unlock_exclusive(directory_inode);
    }

    mimfs_inode_unlock_exclusive(parent_inode);

    mimfs_path_resolution_release(volume, &parent_resolution);

    if (removal_status != MIMFS_STATUS_SUCCESS)
    {
        return -removal_status;
    }

    /*
     * Destruction happens outside every lock. A directory that some thread still has
     * open survives here and is destroyed when that thread closes it, which is the
     * same behaviour a removed but still open file gets.
     */
    (void)mimfs_inode_destroy_if_orphaned(volume, directory_inode_number);

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Hard links
 * ========================================================================= */

int mimfs_namespace_create_hard_link(struct mimfs_volume *volume,
                                     const char *existing_path, const char *new_path)
{
    struct mimfs_path_resolution existing_resolution;

    /*
     * The existing path is resolved without following a link in its final component,
     * so that linking to a symbolic link produces a second name for the link itself
     * rather than for whatever it happens to point at today.
     */
    int status = mimfs_path_resolve(volume, existing_path, 0u, &existing_resolution);

    if (status != MIMFS_STATUS_SUCCESS)
    {
        return -status;
    }

    const uint64_t target_inode_number = existing_resolution.inode_number;

    struct mimfs_path_resolution parent_resolution;
    struct mimfs_name_reference new_link_name;

    status = mimfs_resolve_parent_and_name(volume, new_path, &parent_resolution,
                                           &new_link_name);

    if (status != MIMFS_STATUS_SUCCESS)
    {
        mimfs_path_resolution_release(volume, &existing_resolution);
        return -status;
    }

    const uint64_t parent_inode_number = parent_resolution.inode_number;
    struct mimfs_inode *parent_inode = mimfs_inode_at(volume, parent_inode_number);
    struct mimfs_inode *target_inode = mimfs_inode_at(volume, target_inode_number);

    /* Directory first, then the file it will name. */
    mimfs_inode_lock_exclusive(parent_inode);
    mimfs_inode_lock_exclusive(target_inode);

    if (!mimfs_inode_is_directory(parent_inode))
    {
        status = MIMFS_STATUS_NOT_A_DIRECTORY;
    }
    else if (mimfs_inode_is_directory(target_inode))
    {
        /*
         * A second name for a directory would give the tree a cycle, and a cycle makes
         * "walk to the root" a non-terminating operation. Refused, as it is by every
         * filesystem that intends to stay a tree.
         */
        status = MIMFS_STATUS_NOT_PERMITTED;
    }
    else if (!mimfs_inode_is_live(target_inode))
    {
        status = MIMFS_STATUS_NO_SUCH_ENTRY;
    }
    else if ((*target_inode).hard_link_count >= MIMFS_MAXIMUM_HARD_LINK_COUNT)
    {
        status = MIMFS_STATUS_TOO_MANY_LINKS;
    }
    else
    {
        status = mimfs_directory_insert(volume, parent_inode, &new_link_name,
                                        target_inode_number,
                                        mimfs_inode_entry_type_tag(target_inode));

        if (status == MIMFS_STATUS_SUCCESS)
        {
            (void)mimfs_inode_increment_hard_link_count(target_inode);
        }
    }

    mimfs_inode_unlock_exclusive(target_inode);
    mimfs_inode_unlock_exclusive(parent_inode);

    mimfs_path_resolution_release(volume, &parent_resolution);
    mimfs_path_resolution_release(volume, &existing_resolution);

    return status == MIMFS_STATUS_SUCCESS ? MIMFS_STATUS_SUCCESS : -status;
}

int mimfs_namespace_remove_link(struct mimfs_volume *volume, const char *path)
{
    struct mimfs_path_resolution parent_resolution;
    struct mimfs_name_reference link_name;

    const int resolve_status =
        mimfs_resolve_parent_and_name(volume, path, &parent_resolution, &link_name);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    const uint64_t parent_inode_number = parent_resolution.inode_number;
    struct mimfs_inode *parent_inode = mimfs_inode_at(volume, parent_inode_number);

    uint64_t target_inode_number = MIMFS_INVALID_INODE_NUMBER;
    int status;

    mimfs_inode_lock_exclusive(parent_inode);

    struct mimfs_directory_lookup_result lookup_result;

    status = mimfs_directory_lookup(volume, parent_inode, &link_name, &lookup_result);

    if (status == MIMFS_STATUS_SUCCESS)
    {
        target_inode_number = lookup_result.inode_number;

        struct mimfs_inode *target_inode = mimfs_inode_at(volume, target_inode_number);

        mimfs_inode_lock_exclusive(target_inode);

        if (mimfs_inode_is_directory(target_inode))
        {
            /* Directories are removed by the operation that knows how to check that
             * they are empty. */
            status = MIMFS_STATUS_IS_A_DIRECTORY;
        }
        else
        {
            status = mimfs_directory_remove(volume, parent_inode, &link_name, NULL);

            if (status == MIMFS_STATUS_SUCCESS)
            {
                (void)mimfs_inode_decrement_hard_link_count(target_inode);
            }
        }

        mimfs_inode_unlock_exclusive(target_inode);
    }

    mimfs_inode_unlock_exclusive(parent_inode);

    mimfs_path_resolution_release(volume, &parent_resolution);

    if (status != MIMFS_STATUS_SUCCESS)
    {
        return -status;
    }

    /*
     * The name is gone from the namespace already. Whether the inode itself goes now
     * depends on whether anyone still holds it open, which is exactly the semantics a
     * caller expects: a file being read can be removed, and the reader keeps reading.
     */
    (void)mimfs_inode_destroy_if_orphaned(volume, target_inode_number);

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Symbolic links
 * ========================================================================= */

int mimfs_namespace_create_symbolic_link(struct mimfs_volume *volume,
                                         const char *target_text, const char *link_path)
{
    if (target_text == NULL)
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    const size_t target_length = strlen(target_text);

    if (target_length == 0u)
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (target_length >= MIMFS_MAXIMUM_PATH_LENGTH)
    {
        return -MIMFS_STATUS_NAME_TOO_LONG;
    }

    struct mimfs_path_resolution parent_resolution;
    struct mimfs_name_reference link_name;

    const int resolve_status = mimfs_resolve_parent_and_name(
        volume, link_path, &parent_resolution, &link_name);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    const uint64_t parent_inode_number = parent_resolution.inode_number;

    const uint64_t link_inode_number = mimfs_inode_allocator_allocate(
        volume, parent_inode_number, MIMFS_INODE_PLACE_NEAR_PARENT);

    if (link_inode_number == MIMFS_INVALID_INODE_NUMBER)
    {
        mimfs_path_resolution_release(volume, &parent_resolution);
        return -MIMFS_STATUS_NO_SPACE_LEFT;
    }

    struct mimfs_inode *link_inode = mimfs_inode_at(volume, link_inode_number);

    mimfs_inode_lock_exclusive(link_inode);

    mimfs_inode_initialize_as_symbolic_link(link_inode, 0u, 0u);

    int status = mimfs_file_store_symbolic_link_target(volume, link_inode, target_text,
                                                       target_length);

    if (status < 0)
    {
        status = -status;
    }

    if (status == MIMFS_STATUS_SUCCESS)
    {
        (*link_inode).hard_link_count = 1u;
    }

    mimfs_inode_unlock_exclusive(link_inode);

    if (status == MIMFS_STATUS_SUCCESS)
    {
        struct mimfs_inode *parent_inode = mimfs_inode_at(volume, parent_inode_number);

        mimfs_inode_lock_exclusive(parent_inode);

        if (!mimfs_inode_is_directory(parent_inode))
        {
            status = MIMFS_STATUS_NOT_A_DIRECTORY;
        }
        else
        {
            status = mimfs_directory_insert(volume, parent_inode, &link_name,
                                            link_inode_number,
                                            MIMFS_ENTRY_TYPE_SYMBOLIC_LINK);
        }

        mimfs_inode_unlock_exclusive(parent_inode);
    }

    mimfs_path_resolution_release(volume, &parent_resolution);

    if (status != MIMFS_STATUS_SUCCESS)
    {
        mimfs_inode_lock_exclusive(link_inode);
        (*link_inode).hard_link_count = 0u;
        mimfs_inode_unlock_exclusive(link_inode);

        (void)mimfs_inode_destroy_if_orphaned(volume, link_inode_number);

        return -status;
    }

    return MIMFS_STATUS_SUCCESS;
}

long mimfs_namespace_read_symbolic_link(struct mimfs_volume *volume, const char *path,
                                        char *destination_buffer,
                                        size_t buffer_size_in_bytes)
{
    if (destination_buffer == NULL || buffer_size_in_bytes == 0u)
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    struct mimfs_path_resolution resolution;

    /* No follow: the point of this operation is to see the link, not its target. */
    const int resolve_status = mimfs_path_resolve(volume, path, 0u, &resolution);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    struct mimfs_inode *link_inode = mimfs_inode_at(volume, resolution.inode_number);

    mimfs_inode_lock_shared(link_inode);

    long result;

    if (!mimfs_inode_is_symbolic_link(link_inode))
    {
        result = -MIMFS_STATUS_INVALID_ARGUMENT;
    }
    else
    {
        result = mimfs_file_load_symbolic_link_target(
            volume, link_inode, destination_buffer, buffer_size_in_bytes);
    }

    mimfs_inode_unlock_shared(link_inode);

    mimfs_path_resolution_release(volume, &resolution);

    return result;
}

/* =========================================================================
 * Rename
 * ========================================================================= */

/*
 * Refuse to move a directory into a place beneath itself.
 *
 * Such a move would detach the whole subtree from the root and make it a ring, after
 * which walking upwards never terminates and nothing in the subtree can ever be
 * reached or freed again. The check walks upwards from the intended destination,
 * following each directory's "..", and fails if the directory being moved appears on
 * the way.
 *
 * It is correct to perform this before the destination's lock is taken because the
 * caller holds the volume's rename lock, and rename is the only operation that can
 * change which directory contains which. No other thread can alter the chain being
 * walked here while that lock is held.
 */
static int
mimfs_verify_directory_move_is_not_into_itself(struct mimfs_volume *volume,
                                               uint64_t moving_inode_number,
                                               uint64_t destination_parent_number)
{
    if (moving_inode_number == destination_parent_number)
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    struct mimfs_name_reference parent_directory_name;

    if (!mimfs_name_reference_initialize(&parent_directory_name, "..", 2u))
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    uint64_t ancestor_inode_number = destination_parent_number;

    /*
     * The walk is bounded by the number of inodes, which makes it terminate even on a
     * volume whose parent chain has already been damaged into a ring. Without the
     * bound, a corrupted volume would turn this into an infinite loop.
     */
    for (uint64_t step_count = 0u; step_count <= (*volume).total_inode_count;
         step_count++)
    {
        if (ancestor_inode_number == MIMFS_ROOT_INODE_NUMBER)
        {
            return MIMFS_STATUS_SUCCESS;
        }

        if (ancestor_inode_number == moving_inode_number)
        {
            return MIMFS_STATUS_INVALID_ARGUMENT;
        }

        struct mimfs_inode *ancestor_inode =
            mimfs_inode_at(volume, ancestor_inode_number);

        mimfs_inode_lock_shared(ancestor_inode);

        struct mimfs_directory_lookup_result lookup_result;

        const int lookup_status = mimfs_directory_lookup(
            volume, ancestor_inode, &parent_directory_name, &lookup_result);

        mimfs_inode_unlock_shared(ancestor_inode);

        if (lookup_status != MIMFS_STATUS_SUCCESS)
        {
            return MIMFS_STATUS_CORRUPTED_VOLUME;
        }

        if (lookup_result.inode_number == ancestor_inode_number)
        {
            /* A directory that is its own parent is the root, by definition. */
            return MIMFS_STATUS_SUCCESS;
        }

        ancestor_inode_number = lookup_result.inode_number;
    }

    return MIMFS_STATUS_CORRUPTED_VOLUME;
}

/* Take the locks of the two parent directories, in ascending inode number order.
 * When both paths share a parent there is only one lock to take, and taking it twice
 * would deadlock against itself. */
static void mimfs_lock_both_parents(struct mimfs_inode *first_parent_inode,
                                    struct mimfs_inode *second_parent_inode,
                                    uint64_t first_parent_number,
                                    uint64_t second_parent_number)
{
    if (first_parent_number == second_parent_number)
    {
        mimfs_inode_lock_exclusive(first_parent_inode);
        return;
    }

    if (first_parent_number < second_parent_number)
    {
        mimfs_inode_lock_exclusive(first_parent_inode);
        mimfs_inode_lock_exclusive(second_parent_inode);
    }
    else
    {
        mimfs_inode_lock_exclusive(second_parent_inode);
        mimfs_inode_lock_exclusive(first_parent_inode);
    }
}

static void mimfs_unlock_both_parents(struct mimfs_inode *first_parent_inode,
                                      struct mimfs_inode *second_parent_inode,
                                      uint64_t first_parent_number,
                                      uint64_t second_parent_number)
{
    mimfs_inode_unlock_exclusive(first_parent_inode);

    if (first_parent_number != second_parent_number)
    {
        mimfs_inode_unlock_exclusive(second_parent_inode);
    }
}

int mimfs_namespace_rename(struct mimfs_volume *volume, const char *old_path,
                           const char *new_path)
{
    struct mimfs_path_resolution source_parent_resolution;
    struct mimfs_path_resolution destination_parent_resolution;
    struct mimfs_name_reference source_name;
    struct mimfs_name_reference destination_name;

    /*
     * Serialise against every other rename before anything else. This is the lock that
     * makes it safe for one operation to hold two directories at once and to reason
     * about the ancestry of a third.
     */
    mimfs_reader_writer_lock_acquire_exclusive(&(*volume).rename_serialization_lock);

    int status = mimfs_resolve_parent_and_name(volume, old_path,
                                               &source_parent_resolution, &source_name);

    if (status != MIMFS_STATUS_SUCCESS)
    {
        mimfs_reader_writer_lock_release_exclusive(
            &(*volume).rename_serialization_lock);
        return -status;
    }

    status = mimfs_resolve_parent_and_name(
        volume, new_path, &destination_parent_resolution, &destination_name);

    if (status != MIMFS_STATUS_SUCCESS)
    {
        mimfs_path_resolution_release(volume, &source_parent_resolution);
        mimfs_reader_writer_lock_release_exclusive(
            &(*volume).rename_serialization_lock);
        return -status;
    }

    const uint64_t source_parent_number = source_parent_resolution.inode_number;
    const uint64_t destination_parent_number =
        destination_parent_resolution.inode_number;

    struct mimfs_inode *source_parent_inode =
        mimfs_inode_at(volume, source_parent_number);
    struct mimfs_inode *destination_parent_inode =
        mimfs_inode_at(volume, destination_parent_number);

    /*
     * Identify the object being moved before any parent lock is taken, because the
     * ancestry check below needs to know whether it is a directory and has to run
     * before those locks exist. The entry is looked up again once the locks are held,
     * so a concurrent removal in between is caught rather than acted upon.
     */
    uint64_t source_inode_number = MIMFS_INVALID_INODE_NUMBER;
    bool source_is_directory = false;

    mimfs_inode_lock_shared(source_parent_inode);

    struct mimfs_directory_lookup_result preliminary_lookup;

    status = mimfs_directory_lookup(volume, source_parent_inode, &source_name,
                                    &preliminary_lookup);

    if (status == MIMFS_STATUS_SUCCESS)
    {
        source_inode_number = preliminary_lookup.inode_number;
        source_is_directory =
            preliminary_lookup.entry_type == MIMFS_ENTRY_TYPE_DIRECTORY;
        mimfs_inode_acquire_reference(volume, source_inode_number);
    }

    mimfs_inode_unlock_shared(source_parent_inode);

    if (status != MIMFS_STATUS_SUCCESS)
    {
        mimfs_path_resolution_release(volume, &destination_parent_resolution);
        mimfs_path_resolution_release(volume, &source_parent_resolution);
        mimfs_reader_writer_lock_release_exclusive(
            &(*volume).rename_serialization_lock);
        return -status;
    }

    if (source_is_directory)
    {
        status = mimfs_verify_directory_move_is_not_into_itself(
            volume, source_inode_number, destination_parent_number);

        if (status != MIMFS_STATUS_SUCCESS)
        {
            mimfs_inode_release_reference(volume, source_inode_number);
            mimfs_path_resolution_release(volume, &destination_parent_resolution);
            mimfs_path_resolution_release(volume, &source_parent_resolution);
            mimfs_reader_writer_lock_release_exclusive(
                &(*volume).rename_serialization_lock);
            return -status;
        }
    }

    uint64_t replaced_inode_number = MIMFS_INVALID_INODE_NUMBER;
    bool replaced_inode_is_directory = false;

    mimfs_lock_both_parents(source_parent_inode, destination_parent_inode,
                            source_parent_number, destination_parent_number);

    /* Re-establish the source entry now that the locks are held. */
    struct mimfs_directory_lookup_result source_lookup;

    status = mimfs_directory_lookup(volume, source_parent_inode, &source_name,
                                    &source_lookup);

    if (status == MIMFS_STATUS_SUCCESS &&
        source_lookup.inode_number != source_inode_number)
    {
        /* The name now refers to something else; the operation the caller asked for no
         * longer describes anything that exists. */
        status = MIMFS_STATUS_NO_SUCH_ENTRY;
    }

    struct mimfs_directory_lookup_result destination_lookup;
    bool destination_exists = false;

    if (status == MIMFS_STATUS_SUCCESS)
    {
        destination_exists =
            mimfs_directory_lookup(volume, destination_parent_inode, &destination_name,
                                   &destination_lookup) == MIMFS_STATUS_SUCCESS;
    }

    if (status == MIMFS_STATUS_SUCCESS && destination_exists)
    {
        replaced_inode_number = destination_lookup.inode_number;

        if (replaced_inode_number == source_inode_number)
        {
            /*
             * The two paths already name the same inode. There is nothing to move, and
             * reporting success is the required behaviour rather than a shortcut.
             */
            status = MIMFS_STATUS_SUCCESS;
            destination_exists = false;
            replaced_inode_number = MIMFS_INVALID_INODE_NUMBER;

            mimfs_unlock_both_parents(source_parent_inode, destination_parent_inode,
                                      source_parent_number, destination_parent_number);

            mimfs_inode_release_reference(volume, source_inode_number);
            mimfs_path_resolution_release(volume, &destination_parent_resolution);
            mimfs_path_resolution_release(volume, &source_parent_resolution);
            mimfs_reader_writer_lock_release_exclusive(
                &(*volume).rename_serialization_lock);

            return MIMFS_STATUS_SUCCESS;
        }

        struct mimfs_inode *replaced_inode =
            mimfs_inode_at(volume, replaced_inode_number);

        mimfs_inode_lock_shared(replaced_inode);

        replaced_inode_is_directory = mimfs_inode_is_directory(replaced_inode);

        const bool replaced_directory_is_empty =
            replaced_inode_is_directory &&
            mimfs_directory_is_empty(volume, replaced_inode);

        mimfs_inode_unlock_shared(replaced_inode);

        /*
         * The two ends have to agree on being a directory. Letting a file replace a
         * directory would orphan everything inside it, and letting a directory replace
         * a file would leave a caller expecting a file holding a directory.
         */
        if (source_is_directory && !replaced_inode_is_directory)
        {
            status = MIMFS_STATUS_NOT_A_DIRECTORY;
        }
        else if (!source_is_directory && replaced_inode_is_directory)
        {
            status = MIMFS_STATUS_IS_A_DIRECTORY;
        }
        else if (replaced_inode_is_directory && !replaced_directory_is_empty)
        {
            status = MIMFS_STATUS_DIRECTORY_NOT_EMPTY;
        }
    }

    if (status == MIMFS_STATUS_SUCCESS)
    {
        /*
         * The destination is established before the source is removed, so that a
         * failure to create it leaves the tree exactly as it was. When the destination
         * already exists it is repointed, which replaces it in a single store and is
         * what makes the replacement atomic for any concurrent observer.
         */
        if (destination_exists)
        {
            status =
                mimfs_directory_repoint(volume, destination_parent_inode,
                                        &destination_name, source_inode_number,
                                        source_is_directory ? MIMFS_ENTRY_TYPE_DIRECTORY
                                                            : source_lookup.entry_type,
                                        NULL);
        }
        else
        {
            status = mimfs_directory_insert(volume, destination_parent_inode,
                                            &destination_name, source_inode_number,
                                            source_lookup.entry_type);
        }
    }

    if (status == MIMFS_STATUS_SUCCESS)
    {
        status =
            mimfs_directory_remove(volume, source_parent_inode, &source_name, NULL);
    }

    if (status == MIMFS_STATUS_SUCCESS && source_is_directory &&
        source_parent_number != destination_parent_number)
    {
        /* A moved directory's ".." has to follow it, and the two parents' link counts
         * have to follow that. */
        struct mimfs_inode *source_inode = mimfs_inode_at(volume, source_inode_number);

        mimfs_inode_lock_exclusive(source_inode);
        (void)mimfs_directory_update_parent_entry(volume, source_inode,
                                                  destination_parent_number);
        mimfs_inode_unlock_exclusive(source_inode);

        (void)mimfs_inode_decrement_hard_link_count(source_parent_inode);
        (void)mimfs_inode_increment_hard_link_count(destination_parent_inode);
    }

    if (status == MIMFS_STATUS_SUCCESS &&
        replaced_inode_number != MIMFS_INVALID_INODE_NUMBER)
    {
        struct mimfs_inode *replaced_inode =
            mimfs_inode_at(volume, replaced_inode_number);

        mimfs_inode_lock_exclusive(replaced_inode);

        if (replaced_inode_is_directory)
        {
            /* Its name and its own "." both cease to exist. */
            (*replaced_inode).hard_link_count = 0u;

            /* And the parent loses the reference held by its "..". */
            (void)mimfs_inode_decrement_hard_link_count(destination_parent_inode);
        }
        else
        {
            (void)mimfs_inode_decrement_hard_link_count(replaced_inode);
        }

        mimfs_inode_unlock_exclusive(replaced_inode);
    }

    mimfs_unlock_both_parents(source_parent_inode, destination_parent_inode,
                              source_parent_number, destination_parent_number);

    if (status == MIMFS_STATUS_SUCCESS &&
        replaced_inode_number != MIMFS_INVALID_INODE_NUMBER)
    {
        (void)mimfs_inode_destroy_if_orphaned(volume, replaced_inode_number);
    }

    mimfs_inode_release_reference(volume, source_inode_number);
    mimfs_path_resolution_release(volume, &destination_parent_resolution);
    mimfs_path_resolution_release(volume, &source_parent_resolution);

    mimfs_reader_writer_lock_release_exclusive(&(*volume).rename_serialization_lock);

    return status == MIMFS_STATUS_SUCCESS ? MIMFS_STATUS_SUCCESS : -status;
}

/* =========================================================================
 * Metadata
 * ========================================================================= */

int mimfs_namespace_get_status(struct mimfs_volume *volume, const char *path,
                               bool follow_final_symbolic_link,
                               struct mimfs_status_information *status_out)
{
    if (status_out == NULL)
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    struct mimfs_path_resolution resolution;

    const unsigned int resolve_flags =
        follow_final_symbolic_link ? MIMFS_RESOLVE_FOLLOW_FINAL_LINK : 0u;

    const int resolve_status =
        mimfs_path_resolve(volume, path, resolve_flags, &resolution);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    struct mimfs_inode *inode = mimfs_inode_at(volume, resolution.inode_number);

    mimfs_inode_lock_shared(inode);
    mimfs_inode_capture_status(volume, resolution.inode_number, inode, status_out);
    mimfs_inode_unlock_shared(inode);

    mimfs_path_resolution_release(volume, &resolution);

    return MIMFS_STATUS_SUCCESS;
}

/*
 * Shared body of the three metadata mutators.
 *
 * Each of them resolves the same way, takes the same lock and stamps the same
 * timestamp; only the field they assign differs. Passing that difference in as a
 * small callback keeps the resolution and locking written once, which is the part
 * that would be dangerous to get subtly different between them.
 */
typedef void (*mimfs_metadata_mutator_function)(struct mimfs_inode *inode,
                                                const void *argument);

static int mimfs_apply_metadata_mutation(struct mimfs_volume *volume, const char *path,
                                         mimfs_metadata_mutator_function mutator,
                                         const void *argument)
{
    struct mimfs_path_resolution resolution;

    const int resolve_status =
        mimfs_path_resolve(volume, path, MIMFS_RESOLVE_FOLLOW_FINAL_LINK, &resolution);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    struct mimfs_inode *inode = mimfs_inode_at(volume, resolution.inode_number);

    mimfs_inode_lock_exclusive(inode);
    mutator(inode, argument);
    mimfs_inode_stamp_status_change_time(inode);
    mimfs_inode_unlock_exclusive(inode);

    mimfs_path_resolution_release(volume, &resolution);

    return MIMFS_STATUS_SUCCESS;
}

static void mimfs_assign_permission_bits(struct mimfs_inode *inode,
                                         const void *argument)
{
    const uint32_t permission_bits = *(const uint32_t *)argument;

    /* The type bits are part of the identity of the inode and are never writable. */
    (*inode).file_mode = (uint16_t)(((*inode).file_mode & MIMFS_FILE_TYPE_MASK) |
                                    (permission_bits & MIMFS_PERMISSION_MASK));
}

int mimfs_namespace_change_permissions(struct mimfs_volume *volume, const char *path,
                                       uint32_t permission_bits)
{
    return mimfs_apply_metadata_mutation(volume, path, mimfs_assign_permission_bits,
                                         &permission_bits);
}

struct mimfs_ownership_argument
{
    uint32_t owner_user_identifier;
    uint32_t owner_group_identifier;
};

static void mimfs_assign_ownership(struct mimfs_inode *inode, const void *argument)
{
    const struct mimfs_ownership_argument *ownership =
        (const struct mimfs_ownership_argument *)argument;

    (*inode).owner_user_identifier = (*ownership).owner_user_identifier;
    (*inode).owner_group_identifier = (*ownership).owner_group_identifier;
}

int mimfs_namespace_change_ownership(struct mimfs_volume *volume, const char *path,
                                     uint32_t owner_user_identifier,
                                     uint32_t owner_group_identifier)
{
    struct mimfs_ownership_argument ownership;

    ownership.owner_user_identifier = owner_user_identifier;
    ownership.owner_group_identifier = owner_group_identifier;

    return mimfs_apply_metadata_mutation(volume, path, mimfs_assign_ownership,
                                         &ownership);
}

struct mimfs_times_argument
{
    int64_t last_access_time_nanoseconds;
    int64_t last_modification_time_nanoseconds;
};

static void mimfs_assign_times(struct mimfs_inode *inode, const void *argument)
{
    const struct mimfs_times_argument *times =
        (const struct mimfs_times_argument *)argument;

    atomic_store_explicit(&(*inode).last_access_time_nanoseconds,
                          (*times).last_access_time_nanoseconds, memory_order_relaxed);

    (*inode).last_modification_time_nanoseconds =
        (*times).last_modification_time_nanoseconds;
}

int mimfs_namespace_set_times(struct mimfs_volume *volume, const char *path,
                              int64_t last_access_time_nanoseconds,
                              int64_t last_modification_time_nanoseconds)
{
    struct mimfs_times_argument times;

    times.last_access_time_nanoseconds = last_access_time_nanoseconds;
    times.last_modification_time_nanoseconds = last_modification_time_nanoseconds;

    return mimfs_apply_metadata_mutation(volume, path, mimfs_assign_times, &times);
}

int mimfs_namespace_truncate(struct mimfs_volume *volume, const char *path,
                             uint64_t new_length_in_bytes)
{
    struct mimfs_path_resolution resolution;

    const int resolve_status =
        mimfs_path_resolve(volume, path, MIMFS_RESOLVE_FOLLOW_FINAL_LINK, &resolution);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return -resolve_status;
    }

    struct mimfs_inode *inode = mimfs_inode_at(volume, resolution.inode_number);

    mimfs_inode_lock_exclusive(inode);

    int status;

    if (mimfs_inode_is_directory(inode))
    {
        /* A directory's length is decided by the entries it holds, not by a caller. */
        status = -MIMFS_STATUS_IS_A_DIRECTORY;
    }
    else if (!mimfs_inode_is_regular_file(inode))
    {
        status = -MIMFS_STATUS_INVALID_ARGUMENT;
    }
    else
    {
        status = mimfs_file_set_size(volume, inode, new_length_in_bytes);
    }

    mimfs_inode_unlock_exclusive(inode);

    mimfs_path_resolution_release(volume, &resolution);

    return status;
}

/* =========================================================================
 * The lookup behind opening a file
 * ========================================================================= */

/* Resolve an existing path for opening, without creating anything. */
static int mimfs_open_existing_target(struct mimfs_volume *volume, const char *path,
                                      unsigned int open_flags,
                                      uint64_t *inode_number_out)
{
    struct mimfs_path_resolution resolution;

    const unsigned int resolve_flags = (open_flags & MIMFS_OPEN_NO_FOLLOW) != 0u
                                           ? 0u
                                           : MIMFS_RESOLVE_FOLLOW_FINAL_LINK;

    const int resolve_status =
        mimfs_path_resolve(volume, path, resolve_flags, &resolution);

    if (resolve_status != MIMFS_STATUS_SUCCESS)
    {
        return resolve_status;
    }

    /*
     * The reference established by the resolution is handed to the caller rather than
     * released, which is what keeps the inode alive for the lifetime of the descriptor
     * about to be built from it.
     */
    *inode_number_out = resolution.inode_number;

    return MIMFS_STATUS_SUCCESS;
}

/* Resolve a path for opening with creation, creating the file when it is absent. */
static int mimfs_open_or_create_target(struct mimfs_volume *volume, const char *path,
                                       unsigned int open_flags,
                                       uint32_t creation_permission_bits,
                                       uint64_t *inode_number_out,
                                       bool *inode_was_created_out)
{
    struct mimfs_path_resolution parent_resolution;
    struct mimfs_name_reference file_name;

    int status =
        mimfs_resolve_parent_and_name(volume, path, &parent_resolution, &file_name);

    if (status != MIMFS_STATUS_SUCCESS)
    {
        return status;
    }

    const uint64_t parent_inode_number = parent_resolution.inode_number;
    struct mimfs_inode *parent_inode = mimfs_inode_at(volume, parent_inode_number);

    /*
     * Allocate the candidate inode before the parent is locked. Allocation can fail,
     * and doing it inside the parent's critical section would hold up every other
     * thread working in that directory while this one searches the inode bitmap.
     */
    const uint64_t candidate_inode_number = mimfs_inode_allocator_allocate(
        volume, parent_inode_number, MIMFS_INODE_PLACE_NEAR_PARENT);

    bool candidate_was_used = false;

    mimfs_inode_lock_exclusive(parent_inode);

    struct mimfs_directory_lookup_result lookup_result;

    const bool file_already_exists =
        mimfs_directory_lookup(volume, parent_inode, &file_name, &lookup_result) ==
        MIMFS_STATUS_SUCCESS;

    if (file_already_exists)
    {
        if ((open_flags & MIMFS_OPEN_EXCLUSIVE) != 0u)
        {
            status = MIMFS_STATUS_ALREADY_EXISTS;
        }
        else
        {
            *inode_number_out = lookup_result.inode_number;
            *inode_was_created_out = false;

            /* Pinned while the parent is still locked, so the inode cannot be destroyed
             * between being found and being attached to a descriptor. */
            mimfs_inode_acquire_reference(volume, lookup_result.inode_number);
        }
    }
    else if (candidate_inode_number == MIMFS_INVALID_INODE_NUMBER)
    {
        status = MIMFS_STATUS_NO_SPACE_LEFT;
    }
    else
    {
        struct mimfs_inode *candidate_inode =
            mimfs_inode_at(volume, candidate_inode_number);

        mimfs_inode_initialize_as_regular_file(
            candidate_inode, creation_permission_bits & MIMFS_PERMISSION_MASK, 0u, 0u);

        (*candidate_inode).hard_link_count = 1u;

        status =
            mimfs_directory_insert(volume, parent_inode, &file_name,
                                   candidate_inode_number, MIMFS_ENTRY_TYPE_REGULAR);

        if (status == MIMFS_STATUS_SUCCESS)
        {
            candidate_was_used = true;

            *inode_number_out = candidate_inode_number;
            *inode_was_created_out = true;

            mimfs_inode_acquire_reference(volume, candidate_inode_number);
        }
        else
        {
            (*candidate_inode).hard_link_count = 0u;
            (*candidate_inode).file_mode = 0u;
        }
    }

    mimfs_inode_unlock_exclusive(parent_inode);

    /* An inode allocated speculatively and not needed goes straight back. */
    if (candidate_inode_number != MIMFS_INVALID_INODE_NUMBER && !candidate_was_used)
    {
        mimfs_inode_allocator_release(volume, candidate_inode_number);
    }

    mimfs_path_resolution_release(volume, &parent_resolution);

    return status;
}

int mimfs_namespace_open_target_inode(struct mimfs_volume *volume, const char *path,
                                      unsigned int open_flags,
                                      uint32_t creation_permission_bits,
                                      uint64_t *inode_number_out,
                                      bool *inode_was_created_out)
{
    if (inode_number_out == NULL || inode_was_created_out == NULL)
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    *inode_number_out = MIMFS_INVALID_INODE_NUMBER;
    *inode_was_created_out = false;

    int status;

    if ((open_flags & MIMFS_OPEN_CREATE) != 0u)
    {
        status = mimfs_open_or_create_target(volume, path, open_flags,
                                             creation_permission_bits, inode_number_out,
                                             inode_was_created_out);

        /*
         * Creation resolves only as far as the parent, so a path naming an existing
         * directory, including the root, arrives here as an invalid argument. Retrying
         * the resolution without creation turns that into the correct answer: the
         * directory is opened when the caller is allowed to, and refused as a directory
         * when it is not.
         */
        if (status == MIMFS_STATUS_INVALID_ARGUMENT)
        {
            const int fallback_status = mimfs_open_existing_target(
                volume, path, open_flags & ~(unsigned int)MIMFS_OPEN_NO_FOLLOW,
                inode_number_out);

            if (fallback_status == MIMFS_STATUS_SUCCESS)
            {
                status = MIMFS_STATUS_SUCCESS;
            }
        }
    }
    else
    {
        status = mimfs_open_existing_target(volume, path, open_flags, inode_number_out);
    }

    if (status != MIMFS_STATUS_SUCCESS)
    {
        return status;
    }

    /* ---- reconcile what was found with what was asked for ---------------- */

    struct mimfs_inode *inode = mimfs_inode_at(volume, *inode_number_out);

    mimfs_inode_lock_shared(inode);

    const bool target_is_directory = mimfs_inode_is_directory(inode);
    const bool target_is_symbolic_link = mimfs_inode_is_symbolic_link(inode);

    mimfs_inode_unlock_shared(inode);

    const unsigned int access_mode = open_flags & MIMFS_OPEN_ACCESS_MODE_MASK;
    const bool write_access_requested = access_mode != MIMFS_OPEN_READ_ONLY;

    if ((open_flags & MIMFS_OPEN_DIRECTORY) != 0u && !target_is_directory)
    {
        status = MIMFS_STATUS_NOT_A_DIRECTORY;
    }
    else if (target_is_directory && write_access_requested)
    {
        /* A directory's contents are written by the namespace operations, never by a
         * caller's bytes. */
        status = MIMFS_STATUS_IS_A_DIRECTORY;
    }
    else if (target_is_symbolic_link && (open_flags & MIMFS_OPEN_DIRECTORY) == 0u &&
             (open_flags & MIMFS_OPEN_NO_FOLLOW) != 0u)
    {
        /*
         * The caller asked not to follow the link, so what it has in its hands is the
         * link itself, which is not something that can be read or written as a file.
         */
        status = MIMFS_STATUS_TOO_MANY_SYMBOLIC_LINKS;
    }

    if (status != MIMFS_STATUS_SUCCESS)
    {
        mimfs_inode_release_reference(volume, *inode_number_out);
        *inode_number_out = MIMFS_INVALID_INODE_NUMBER;
    }

    return status;
}
