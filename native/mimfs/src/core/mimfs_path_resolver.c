/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_path_resolver.c
 * Purpose: Implementation of path resolution.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_path_resolver.h"

#include <string.h>

#include "mimfs_directory.h"
#include "mimfs_file_io.h"
#include "mimfs_inode.h"

#define MIMFS_PATH_SEPARATOR '/'

/* Advance past a run of separators. */
static const char *mimfs_skip_separators(const char *cursor)
{
    while (*cursor == MIMFS_PATH_SEPARATOR)
    {
        cursor++;
    }

    return cursor;
}

/* Find the end of the component starting at cursor. */
static const char *mimfs_find_component_end(const char *cursor)
{
    while (*cursor != '\0' && *cursor != MIMFS_PATH_SEPARATOR)
    {
        cursor++;
    }

    return cursor;
}

/*
 * Rewrite the path being walked so that it continues through a symbolic link's
 * target instead of through the link.
 *
 * The result is the target text followed by whatever of the original path was still
 * unconsumed. A target that begins with a separator restarts the walk from the root;
 * one that does not continues from the directory that held the link, which is why
 * the caller keeps that directory rather than stepping into the link.
 */
static int mimfs_splice_symbolic_link_target(struct mimfs_path_resolution *resolution,
                                             const char *target_text,
                                             uint32_t target_length,
                                             const char *unconsumed_remainder)
{
    const size_t remainder_length = strlen(unconsumed_remainder);

    /* One byte for the terminator, one for the separator inserted between the two
     * pieces when both are non empty. */
    if ((size_t)target_length + remainder_length + 2u > MIMFS_MAXIMUM_PATH_LENGTH)
    {
        return MIMFS_STATUS_NAME_TOO_LONG;
    }

    char *write_cursor = (*resolution).rewritten_path;

    memcpy(write_cursor, target_text, target_length);
    write_cursor += target_length;

    if (remainder_length > 0u)
    {
        *write_cursor = MIMFS_PATH_SEPARATOR;
        write_cursor++;

        memcpy(write_cursor, unconsumed_remainder, remainder_length);
        write_cursor += remainder_length;
    }

    *write_cursor = '\0';

    /*
     * The rewritten path is built in a second buffer and only then copied over the
     * one being walked, because the remainder that was just consumed is itself a
     * slice of that buffer.
     */
    memcpy((*resolution).working_path, (*resolution).rewritten_path,
           (size_t)(write_cursor - (*resolution).rewritten_path) + 1u);

    return MIMFS_STATUS_SUCCESS;
}

/*
 * Read the target of a symbolic link into a caller supplied buffer.
 *
 * Requires the shared lock on the link's inode, which the caller already holds.
 */
static int mimfs_read_link_target_text(struct mimfs_volume *volume,
                                       struct mimfs_inode *link_inode,
                                       char *target_buffer, uint32_t *target_length_out)
{
    const uint64_t target_length = (*link_inode).file_size_in_bytes;

    if (target_length == 0u || target_length >= MIMFS_MAXIMUM_PATH_LENGTH)
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    const long bytes_loaded = mimfs_file_load_symbolic_link_target(
        volume, link_inode, target_buffer, (size_t)target_length);

    if (bytes_loaded < 0 || (uint64_t)bytes_loaded != target_length)
    {
        return MIMFS_STATUS_CORRUPTED_VOLUME;
    }

    *(target_buffer + target_length) = '\0';
    *target_length_out = (uint32_t)target_length;

    return MIMFS_STATUS_SUCCESS;
}

int mimfs_path_resolve(struct mimfs_volume *volume, const char *path,
                       unsigned int resolve_flags,
                       struct mimfs_path_resolution *resolution_out)
{
    if (MIMFS_UNLIKELY(volume == NULL || path == NULL || resolution_out == NULL))
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    const size_t path_length = strlen(path);

    if (MIMFS_UNLIKELY(path_length == 0u))
    {
        return MIMFS_STATUS_NO_SUCH_ENTRY;
    }

    if (MIMFS_UNLIKELY(path_length >= MIMFS_MAXIMUM_PATH_LENGTH))
    {
        return MIMFS_STATUS_NAME_TOO_LONG;
    }

