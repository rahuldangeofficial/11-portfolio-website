/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    include/mimfs/mimfs.h
 * Purpose: The complete public interface. This is the only header a caller has
 *          to include.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * Concurrency contract
 * ---------------------------------------------------------------------------
 * Every entry point below is safe to call from any number of threads at the
 * same time, on the same volume, without any external serialisation. The
 * guarantees a caller may rely on are:
 *
 *   1. Metadata operations are atomic. A concurrent observer sees a directory
 *      entry either fully present or fully absent, never half written.
 *   2. A read or a write through one descriptor consumes the shared file
 *      offset atomically, so two threads sharing a descriptor never read the
 *      same bytes twice or skip a range.
 *   3. A write performed with MIMFS_OPEN_APPEND places its bytes at the end of
 *      the file atomically with respect to every other append.
 *   4. Reads and writes of a single file are serialised against each other per
 *      inode, so a reader never observes a partially applied write.
 *
 * The following are deliberately not guaranteed, matching the behaviour of
 * general purpose filesystems:
 *
 *   * A read that races a write to a different region of the same file may be
 *     ordered either way.
 *   * Operations are not transactional across more than one path. A rename is
 *     atomic; a rename plus an unlink is not.
 *
 * ---------------------------------------------------------------------------
 * Error reporting
 * ---------------------------------------------------------------------------
 * Functions returning int, long or long long report success with a non
 * negative value and failure with a negated MIMFS_STATUS_* code. Functions
 * returning a pointer report failure with NULL and write the status code to
 * the status_out argument when one was supplied.
 */

#ifndef MIMFS_H
#define MIMFS_H

