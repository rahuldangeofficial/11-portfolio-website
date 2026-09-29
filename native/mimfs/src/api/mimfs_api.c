/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/api/mimfs_api.c
 * Purpose: The public entry points. Each one validates its arguments, resolves the
 *          descriptor or path it was given, and delegates to exactly one core
 *          module.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * Nothing in this file implements filesystem behaviour. Its whole job is to be the
 * boundary: past this file, arguments are known to be well formed, the volume is
 * known to be mounted, and a descriptor is known to be open. That separation is what
 * lets every core module state its preconditions and rely on them instead of
 * re-checking them.
 */

#include "mimfs/mimfs.h"

#include "core/mimfs_directory.h"
#include "core/mimfs_file_io.h"
#include "core/mimfs_file_table.h"
#include "core/mimfs_inode.h"
#include "core/mimfs_layout.h"
#include "core/mimfs_namespace.h"
#include "core/mimfs_volume.h"

/* =========================================================================
 * Shared entry point checks
 * ========================================================================= */

/*
 * Resolve a descriptor all the way to the inode behind it, validating every step.
 *
 * The four checks here are the ones every descriptor based operation needs, and
 * performing them in one place is what keeps them from drifting apart between the
 * eight functions that need them.
 */
struct mimfs_descriptor_context
{
    struct mimfs_open_file_description *description;
    struct mimfs_inode *inode;
    uint64_t inode_number;
    unsigned int retained_open_flags;
};

static int
mimfs_establish_descriptor_context(struct mimfs_volume *volume, int descriptor,
                                   struct mimfs_descriptor_context *context_out)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume)))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    struct mimfs_open_file_description *description =
        mimfs_file_table_resolve_descriptor(volume, descriptor);

    if (MIMFS_UNLIKELY(description == NULL))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    const uint64_t inode_number =
        mimfs_file_table_description_inode_number(volume, description);

    if (MIMFS_UNLIKELY(inode_number == MIMFS_INVALID_INODE_NUMBER))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    (*context_out).description = description;
    (*context_out).inode = mimfs_inode_at(volume, inode_number);
    (*context_out).inode_number = inode_number;
    (*context_out).retained_open_flags = (*description).retained_open_flags;

    return MIMFS_STATUS_SUCCESS;
}

static bool mimfs_flags_permit_reading(unsigned int retained_open_flags)
{
    const unsigned int access_mode = retained_open_flags & MIMFS_OPEN_ACCESS_MODE_MASK;

    return access_mode == MIMFS_OPEN_READ_ONLY || access_mode == MIMFS_OPEN_READ_WRITE;
}

static bool mimfs_flags_permit_writing(unsigned int retained_open_flags)
{
    const unsigned int access_mode = retained_open_flags & MIMFS_OPEN_ACCESS_MODE_MASK;

    return access_mode == MIMFS_OPEN_WRITE_ONLY || access_mode == MIMFS_OPEN_READ_WRITE;
}

/* Every flag this build understands. Anything else is a caller mistake and is
 * reported rather than ignored, because a silently dropped flag is a behaviour
 * difference that only shows up much later. */
#define MIMFS_ALL_RECOGNISED_OPEN_FLAGS                                                \
    (MIMFS_OPEN_ACCESS_MODE_MASK | MIMFS_OPEN_CREATE | MIMFS_OPEN_EXCLUSIVE |          \
     MIMFS_OPEN_TRUNCATE | MIMFS_OPEN_APPEND | MIMFS_OPEN_DIRECTORY |                  \
     MIMFS_OPEN_NO_FOLLOW)

