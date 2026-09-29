/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    include/mimfs/mimfs_types.h
 * Purpose: Public vocabulary of the library: the opaque volume handle, the
 *          structures exchanged with callers, the flag words and the status
 *          codes. No internal layout is exposed here, which is what lets the
 *          format evolve without recompiling callers.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#ifndef MIMFS_TYPES_H
#define MIMFS_TYPES_H

#include <stddef.h>
#include <stdint.h>

#include "mimfs_limits.h"

/* -------------------------------------------------------------------------
 * Opaque volume handle
 * ------------------------------------------------------------------------- */

/*
 * A mounted volume. The handle is a pointer into the caller supplied memory
 * region, so the library never allocates and a volume can live in a static
 * buffer, in a shared mapping or in battery backed memory.
 */
struct mimfs_volume;

/* -------------------------------------------------------------------------
 * Format parameters
 * ------------------------------------------------------------------------- */

/*
 * Geometry requests handed to mimfs_format. Every field may be left at zero,
 * in which case the documented default from mimfs_limits.h is applied. The
 * resolved geometry is recorded in the volume and can be read back with
 * mimfs_query_volume_statistics.
 */
struct mimfs_format_parameters
{
    /* One inode is created per this many bytes of data area. */
    uint64_t bytes_per_inode_ratio;

    /* Size of the descriptor and open file description tables. */
    uint32_t maximum_open_file_count;

    /* Percentage of the data area withheld from ordinary allocation. */
    uint32_t reserved_block_percentage;

    /* Hard cap on blocks per group; zero lets the layout engine decide. */
    uint32_t blocks_per_block_group;
};

/* -------------------------------------------------------------------------
 * File mode: type bits and permission bits
 * ------------------------------------------------------------------------- */

#define MIMFS_FILE_TYPE_MASK               0170000u
#define MIMFS_FILE_TYPE_REGULAR            0100000u
#define MIMFS_FILE_TYPE_DIRECTORY          0040000u
#define MIMFS_FILE_TYPE_SYMBOLIC_LINK      0120000u

#define MIMFS_PERMISSION_MASK              0007777u

#define MIMFS_PERMISSION_OWNER_READ        0000400u
#define MIMFS_PERMISSION_OWNER_WRITE       0000200u
#define MIMFS_PERMISSION_OWNER_EXECUTE     0000100u
#define MIMFS_PERMISSION_GROUP_READ        0000040u
#define MIMFS_PERMISSION_GROUP_WRITE       0000020u
#define MIMFS_PERMISSION_GROUP_EXECUTE     0000010u
#define MIMFS_PERMISSION_OTHER_READ        0000004u
#define MIMFS_PERMISSION_OTHER_WRITE       0000002u
#define MIMFS_PERMISSION_OTHER_EXECUTE     0000001u

#define MIMFS_PERMISSION_DEFAULT_FILE      0000644u
#define MIMFS_PERMISSION_DEFAULT_DIRECTORY 0000755u

#define MIMFS_MODE_IS_REGULAR_FILE(mode)                                               \
    (((mode) & MIMFS_FILE_TYPE_MASK) == MIMFS_FILE_TYPE_REGULAR)
#define MIMFS_MODE_IS_DIRECTORY(mode)                                                  \
    (((mode) & MIMFS_FILE_TYPE_MASK) == MIMFS_FILE_TYPE_DIRECTORY)
#define MIMFS_MODE_IS_SYMBOLIC_LINK(mode)                                              \
    (((mode) & MIMFS_FILE_TYPE_MASK) == MIMFS_FILE_TYPE_SYMBOLIC_LINK)

/* -------------------------------------------------------------------------
 * Directory entry type tags
 * ------------------------------------------------------------------------- */

/*
 * Every directory entry caches the type of the inode it names, so a directory
 * listing can report types without touching the inode table at all. The tag
 * values follow the established filesystem convention.
 */
#define MIMFS_ENTRY_TYPE_UNKNOWN       0u
#define MIMFS_ENTRY_TYPE_REGULAR       1u
#define MIMFS_ENTRY_TYPE_DIRECTORY     2u
#define MIMFS_ENTRY_TYPE_SYMBOLIC_LINK 7u

/* -------------------------------------------------------------------------
 * Open flags
 * ------------------------------------------------------------------------- */

/* The two low bits hold the access mode; the rest are independent switches. */
#define MIMFS_OPEN_ACCESS_MODE_MASK    0x00000003u
#define MIMFS_OPEN_READ_ONLY           0x00000000u
#define MIMFS_OPEN_WRITE_ONLY          0x00000001u
#define MIMFS_OPEN_READ_WRITE          0x00000002u

/* Create the file when it does not exist. */
#define MIMFS_OPEN_CREATE              0x00000004u
/* With MIMFS_OPEN_CREATE, fail when the file already exists. */
#define MIMFS_OPEN_EXCLUSIVE           0x00000008u
/* Discard the existing contents of the file. */
#define MIMFS_OPEN_TRUNCATE            0x00000010u
/* Every write is placed at the current end of the file, atomically. */
#define MIMFS_OPEN_APPEND              0x00000020u
/* Fail unless the target is a directory. */
#define MIMFS_OPEN_DIRECTORY           0x00000040u
/* Do not follow a symbolic link in the final path component. */
#define MIMFS_OPEN_NO_FOLLOW           0x00000080u