#include "mimfs_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================================================
     * Volume lifecycle
     * ========================================================================= */

    /*
     * Create a brand new empty volume inside the caller supplied memory region and
     * mount it. Any previous content of the region is destroyed.
     *
     * The region is used exactly as given: the library performs no dynamic
     * allocation of its own, neither at format time nor afterwards. The returned
     * handle points inside the region, therefore the region must stay mapped, and
     * must not be moved, for as long as the volume is mounted.
     *
     * memory_region_base      first byte of the region; may be unaligned, the
     *                         layout engine aligns internally and reports the
     *                         bytes it had to skip through the statistics call
     * memory_region_size      number of usable bytes at memory_region_base
     * parameters              geometry request, or NULL for the defaults
     * status_out              receives a MIMFS_STATUS_* code, or NULL
     *
     * Returns the mounted volume, or NULL when the region is too small to hold a
     * minimal volume or the parameters are self contradictory.
     */
    struct mimfs_volume *mimfs_format(void *memory_region_base,
                                      size_t memory_region_size_in_bytes,
                                      const struct mimfs_format_parameters *parameters,
                                      int *status_out);

    /*
     * Mount a volume that was formatted earlier in the same memory region. The
     * persistent state, meaning the namespace and the file contents, is preserved.
     * The volatile state, meaning every lock, every reference count and the whole
     * descriptor table, is rebuilt from scratch, so a volume left behind by a
     * crashed process mounts cleanly and no stale descriptor survives.
     *
     * Returns the mounted volume, or NULL when the region does not hold a volume
     * this build of the library recognises.
     */
    struct mimfs_volume *mimfs_mount(void *memory_region_base,
                                     size_t memory_region_size_in_bytes,
                                     int *status_out);

    /*
     * Release every descriptor still open and detach the handle. The content of
     * the memory region stays valid and can be mounted again later.
     *
     * The caller is responsible for making sure no other thread is inside the
     * library when this is called; that is the one and only operation on a volume
     * that is not itself concurrency safe, exactly as unmounting a real volume is
     * not safe while it is in use.
     */
    int mimfs_unmount(struct mimfs_volume *volume);

    /* Fill in whole volume accounting. */
    int mimfs_query_volume_statistics(struct mimfs_volume *volume,
                                      struct mimfs_volume_statistics *statistics_out);

    /* =========================================================================
     * File descriptors
     * ========================================================================= */

    /*
     * Open, and optionally create, the file named by path.
     *
     * open_flags                  one access mode from MIMFS_OPEN_READ_ONLY,
     *                             MIMFS_OPEN_WRITE_ONLY or MIMFS_OPEN_READ_WRITE,
     *                             combined with any of the independent switches
     * creation_permission_bits    permissions applied when the file is created;
     *                             ignored otherwise
     *
     * Returns a non negative descriptor, or a negated status code.
     */
    int mimfs_open(struct mimfs_volume *volume, const char *path,
                   unsigned int open_flags, unsigned int creation_permission_bits);

    /*
     * Drop one descriptor. The underlying open file description, and with it the
     * shared file offset, survives while any duplicate of the descriptor remains.
     * An inode whose last link was removed while it was open is destroyed here,
     * when its last descriptor goes away.
     */
    int mimfs_close(struct mimfs_volume *volume, int descriptor);

    /*
     * Duplicate a descriptor. Both descriptors then share one file offset and one
     * set of access flags, so a seek through either is observed through both.
     */
    int mimfs_duplicate_descriptor(struct mimfs_volume *volume, int descriptor);

    /*
     * Read up to byte_count bytes starting at the shared file offset, and advance
     * that offset by the number of bytes transferred.
     *
     * Returns the number of bytes read, which is zero at end of file and may be
     * short of byte_count only at end of file, or a negated status code.
     */
    long mimfs_read(struct mimfs_volume *volume, int descriptor, void *buffer,
                    size_t byte_count);

    /*
     * Write byte_count bytes starting at the shared file offset, and advance that
     * offset by the number of bytes transferred. When the description carries
     * MIMFS_OPEN_APPEND the bytes go to the end of the file instead, atomically
     * with respect to every other append to the same inode.
     *
     * Returns the number of bytes written, which is short of byte_count only when
     * the volume ran out of space part way, or a negated status code.
     */
    long mimfs_write(struct mimfs_volume *volume, int descriptor, const void *buffer,
                     size_t byte_count);

    /*
     * Positioned transfers. These do not consult and do not modify the shared file
     * offset, which makes them the right primitive for a pool of threads working
     * on disjoint regions of one file through one descriptor.
     */
    long mimfs_read_at_offset(struct mimfs_volume *volume, int descriptor, void *buffer,
                              size_t byte_count, uint64_t file_offset);
    long mimfs_write_at_offset(struct mimfs_volume *volume, int descriptor,
                               const void *buffer, size_t byte_count,
                               uint64_t file_offset);

    /*
     * Reposition the shared file offset. Seeking past the end of the file is
     * legal; the gap becomes a hole that reads back as zeroes and consumes no
     * blocks until it is written.
     *
     * Returns the resulting absolute offset, or a negated status code.
     */
    long long mimfs_seek(struct mimfs_volume *volume, int descriptor, long long offset,
                         int seek_origin);

    /*
     * Set the length of a file, either growing it with a hole or discarding the
     * bytes beyond the new length.
     */
    int mimfs_truncate_by_path(struct mimfs_volume *volume, const char *path,
                               uint64_t new_length_in_bytes);
    int mimfs_truncate_by_descriptor(struct mimfs_volume *volume, int descriptor,
                                     uint64_t new_length_in_bytes);

    /*
     * Flush pending changes of one file. The backing store of this filesystem is
     * memory that is already coherent across threads, so the only work this has to
     * do is publish every prior store to any thread that later observes the file;
     * it is provided so that portable code can call it unconditionally.
     */
    int mimfs_synchronize(struct mimfs_volume *volume, int descriptor);

    /* =========================================================================
     * Namespace
     * ========================================================================= */

    int mimfs_create_directory(struct mimfs_volume *volume, const char *path,
                               unsigned int permission_bits);
    int mimfs_remove_directory(struct mimfs_volume *volume, const char *path);

    /* Attach a second name to an existing regular file or symbolic link. */
    int mimfs_create_hard_link(struct mimfs_volume *volume, const char *existing_path,
                               const char *new_path);

    /*
     * Detach one name. The inode itself is destroyed once its last name is gone
     * and its last descriptor is closed, whichever happens later.
     */
    int mimfs_remove_link(struct mimfs_volume *volume, const char *path);

    int mimfs_create_symbolic_link(struct mimfs_volume *volume, const char *target_text,
                                   const char *link_path);

    /*
     * Copy the target text of a symbolic link into buffer. The result is not NUL
     * terminated when it exactly fills the buffer, matching the established
     * behaviour of this operation.
     *
     * Returns the number of bytes copied, or a negated status code.
     */
    long mimfs_read_symbolic_link(struct mimfs_volume *volume, const char *path,
                                  char *buffer, size_t buffer_size_in_bytes);

    /*
     * Move a name, atomically with respect to every other namespace operation on
     * the volume. When the destination exists it is replaced, provided the two
     * ends agree on being a directory or not being one.
     */
    int mimfs_rename(struct mimfs_volume *volume, const char *old_path,
                     const char *new_path);

    /* =========================================================================
     * Metadata
     * ========================================================================= */

    /* Follows a symbolic link in the final component. */
    int mimfs_get_status(struct mimfs_volume *volume, const char *path,
                         struct mimfs_status_information *status_out);

    /* Reports on the link itself rather than on its target. */
    int mimfs_get_link_status(struct mimfs_volume *volume, const char *path,
                              struct mimfs_status_information *status_out);

    int mimfs_get_status_by_descriptor(struct mimfs_volume *volume, int descriptor,
                                       struct mimfs_status_information *status_out);

    int mimfs_change_permissions(struct mimfs_volume *volume, const char *path,
                                 unsigned int permission_bits);
    int mimfs_change_ownership(struct mimfs_volume *volume, const char *path,
                               uint32_t owner_user_identifier,
                               uint32_t owner_group_identifier);
    int mimfs_set_times(struct mimfs_volume *volume, const char *path,
                        int64_t last_access_time_nanoseconds,
                        int64_t last_modification_time_nanoseconds);

    /* =========================================================================
     * Directory streams
     * ========================================================================= */

    /*
     * Open a directory for reading. The returned descriptor accepts only the
     * directory stream calls below; ordinary reads and writes on it are refused.
     */
    int mimfs_open_directory(struct mimfs_volume *volume, const char *path);

    /*
     * Fetch the next entry, including the "." and ".." entries.
     *
     * Returns 1 when an entry was produced, 0 at the end of the directory, or a
     * negated status code.
     */
    int mimfs_read_directory(struct mimfs_volume *volume, int descriptor,
                             struct mimfs_directory_entry_information *entry_out);

    /* Restart the stream from the first entry. */
    int mimfs_rewind_directory(struct mimfs_volume *volume, int descriptor);

    /* Alias of mimfs_close, provided for symmetry of the directory interface. */
    int mimfs_close_directory(struct mimfs_volume *volume, int descriptor);

    /* =========================================================================
     * Diagnostics
     * ========================================================================= */

    /*
     * Translate a status code, given either as the negative value returned by an
     * entry point or as the positive MIMFS_STATUS_* constant, into a stable
     * English sentence fragment. The returned string has static storage duration.
     */
    const char *mimfs_describe_status(int status);

    /* Version of the library, as a dotted string and as a comparable integer. */
    const char *mimfs_library_version_string(void);
    uint32_t mimfs_library_version_number(void);

#ifdef __cplusplus
}
#endif

#endif /* MIMFS_H */