    /* Only absolute paths; a volume has no working directory to resolve against. */
    if (MIMFS_UNLIKELY(*(path + 0) != MIMFS_PATH_SEPARATOR))
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    (*resolution_out).inode_number = MIMFS_INVALID_INODE_NUMBER;
    (*resolution_out).final_component_length = 0u;
    *((*resolution_out).final_component_storage + 0) = '\0';
    (*resolution_out).path_ended_with_separator =
        *(path + (path_length - 1u)) == MIMFS_PATH_SEPARATOR && path_length > 1u;

    memcpy((*resolution_out).working_path, path, path_length + 1u);

    const bool stop_at_parent = (resolve_flags & MIMFS_RESOLVE_TO_PARENT) != 0u;
    const bool follow_final_link =
        (resolve_flags & MIMFS_RESOLVE_FOLLOW_FINAL_LINK) != 0u;

    uint64_t current_directory_inode_number = MIMFS_ROOT_INODE_NUMBER;
    mimfs_inode_acquire_reference(volume, current_directory_inode_number);

    const char *cursor = (*resolution_out).working_path;
    uint32_t symbolic_links_followed = 0u;

    for (;;)
    {
        cursor = mimfs_skip_separators(cursor);

        /* The path is exhausted: whatever was reached is the answer. */
        if (*cursor == '\0')
        {
            if (stop_at_parent)
            {
                /*
                 * There is no final component to report. The only path that produces
                 * this is one naming the root, which no create, remove or rename
                 * operation may target.
                 */
                mimfs_inode_release_reference(volume, current_directory_inode_number);
                return MIMFS_STATUS_INVALID_ARGUMENT;
            }

            (*resolution_out).inode_number = current_directory_inode_number;
            return MIMFS_STATUS_SUCCESS;
        }

        const char *component_start = cursor;
        const char *component_end = mimfs_find_component_end(component_start);
        const uint32_t component_length = (uint32_t)(component_end - component_start);

        if (MIMFS_UNLIKELY(component_length > MIMFS_MAXIMUM_FILE_NAME_LENGTH))
        {
            mimfs_inode_release_reference(volume, current_directory_inode_number);
            return MIMFS_STATUS_NAME_TOO_LONG;
        }

        /* Whatever follows the component, once separators are skipped. */
        const char *remainder = mimfs_skip_separators(component_end);
        const bool component_is_final = *remainder == '\0';

        struct mimfs_name_reference component_name;

        if (!mimfs_name_reference_initialize(&component_name, component_start,
                                             component_length))
        {
            mimfs_inode_release_reference(volume, current_directory_inode_number);
            return MIMFS_STATUS_INVALID_ARGUMENT;
        }

        if (component_is_final && stop_at_parent)
        {
            memcpy((*resolution_out).final_component_storage, component_start,
                   component_length);
            *((*resolution_out).final_component_storage + component_length) = '\0';
            (*resolution_out).final_component_length = component_length;
            (*resolution_out).inode_number = current_directory_inode_number;

            return MIMFS_STATUS_SUCCESS;
        }

        /* ---- resolve one component ------------------------------------- */

        struct mimfs_inode *current_directory_inode =
            mimfs_inode_at(volume, current_directory_inode_number);

        mimfs_inode_lock_shared(current_directory_inode);

        if (!mimfs_inode_is_directory(current_directory_inode))
        {
            mimfs_inode_unlock_shared(current_directory_inode);
            mimfs_inode_release_reference(volume, current_directory_inode_number);
            return MIMFS_STATUS_NOT_A_DIRECTORY;
        }

        struct mimfs_directory_lookup_result lookup_result;

        const int lookup_status = mimfs_directory_lookup(
            volume, current_directory_inode, &component_name, &lookup_result);

        if (lookup_status != MIMFS_STATUS_SUCCESS)
        {
            mimfs_inode_unlock_shared(current_directory_inode);
            mimfs_inode_release_reference(volume, current_directory_inode_number);
            return MIMFS_STATUS_NO_SUCH_ENTRY;
        }

        const uint64_t child_inode_number = lookup_result.inode_number;

        if (MIMFS_UNLIKELY(!mimfs_inode_number_is_in_range(volume, child_inode_number)))
        {
            mimfs_inode_unlock_shared(current_directory_inode);
            mimfs_inode_release_reference(volume, current_directory_inode_number);
            return MIMFS_STATUS_CORRUPTED_VOLUME;
        }

        /*
         * Pin the child before letting go of the directory. This is the step that
         * makes single-lock walking safe; see the commentary in the header.
         */
        mimfs_inode_acquire_reference(volume, child_inode_number);

        mimfs_inode_unlock_shared(current_directory_inode);

        /* ---- decide whether the child is a link to follow -------------- */

        struct mimfs_inode *child_inode = mimfs_inode_at(volume, child_inode_number);

        mimfs_inode_lock_shared(child_inode);

        const bool child_is_symbolic_link = mimfs_inode_is_symbolic_link(child_inode);
        const bool must_follow_this_link =
            child_is_symbolic_link && (!component_is_final || follow_final_link);

        if (must_follow_this_link)
        {
            if (symbolic_links_followed >= MIMFS_MAXIMUM_SYMBOLIC_LINK_RESOLUTION_DEPTH)
            {
                mimfs_inode_unlock_shared(child_inode);
                mimfs_inode_release_reference(volume, child_inode_number);
                mimfs_inode_release_reference(volume, current_directory_inode_number);
                return MIMFS_STATUS_TOO_MANY_SYMBOLIC_LINKS;
            }

            symbolic_links_followed++;

            char link_target_text[MIMFS_MAXIMUM_PATH_LENGTH];
            uint32_t link_target_length = 0u;

            const int read_status = mimfs_read_link_target_text(
                volume, child_inode, link_target_text, &link_target_length);

            mimfs_inode_unlock_shared(child_inode);
            mimfs_inode_release_reference(volume, child_inode_number);

            if (read_status != MIMFS_STATUS_SUCCESS)
            {
                mimfs_inode_release_reference(volume, current_directory_inode_number);
                return read_status;
            }

            const int splice_status = mimfs_splice_symbolic_link_target(
                resolution_out, link_target_text, link_target_length, remainder);

            if (splice_status != MIMFS_STATUS_SUCCESS)
            {
                mimfs_inode_release_reference(volume, current_directory_inode_number);
                return splice_status;
            }

            /*
             * An absolute target abandons everything walked so far. A relative one
             * continues from the directory that held the link, which is exactly the
             * directory still pinned in current_directory_inode_number.
             */
            if (*((*resolution_out).working_path + 0) == MIMFS_PATH_SEPARATOR)
            {
                mimfs_inode_release_reference(volume, current_directory_inode_number);
                current_directory_inode_number = MIMFS_ROOT_INODE_NUMBER;
                mimfs_inode_acquire_reference(volume, current_directory_inode_number);
            }

            cursor = (*resolution_out).working_path;
            continue;
        }

        /* Not a link to follow: step into the child. */
        const bool child_is_directory = mimfs_inode_is_directory(child_inode);

        mimfs_inode_unlock_shared(child_inode);

        /*
         * A trailing separator in the caller's path is an assertion that the target is
         * a directory, and it is enforced here so that every operation inherits the
         * rule without repeating it.
         */
        if (component_is_final && (*resolution_out).path_ended_with_separator &&
            !child_is_directory)
        {
            mimfs_inode_release_reference(volume, child_inode_number);
            mimfs_inode_release_reference(volume, current_directory_inode_number);
            return MIMFS_STATUS_NOT_A_DIRECTORY;
        }

        mimfs_inode_release_reference(volume, current_directory_inode_number);
        current_directory_inode_number = child_inode_number;

        if (component_is_final)
        {
            (*resolution_out).inode_number = current_directory_inode_number;
            return MIMFS_STATUS_SUCCESS;
        }

        cursor = remainder;
    }
}

void mimfs_path_resolution_release(struct mimfs_volume *volume,
                                   struct mimfs_path_resolution *resolution)
{
    if (resolution == NULL || (*resolution).inode_number == MIMFS_INVALID_INODE_NUMBER)
    {
        return;
    }

    mimfs_inode_release_reference(volume, (*resolution).inode_number);
    (*resolution).inode_number = MIMFS_INVALID_INODE_NUMBER;
}

bool mimfs_path_resolution_final_name(const struct mimfs_path_resolution *resolution,
                                      struct mimfs_name_reference *name_out)
{
    return mimfs_name_reference_initialize(name_out,
                                           (*resolution).final_component_storage,
                                           (*resolution).final_component_length);
}