static int mimfs_validate_open_flags(unsigned int open_flags)
{
    if ((open_flags & ~(unsigned int)MIMFS_ALL_RECOGNISED_OPEN_FLAGS) != 0u)
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    /* Three of the four bit patterns of the access mode field are meaningful; the
     * fourth is not a mode at all. */
    if ((open_flags & MIMFS_OPEN_ACCESS_MODE_MASK) == MIMFS_OPEN_ACCESS_MODE_MASK)
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    /* Exclusive creation is a modifier on creation and means nothing without it. */
    if ((open_flags & MIMFS_OPEN_EXCLUSIVE) != 0u &&
        (open_flags & MIMFS_OPEN_CREATE) == 0u)
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    /* Truncating and appending both require the ability to write. */
    if (((open_flags & MIMFS_OPEN_TRUNCATE) != 0u ||
         (open_flags & MIMFS_OPEN_APPEND) != 0u) &&
        !mimfs_flags_permit_writing(open_flags))
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    /* A directory cannot be written, so asking for both is contradictory. */
    if ((open_flags & MIMFS_OPEN_DIRECTORY) != 0u &&
        mimfs_flags_permit_writing(open_flags))
    {
        return MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Volume lifecycle
 * ========================================================================= */

struct mimfs_volume *mimfs_format(void *memory_region_base,
                                  size_t memory_region_size_in_bytes,
                                  const struct mimfs_format_parameters *parameters,
                                  int *status_out)
{
    return mimfs_volume_format(memory_region_base, memory_region_size_in_bytes,
                               parameters, status_out);
}

struct mimfs_volume *mimfs_mount(void *memory_region_base,
                                 size_t memory_region_size_in_bytes, int *status_out)
{
    return mimfs_volume_mount(memory_region_base, memory_region_size_in_bytes,
                              status_out);
}

int mimfs_unmount(struct mimfs_volume *volume)
{
    return mimfs_volume_unmount(volume);
}

int mimfs_query_volume_statistics(struct mimfs_volume *volume,
                                  struct mimfs_volume_statistics *statistics_out)
{
    return mimfs_volume_query_statistics(volume, statistics_out);
}

/* =========================================================================
 * Opening and closing
 * ========================================================================= */

int mimfs_open(struct mimfs_volume *volume, const char *path, unsigned int open_flags,
               unsigned int creation_permission_bits)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    const int flag_status = mimfs_validate_open_flags(open_flags);

    if (flag_status != MIMFS_STATUS_SUCCESS)
    {
        return -flag_status;
    }

    uint64_t inode_number = MIMFS_INVALID_INODE_NUMBER;
    bool inode_was_created = false;

    const int lookup_status = mimfs_namespace_open_target_inode(
        volume, path, open_flags, creation_permission_bits, &inode_number,
        &inode_was_created);

    if (lookup_status != MIMFS_STATUS_SUCCESS)
    {
        return -lookup_status;
    }

    /* From here on a reference is held on the inode and every exit path either hands
     * it to a descriptor or releases it. */

    if ((open_flags & MIMFS_OPEN_TRUNCATE) != 0u && !inode_was_created)
    {
        struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

        mimfs_inode_lock_exclusive(inode);

        int truncate_status = MIMFS_STATUS_SUCCESS;

        if (mimfs_inode_is_regular_file(inode))
        {
            const int set_size_status = mimfs_file_set_size(volume, inode, 0u);

            truncate_status =
                set_size_status < 0 ? -set_size_status : MIMFS_STATUS_SUCCESS;
        }

        mimfs_inode_unlock_exclusive(inode);

        if (truncate_status != MIMFS_STATUS_SUCCESS)
        {
            mimfs_inode_release_reference(volume, inode_number);
            return -truncate_status;
        }
    }

    /*
     * Only the flags that describe the open file description itself are retained. The
     * ones that describe the act of opening, creation and truncation, have already had
     * their effect and would be misleading if kept.
     */
    const unsigned int retained_open_flags =
        open_flags & (unsigned int)(MIMFS_OPEN_ACCESS_MODE_MASK | MIMFS_OPEN_APPEND |
                                    MIMFS_OPEN_DIRECTORY);

    const int descriptor =
        mimfs_file_table_open_descriptor(volume, inode_number, retained_open_flags, 0u);

    if (descriptor < 0)
    {
        mimfs_inode_release_reference(volume, inode_number);
        return descriptor;
    }

    return descriptor;
}

int mimfs_close(struct mimfs_volume *volume, int descriptor)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume)))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_file_table_close_descriptor(volume, descriptor);
}

int mimfs_duplicate_descriptor(struct mimfs_volume *volume, int descriptor)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume)))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_file_table_duplicate_descriptor(volume, descriptor);
}

/* =========================================================================
 * Reading and writing
 * ========================================================================= */

