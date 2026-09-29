/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/api/mimfs_diagnostics.c
 * Purpose: Human readable status descriptions and the library version.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * The descriptions are written here rather than delegated to the platform's error
 * string function for two reasons. Several of the codes are specific to this
 * filesystem and have no platform equivalent, and the wording of the rest would vary
 * between platforms and locales, which makes a log written on one machine
 * uncomparable with the same log written on another.
 */

#include "mimfs/mimfs.h"

#include "core/mimfs_layout.h"

const char *mimfs_describe_status(int status)
{
    /* Accept both the negated value an entry point returned and the positive
     * constant, so that a caller never has to remember which one it holds. */
    const int status_code = status < 0 ? -status : status;

    switch (status_code)
    {
    case MIMFS_STATUS_SUCCESS:
        return "success";
    case MIMFS_STATUS_NOT_PERMITTED:
        return "operation not permitted on an object of this type";
    case MIMFS_STATUS_NO_SUCH_ENTRY:
        return "no such file or directory";
    case MIMFS_STATUS_INTERRUPTED:
        return "operation interrupted";
    case MIMFS_STATUS_INPUT_OUTPUT_ERROR:
        return "transfer error";
    case MIMFS_STATUS_BAD_DESCRIPTOR:
        return "descriptor is not open, or not open for this kind of access";
    case MIMFS_STATUS_WOULD_BLOCK:
        return "operation would block";
    case MIMFS_STATUS_OUT_OF_MEMORY:
        return "out of memory";
    case MIMFS_STATUS_ACCESS_DENIED:
        return "permission denied";
    case MIMFS_STATUS_DEVICE_BUSY:
        return "object is in use";
    case MIMFS_STATUS_ALREADY_EXISTS:
        return "a file with that name already exists";
    case MIMFS_STATUS_CROSS_DEVICE_LINK:
        return "cannot link across volumes";
    case MIMFS_STATUS_NOT_A_DIRECTORY:
        return "a path component is not a directory";
    case MIMFS_STATUS_IS_A_DIRECTORY:
        return "target is a directory";
    case MIMFS_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case MIMFS_STATUS_TOO_MANY_OPEN_FILES:
        return "the descriptor table is full";
    case MIMFS_STATUS_FILE_TOO_LARGE:
        return "file would exceed the largest addressable size";
    case MIMFS_STATUS_NO_SPACE_LEFT:
        return "no space left on the volume";
    case MIMFS_STATUS_ILLEGAL_SEEK:
        return "seek is not possible on this object";
    case MIMFS_STATUS_READ_ONLY_VOLUME:
        return "volume is mounted read only";
    case MIMFS_STATUS_TOO_MANY_LINKS:
        return "too many hard links to one file";
    case MIMFS_STATUS_RESULT_OUT_OF_RANGE:
        return "result out of range";
    case MIMFS_STATUS_NAME_TOO_LONG:
        return "name or path is too long";
    case MIMFS_STATUS_NOT_IMPLEMENTED:
        return "operation is not implemented";
    case MIMFS_STATUS_DIRECTORY_NOT_EMPTY:
        return "directory is not empty";
    case MIMFS_STATUS_TOO_MANY_SYMBOLIC_LINKS:
        return "too many levels of symbolic links";
    case MIMFS_STATUS_NO_INODES_LEFT:
        return "no inodes left on the volume";
    case MIMFS_STATUS_CORRUPTED_VOLUME:
        return "volume structure is not consistent";
    default:
        return "unrecognised status code";
    }
}

const char *mimfs_library_version_string(void)
{
    return "1.0.0";
}

uint32_t mimfs_library_version_number(void)
{
    /*
     * One comparable integer, eight bits per component, so that a caller can write a
     * single inequality against a minimum version instead of parsing the string.
     */
    return (MIMFS_LIBRARY_VERSION_MAJOR << 16) | (MIMFS_LIBRARY_VERSION_MINOR << 8) |
           MIMFS_LIBRARY_VERSION_PATCH;
}
