/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_namespace.h
 * Purpose: The operations that change or interrogate the tree of names.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * The locking pattern every operation here follows
 * ---------------------------------------------------------------------------
 * Resolve the path to the parent directory, take the parent's exclusive lock, and
 * only then take the lock of the object being operated on. Parent before child,
 * always and everywhere.
 *
 * That single rule is what makes the whole layer deadlock free, and the reason is
 * that it agrees with the shape of the data. A directory's lock is only ever taken
 * before the lock of something it contains, and something contained never waits for
 * its container, so the wait-for graph follows the tree downwards and a tree has no
 * cycles.
 *
 * Rename is the one operation that cannot obey the rule on its own, because it holds
 * two directories at once and the pair is chosen by the caller's two paths rather
 * than by the tree. It therefore takes the volume's rename lock first, which
 * serialises it against every other rename and is what makes the acquisition order
 * of the two directories irrelevant.
 *
 * ---------------------------------------------------------------------------
 * Link count accounting
 * ---------------------------------------------------------------------------
 * Stated once, because getting it wrong is how a filesystem leaks inodes:
 *
 *   a regular file          one link per name
 *   a directory             one for its own ".", one for its name in its parent,
 *                           and one more for the ".." of each subdirectory
 *
 * So an empty directory has a count of two, and creating a subdirectory inside it
 * raises the parent's count to three. Every increment in this file sits next to the
 * entry insertion that caused it, and every decrement next to the removal.
 */

#ifndef MIMFS_NAMESPACE_H
#define MIMFS_NAMESPACE_H

#include "mimfs_layout.h"

/* ---- directories -------------------------------------------------------- */

int mimfs_namespace_create_directory(struct mimfs_volume *volume, const char *path,
                                     uint32_t permission_bits);

int mimfs_namespace_remove_directory(struct mimfs_volume *volume, const char *path);

/* ---- names ------------------------------------------------------------- */

int mimfs_namespace_create_hard_link(struct mimfs_volume *volume,
                                     const char *existing_path, const char *new_path);

int mimfs_namespace_remove_link(struct mimfs_volume *volume, const char *path);

int mimfs_namespace_create_symbolic_link(struct mimfs_volume *volume,
                                         const char *target_text,
                                         const char *link_path);

long mimfs_namespace_read_symbolic_link(struct mimfs_volume *volume, const char *path,
                                        char *destination_buffer,
                                        size_t buffer_size_in_bytes);

int mimfs_namespace_rename(struct mimfs_volume *volume, const char *old_path,
                           const char *new_path);

/* ---- metadata ---------------------------------------------------------- */

int mimfs_namespace_get_status(struct mimfs_volume *volume, const char *path,
                               bool follow_final_symbolic_link,
                               struct mimfs_status_information *status_out);

int mimfs_namespace_change_permissions(struct mimfs_volume *volume, const char *path,
                                       uint32_t permission_bits);

int mimfs_namespace_change_ownership(struct mimfs_volume *volume, const char *path,
                                     uint32_t owner_user_identifier,
                                     uint32_t owner_group_identifier);

int mimfs_namespace_set_times(struct mimfs_volume *volume, const char *path,
                              int64_t last_access_time_nanoseconds,
                              int64_t last_modification_time_nanoseconds);

int mimfs_namespace_truncate(struct mimfs_volume *volume, const char *path,
                             uint64_t new_length_in_bytes);

/* ---- the lookup behind opening a file ---------------------------------- */

/*
 * Find, or create, the inode that a call to open should attach to, and pin it.
 *
 * Creation and lookup are one operation rather than two because they have to be
 * indivisible: performing the lookup, finding nothing, and then creating would let
 * two threads opening the same path with the create flag each conclude that they are
 * the one creating it.
 *
 * On success a reference is held on *inode_number_out and the caller must release it.
 * The caller also receives whether the inode was created by this call, which is what
 * it needs to decide whether the truncate flag has any work to do.
 *
 * Returns MIMFS_STATUS_SUCCESS or a positive status code.
 */
int mimfs_namespace_open_target_inode(struct mimfs_volume *volume, const char *path,
                                      unsigned int open_flags,
                                      uint32_t creation_permission_bits,
                                      uint64_t *inode_number_out,
                                      bool *inode_was_created_out);

#endif /* MIMFS_NAMESPACE_H */