long mimfs_read(struct mimfs_volume *volume, int descriptor, void *buffer,
                size_t byte_count)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    if (MIMFS_UNLIKELY(buffer == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (MIMFS_UNLIKELY(!mimfs_flags_permit_reading(context.retained_open_flags)))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    if (MIMFS_UNLIKELY((context.retained_open_flags & MIMFS_OPEN_DIRECTORY) != 0u))
    {
        /* A directory is read through the directory stream interface, which reports
         * entries rather than bytes. */
        return -MIMFS_STATUS_IS_A_DIRECTORY;
    }

    if (byte_count == 0u)
    {
        return 0;
    }

    /*
     * Claim the whole requested range from the shared offset up front, then give back
     * whatever the transfer did not use. Claiming first is what makes two threads
     * reading through one descriptor receive two disjoint ranges instead of racing to
     * read the same one.
     */
    const uint64_t claimed_offset =
        mimfs_file_table_claim_offset(context.description, (uint64_t)byte_count);

    mimfs_inode_lock_shared(context.inode);

    const long transfer_result = mimfs_file_read_at_offset(
        volume, context.inode, claimed_offset, buffer, byte_count);

    mimfs_inode_unlock_shared(context.inode);

    if (transfer_result < 0)
    {
        mimfs_file_table_rewind_offset(context.description, (uint64_t)byte_count);
        return transfer_result;
    }

    mimfs_file_table_rewind_offset(context.description,
                                   (uint64_t)byte_count - (uint64_t)transfer_result);

    return transfer_result;
}

long mimfs_write(struct mimfs_volume *volume, int descriptor, const void *buffer,
                 size_t byte_count)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    if (MIMFS_UNLIKELY(buffer == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (MIMFS_UNLIKELY(!mimfs_flags_permit_writing(context.retained_open_flags)))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    if (byte_count == 0u)
    {
        return 0;
    }

    /*
     * An appending write places its bytes at whatever the end of the file is at the
     * moment it runs, and it decides that while holding the inode exclusively. That is
     * what makes concurrent appends from many threads interleave whole writes rather
     * than overwrite each other, without any caller side locking.
     */
    if ((context.retained_open_flags & MIMFS_OPEN_APPEND) != 0u)
    {
        mimfs_inode_lock_exclusive(context.inode);

        const uint64_t end_of_file_offset = (*context.inode).file_size_in_bytes;

        const long transfer_result = mimfs_file_write_at_offset(
            volume, context.inode, end_of_file_offset, buffer, byte_count);

        mimfs_inode_unlock_exclusive(context.inode);

        if (transfer_result > 0)
        {
            (void)mimfs_file_table_write_offset(
                context.description, end_of_file_offset + (uint64_t)transfer_result);
        }

        return transfer_result;
    }

    const uint64_t claimed_offset =
        mimfs_file_table_claim_offset(context.description, (uint64_t)byte_count);

    mimfs_inode_lock_exclusive(context.inode);

    const long transfer_result = mimfs_file_write_at_offset(
        volume, context.inode, claimed_offset, buffer, byte_count);

    mimfs_inode_unlock_exclusive(context.inode);

    if (transfer_result < 0)
    {
        mimfs_file_table_rewind_offset(context.description, (uint64_t)byte_count);
        return transfer_result;
    }

    mimfs_file_table_rewind_offset(context.description,
                                   (uint64_t)byte_count - (uint64_t)transfer_result);

    return transfer_result;
}

long mimfs_read_at_offset(struct mimfs_volume *volume, int descriptor, void *buffer,
                          size_t byte_count, uint64_t file_offset)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    if (MIMFS_UNLIKELY(buffer == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (MIMFS_UNLIKELY(!mimfs_flags_permit_reading(context.retained_open_flags)))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    if (MIMFS_UNLIKELY((context.retained_open_flags & MIMFS_OPEN_DIRECTORY) != 0u))
    {
        return -MIMFS_STATUS_IS_A_DIRECTORY;
    }

    mimfs_inode_lock_shared(context.inode);

    const long transfer_result = mimfs_file_read_at_offset(
        volume, context.inode, file_offset, buffer, byte_count);

    mimfs_inode_unlock_shared(context.inode);

    return transfer_result;
}

long mimfs_write_at_offset(struct mimfs_volume *volume, int descriptor,
                           const void *buffer, size_t byte_count, uint64_t file_offset)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    if (MIMFS_UNLIKELY(buffer == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (MIMFS_UNLIKELY(!mimfs_flags_permit_writing(context.retained_open_flags)))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    mimfs_inode_lock_exclusive(context.inode);

    const long transfer_result = mimfs_file_write_at_offset(
        volume, context.inode, file_offset, buffer, byte_count);

    mimfs_inode_unlock_exclusive(context.inode);

    return transfer_result;
}

long long mimfs_seek(struct mimfs_volume *volume, int descriptor, long long offset,
                     int seek_origin)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    long long resulting_offset;

    switch (seek_origin)
    {
    case MIMFS_SEEK_FROM_START:
        resulting_offset = offset;
        break;

    case MIMFS_SEEK_FROM_CURRENT:
        resulting_offset =
            (long long)mimfs_file_table_read_offset(context.description) + offset;
        break;

    case MIMFS_SEEK_FROM_END:
    {
        mimfs_inode_lock_shared(context.inode);
        const uint64_t file_size = (*context.inode).file_size_in_bytes;
        mimfs_inode_unlock_shared(context.inode);

        resulting_offset = (long long)file_size + offset;
        break;
    }

    default:
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    /* A negative offset names no byte of any file. Seeking past the end is legal and
     * creates a hole once something is written there. */
    if (resulting_offset < 0)
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if ((uint64_t)resulting_offset > MIMFS_MAXIMUM_FILE_SIZE_BYTES)
    {
        return -MIMFS_STATUS_FILE_TOO_LARGE;
    }

    (void)mimfs_file_table_write_offset(context.description,
                                        (uint64_t)resulting_offset);

    return resulting_offset;
}

int mimfs_truncate_by_descriptor(struct mimfs_volume *volume, int descriptor,
                                 uint64_t new_length_in_bytes)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    if (MIMFS_UNLIKELY(!mimfs_flags_permit_writing(context.retained_open_flags)))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    mimfs_inode_lock_exclusive(context.inode);

    int status;

    if (!mimfs_inode_is_regular_file(context.inode))
    {
        status = -MIMFS_STATUS_INVALID_ARGUMENT;
    }
    else
    {
        status = mimfs_file_set_size(volume, context.inode, new_length_in_bytes);
    }

    mimfs_inode_unlock_exclusive(context.inode);

    return status;
}

int mimfs_truncate_by_path(struct mimfs_volume *volume, const char *path,
                           uint64_t new_length_in_bytes)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_truncate(volume, path, new_length_in_bytes);
}

int mimfs_synchronize(struct mimfs_volume *volume, int descriptor)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    /*
     * The backing store is memory that every thread already shares, so there is no
     * device to flush. What a caller can still legitimately want is the guarantee that
     * everything it wrote is visible to any thread that synchronises with it
     * afterwards, and that is what the fence provides.
     */
    mimfs_publish_prior_stores();

    return MIMFS_STATUS_SUCCESS;
}

/* =========================================================================
 * Namespace
 * ========================================================================= */

int mimfs_create_directory(struct mimfs_volume *volume, const char *path,
                           unsigned int permission_bits)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_create_directory(volume, path, permission_bits);
}