/* -------------------------------------------------------------------------
 * Seek origins
 * ------------------------------------------------------------------------- */

#define MIMFS_SEEK_FROM_START          0
#define MIMFS_SEEK_FROM_CURRENT        1
#define MIMFS_SEEK_FROM_END            2

/* -------------------------------------------------------------------------
 * Metadata reported to callers
 * ------------------------------------------------------------------------- */

/*
 * Snapshot of one inode. The snapshot is taken while the inode is read locked,
 * so all fields describe one single consistent instant.
 */
struct mimfs_status_information
{
    uint32_t inode_number;
    uint32_t file_mode;
    uint32_t hard_link_count;
    uint32_t owner_user_identifier;
    uint32_t owner_group_identifier;
    uint64_t file_size_in_bytes;
    uint64_t allocated_block_count;
    uint32_t block_size_in_bytes;
    int64_t last_access_time_nanoseconds;
    int64_t last_modification_time_nanoseconds;
    int64_t last_status_change_time_nanoseconds;
    int64_t creation_time_nanoseconds;
};

/* One entry produced by a directory stream. */
struct mimfs_directory_entry_information
{
    uint32_t inode_number;
    uint32_t entry_type;
    uint32_t name_length;
    char name[MIMFS_MAXIMUM_FILE_NAME_LENGTH + 1];
};

/* Whole volume accounting, the equivalent of a filesystem statistics call. */
struct mimfs_volume_statistics
{
    uint32_t block_size_in_bytes;
    uint32_t inode_size_in_bytes;
    uint64_t total_block_count;
    uint64_t free_block_count;
    uint64_t available_block_count; /* free blocks minus the reserve */
    uint64_t total_inode_count;
    uint64_t free_inode_count;
    uint32_t block_group_count;
    uint32_t blocks_per_block_group;
    uint32_t inodes_per_block_group;
    uint32_t maximum_open_file_count;
    uint32_t open_file_count;
    uint64_t maximum_file_size_in_bytes;
    uint32_t maximum_name_length;
};

/* -------------------------------------------------------------------------
 * Status codes
 * ------------------------------------------------------------------------- */

/*
 * Every entry point reports success as a non negative value and failure as the
 * negation of one of the codes below. The numeric values deliberately mirror
 * the standard error numbers, so a caller that already speaks errno can pass
 * the negated result straight to its own reporting layer.
 */
#define MIMFS_STATUS_SUCCESS                 0
#define MIMFS_STATUS_NOT_PERMITTED           1   /* EPERM */
#define MIMFS_STATUS_NO_SUCH_ENTRY           2   /* ENOENT */
#define MIMFS_STATUS_INTERRUPTED             4   /* EINTR */
#define MIMFS_STATUS_INPUT_OUTPUT_ERROR      5   /* EIO */
#define MIMFS_STATUS_BAD_DESCRIPTOR          9   /* EBADF */
#define MIMFS_STATUS_WOULD_BLOCK             11  /* EAGAIN */
#define MIMFS_STATUS_OUT_OF_MEMORY           12  /* ENOMEM */
#define MIMFS_STATUS_ACCESS_DENIED           13  /* EACCES */
#define MIMFS_STATUS_DEVICE_BUSY             16  /* EBUSY */
#define MIMFS_STATUS_ALREADY_EXISTS          17  /* EEXIST */
#define MIMFS_STATUS_CROSS_DEVICE_LINK       18  /* EXDEV */
#define MIMFS_STATUS_NOT_A_DIRECTORY         20  /* ENOTDIR */
#define MIMFS_STATUS_IS_A_DIRECTORY          21  /* EISDIR */
#define MIMFS_STATUS_INVALID_ARGUMENT        22  /* EINVAL */
#define MIMFS_STATUS_TOO_MANY_OPEN_FILES     24  /* EMFILE */
#define MIMFS_STATUS_FILE_TOO_LARGE          27  /* EFBIG */
#define MIMFS_STATUS_NO_SPACE_LEFT           28  /* ENOSPC */
#define MIMFS_STATUS_ILLEGAL_SEEK            29  /* ESPIPE */
#define MIMFS_STATUS_READ_ONLY_VOLUME        30  /* EROFS */
#define MIMFS_STATUS_TOO_MANY_LINKS          31  /* EMLINK */
#define MIMFS_STATUS_RESULT_OUT_OF_RANGE     34  /* ERANGE */
#define MIMFS_STATUS_NAME_TOO_LONG           36  /* ENAMETOOLONG */
#define MIMFS_STATUS_NOT_IMPLEMENTED         38  /* ENOSYS */
#define MIMFS_STATUS_DIRECTORY_NOT_EMPTY     39  /* ENOTEMPTY */
#define MIMFS_STATUS_TOO_MANY_SYMBOLIC_LINKS 40  /* ELOOP */
#define MIMFS_STATUS_NO_INODES_LEFT          116 /* ENOSPC variant for inodes */
#define MIMFS_STATUS_CORRUPTED_VOLUME        117 /* EUCLEAN */

#endif /* MIMFS_TYPES_H */
