/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_path_resolver.h
 * Purpose: Turning a textual path into an inode, safely, while other threads are
 *          modifying the tree it walks.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * Why this is the delicate part
 * ---------------------------------------------------------------------------
 * A path walk touches an unbounded number of inodes, chosen by the caller's text.
 * If it held each directory's lock while taking the next, the order in which locks
 * were acquired would be decided by that text, and two threads walking two paths
 * that cross would deadlock.
 *
 * The walk therefore holds exactly one lock at a time. The sequence for each
 * component is:
 *
 *   1. Take the shared lock of the directory reached so far.
 *   2. Look the component up, and pin the child by raising its reference count.
 *   3. Release the directory's lock.
 *   4. Drop the pin on the directory, and step into the child.
 *
 * The pin taken at step two is what makes step three safe. While the directory's
 * lock is held no entry in it can be removed, so the child certainly has a name and
 * therefore cannot be destroyed. Raising its reference count before letting go of
 * that lock extends the guarantee past the unlock: from then on, the child cannot be
 * destroyed and its inode number cannot be reused, even if every name it had is
 * removed a moment later.
 *
 * What this deliberately does not provide is a snapshot. A path may be modified
 * mid-walk, and the walk then reports what it actually found, which may be a
 * component that no longer exists. That is the same guarantee a real filesystem
 * gives, and the same one every caller of a path based interface already handles.
 *
 * ---------------------------------------------------------------------------
 * Paths are absolute
 * ---------------------------------------------------------------------------
 * A volume has no working directory, because a working directory is a property of a
 * thread rather than of a filesystem, and inventing one here would make every
 * operation depend on hidden state shared between threads. Callers that want
 * relative paths compose them themselves. Relative symbolic link targets are of
 * course resolved relative to the directory holding the link, as they must be.
 */

#ifndef MIMFS_PATH_RESOLVER_H
#define MIMFS_PATH_RESOLVER_H

#include "mimfs_layout.h"
#include "support/mimfs_name.h"

/* Resolution options, combined freely. */

/*
 * Follow a symbolic link that appears as the last component. Links in the middle of
 * a path are always followed, because a path can only continue through the thing a
 * link points at.
 */
#define MIMFS_RESOLVE_FOLLOW_FINAL_LINK 0x0001u

/*
 * Stop one component short and report the directory that would hold the final
 * component, along with the component's text.
 *
 * This is the mode every operation that creates, removes or renames a name uses:
 * such an operation has to lock the parent and then act on a name inside it, and it
 * must not require the name to exist.
 */
#define MIMFS_RESOLVE_TO_PARENT         0x0002u

/*
 * Outcome of a resolution.
 *
 * The structure carries its own path buffers because resolving a symbolic link
 * rewrites the path being walked, and doing that in the caller's string would
 * modify the caller's memory. It is large by design and is meant to live on the
 * stack of the operation that uses it, which is also why it must not be copied: the
 * name reference handed out by mimfs_path_resolution_final_name points into it.
 */
struct mimfs_path_resolution
{
    /*
     * The resolved inode: the target itself, or its parent directory when
     * MIMFS_RESOLVE_TO_PARENT was requested. A reference is held on it and the
     * caller must release it with mimfs_path_resolution_release.
     */
    uint64_t inode_number;

    /* The final component, filled in only under MIMFS_RESOLVE_TO_PARENT. */
    char final_component_storage[MIMFS_MAXIMUM_FILE_NAME_LENGTH + 1u];
    uint32_t final_component_length;

    /*
     * True when the caller's path ended in a separator, which asserts that the
     * target is a directory.
     */
    bool path_ended_with_separator;

    /* Working buffers. Private to the resolver. */
    char working_path[MIMFS_MAXIMUM_PATH_LENGTH];
    char rewritten_path[MIMFS_MAXIMUM_PATH_LENGTH];
};

/*
 * Resolve a path.
 *
 * On success a reference is held on the reported inode. On failure no reference is
 * held and nothing has to be released.
 *
 * Returns MIMFS_STATUS_SUCCESS or a positive status code.
 */
int mimfs_path_resolve(struct mimfs_volume *volume, const char *path,
                       unsigned int resolve_flags,
                       struct mimfs_path_resolution *resolution_out);

/* Release the reference held by a successful resolution. */
void mimfs_path_resolution_release(struct mimfs_volume *volume,
                                   struct mimfs_path_resolution *resolution);

/*
 * Build a name reference over the final component of a resolution performed with
 * MIMFS_RESOLVE_TO_PARENT. The reference points into the resolution, which must
 * therefore outlive it.
 *
 * Returns false when the component is not a name any directory may hold.
 */
bool mimfs_path_resolution_final_name(const struct mimfs_path_resolution *resolution,
                                      struct mimfs_name_reference *name_out);

#endif /* MIMFS_PATH_RESOLVER_H */