int mimfs_remove_directory(struct mimfs_volume *volume, const char *path)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_remove_directory(volume, path);
}

int mimfs_create_hard_link(struct mimfs_volume *volume, const char *existing_path,
                           const char *new_path)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || existing_path == NULL ||
                       new_path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_create_hard_link(volume, existing_path, new_path);
}

int mimfs_remove_link(struct mimfs_volume *volume, const char *path)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_remove_link(volume, path);
}

int mimfs_create_symbolic_link(struct mimfs_volume *volume, const char *target_text,
                               const char *link_path)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || target_text == NULL ||
                       link_path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_create_symbolic_link(volume, target_text, link_path);
}

long mimfs_read_symbolic_link(struct mimfs_volume *volume, const char *path,
                              char *buffer, size_t buffer_size_in_bytes)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_read_symbolic_link(volume, path, buffer,
                                              buffer_size_in_bytes);
}

int mimfs_rename(struct mimfs_volume *volume, const char *old_path,
                 const char *new_path)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || old_path == NULL ||
                       new_path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_rename(volume, old_path, new_path);
}

/* =========================================================================
 * Metadata
 * ========================================================================= */

int mimfs_get_status(struct mimfs_volume *volume, const char *path,
                     struct mimfs_status_information *status_out)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_get_status(volume, path, true, status_out);
}

int mimfs_get_link_status(struct mimfs_volume *volume, const char *path,
                          struct mimfs_status_information *status_out)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_get_status(volume, path, false, status_out);
}

int mimfs_get_status_by_descriptor(struct mimfs_volume *volume, int descriptor,
                                   struct mimfs_status_information *status_out)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    if (MIMFS_UNLIKELY(status_out == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    mimfs_inode_lock_shared(context.inode);
    mimfs_inode_capture_status(volume, context.inode_number, context.inode, status_out);
    mimfs_inode_unlock_shared(context.inode);

    return MIMFS_STATUS_SUCCESS;
}

int mimfs_change_permissions(struct mimfs_volume *volume, const char *path,
                             unsigned int permission_bits)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_change_permissions(volume, path, permission_bits);
}

int mimfs_change_ownership(struct mimfs_volume *volume, const char *path,
                           uint32_t owner_user_identifier,
                           uint32_t owner_group_identifier)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_change_ownership(volume, path, owner_user_identifier,
                                            owner_group_identifier);
}

int mimfs_set_times(struct mimfs_volume *volume, const char *path,
                    int64_t last_access_time_nanoseconds,
                    int64_t last_modification_time_nanoseconds)
{
    if (MIMFS_UNLIKELY(!mimfs_volume_is_usable(volume) || path == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    return mimfs_namespace_set_times(volume, path, last_access_time_nanoseconds,
                                     last_modification_time_nanoseconds);
}

/* =========================================================================
 * Directory streams
 * ========================================================================= */

int mimfs_open_directory(struct mimfs_volume *volume, const char *path)
{
    return mimfs_open(volume, path, MIMFS_OPEN_READ_ONLY | MIMFS_OPEN_DIRECTORY, 0u);
}

int mimfs_read_directory(struct mimfs_volume *volume, int descriptor,
                         struct mimfs_directory_entry_information *entry_out)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    if (MIMFS_UNLIKELY(entry_out == NULL))
    {
        return -MIMFS_STATUS_INVALID_ARGUMENT;
    }

    if (MIMFS_UNLIKELY((context.retained_open_flags & MIMFS_OPEN_DIRECTORY) == 0u))
    {
        return -MIMFS_STATUS_NOT_A_DIRECTORY;
    }

    mimfs_inode_lock_shared(context.inode);

    int result;

    if (!mimfs_inode_is_directory(context.inode))
    {
        result = -MIMFS_STATUS_NOT_A_DIRECTORY;
    }
    else
    {
        /*
         * The cursor is read, advanced past whatever was produced, and written back
         * without releasing the offset lock in between. That is what lets several
         * threads share one directory stream and each receive a different entry, rather
         * than all of them receiving the entry at whatever the cursor happened to be.
         *
         * The order of the two locks, inode then offset, is the order stated for the
         * whole filesystem, and nothing inside the region can block.
         */
        mimfs_file_table_offset_lock_acquire(context.description);

        uint64_t entry_cursor =
            mimfs_file_table_read_offset_locked(context.description);

        result = mimfs_directory_read_next_entry(volume, context.inode, &entry_cursor,
                                                 entry_out);

        mimfs_file_table_write_offset_locked(context.description, entry_cursor);

        mimfs_file_table_offset_lock_release(context.description);
    }

    mimfs_inode_unlock_shared(context.inode);

    return result;
}

int mimfs_rewind_directory(struct mimfs_volume *volume, int descriptor)
{
    struct mimfs_descriptor_context context;

    const int context_status =
        mimfs_establish_descriptor_context(volume, descriptor, &context);

    if (context_status != MIMFS_STATUS_SUCCESS)
    {
        return context_status;
    }

    if (MIMFS_UNLIKELY((context.retained_open_flags & MIMFS_OPEN_DIRECTORY) == 0u))
    {
        return -MIMFS_STATUS_NOT_A_DIRECTORY;
    }

    (void)mimfs_file_table_write_offset(context.description, 0u);

    return MIMFS_STATUS_SUCCESS;
}

int mimfs_close_directory(struct mimfs_volume *volume, int descriptor)
{
    return mimfs_close(volume, descriptor);
}
