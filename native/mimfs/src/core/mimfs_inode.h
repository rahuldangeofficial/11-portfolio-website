/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_inode.h
 * Purpose: Everything that can be done to a single inode: locking it, stamping
 *          it, counting references to it, and destroying it once nothing refers
 *          to it any more.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * Lifetime rules
 * ---------------------------------------------------------------------------
 * An inode is reachable in two independent ways, and it lives until both are
 * gone:
 *
 *   hard_link_count       how many directory entries name it
 *   open_reference_count  how many descriptors and in-flight path lookups hold it
 *
 * The consequence is the behaviour every caller expects from a real filesystem:
 * removing the last name of a file that is still open does not destroy it. The
 * name disappears from the namespace immediately, the blocks stay, and both are
 * released when the last descriptor closes.
 *
 * Every function below states whether it expects the inode lock to be held, and
 * in which mode. That annotation is the interface: the lock is not recursive, so
 * calling a function that takes the lock while already holding it deadlocks.
 */

#ifndef MIMFS_INODE_H
#define MIMFS_INODE_H

#include "mimfs_layout.h"

/* =========================================================================
 * Locking
 * =========================================================================
 * Thin names over the lock inside the inode. They exist so that no other module
 * reaches into the inode structure to lock it, which is what keeps the search for
 * "where is this inode locked" answerable.
 */

MIMFS_ALWAYS_INLINE void mimfs_inode_lock_shared(struct mimfs_inode *inode)
{
    mimfs_reader_writer_lock_acquire_shared(&(*inode).content_lock);
}

MIMFS_ALWAYS_INLINE void mimfs_inode_unlock_shared(struct mimfs_inode *inode)
{
    mimfs_reader_writer_lock_release_shared(&(*inode).content_lock);
}

MIMFS_ALWAYS_INLINE void mimfs_inode_lock_exclusive(struct mimfs_inode *inode)
{
    mimfs_reader_writer_lock_acquire_exclusive(&(*inode).content_lock);
}

MIMFS_ALWAYS_INLINE void mimfs_inode_unlock_exclusive(struct mimfs_inode *inode)
{
    mimfs_reader_writer_lock_release_exclusive(&(*inode).content_lock);
}

/* =========================================================================
 * Type predicates
 * ========================================================================= */

MIMFS_ALWAYS_INLINE bool mimfs_inode_is_regular_file(const struct mimfs_inode *inode)
{
    return MIMFS_MODE_IS_REGULAR_FILE((*inode).file_mode);
}

MIMFS_ALWAYS_INLINE bool mimfs_inode_is_directory(const struct mimfs_inode *inode)
{
    return MIMFS_MODE_IS_DIRECTORY((*inode).file_mode);
}

MIMFS_ALWAYS_INLINE bool mimfs_inode_is_symbolic_link(const struct mimfs_inode *inode)
{
    return MIMFS_MODE_IS_SYMBOLIC_LINK((*inode).file_mode);
}

/*
 * True when this inode is a symbolic link whose target text is held inside the
 * inode itself rather than in a data block.
 *
 * This predicate has to be consulted before any block map operation on a symbolic
 * link, because for such an inode the pointer array does not contain pointers at
 * all: it contains the target text. Treating those bytes as block numbers would
 * free blocks belonging to other files.
 */
MIMFS_ALWAYS_INLINE bool
mimfs_inode_uses_inline_symbolic_link_storage(const struct mimfs_inode *inode)
{
    return MIMFS_MODE_IS_SYMBOLIC_LINK((*inode).file_mode) &&
           (*inode).file_size_in_bytes <= MIMFS_INLINE_SYMBOLIC_LINK_CAPACITY_BYTES;
}

/* True when the inode has been destroyed, or was never initialised. A zero mode
 * names no type, which is why zero is usable as the destroyed marker. */
MIMFS_ALWAYS_INLINE bool mimfs_inode_is_live(const struct mimfs_inode *inode)
{
    return (*inode).file_mode != 0u;
}

/* =========================================================================
 * Initialisation
 * =========================================================================
 * Called on an inode straight from the allocator, before it is reachable from any
 * directory. No lock is required, precisely because it is not yet reachable.
 */

void mimfs_inode_initialize_as_regular_file(struct mimfs_inode *inode,
                                            uint32_t permission_bits,
                                            uint32_t owner_user_identifier,
                                            uint32_t owner_group_identifier);

void mimfs_inode_initialize_as_directory(struct mimfs_inode *inode,
                                         uint32_t permission_bits,
                                         uint32_t owner_user_identifier,
                                         uint32_t owner_group_identifier);

void mimfs_inode_initialize_as_symbolic_link(struct mimfs_inode *inode,
                                             uint32_t owner_user_identifier,
                                             uint32_t owner_group_identifier);

/* =========================================================================
 * Timestamps
 * =========================================================================
 * All three require the inode lock held exclusively, with one deliberate
 * exception noted on the access time function.
 */

/*
 * Stamp the last read.
 *
 * Taken under a shared hold, which is a race by construction: two concurrent
 * readers can write this field at the same time. The race is benign and it is
 * accepted on purpose, because the alternative is to upgrade every read of every
 * file to an exclusive hold and give up read parallelism entirely for the sake of
 * one advisory timestamp. A 64 bit aligned store cannot tear on any supported
 * target, so the field always holds one of the times a reader actually observed.
 */
void mimfs_inode_stamp_access_time(struct mimfs_inode *inode);

/* Stamp a change to the contents. Also advances the status change time, since a
 * content change is a metadata change. Requires the exclusive hold. */
void mimfs_inode_stamp_modification_time(struct mimfs_inode *inode);

/* Stamp a change to the metadata alone. Requires the exclusive hold. */
void mimfs_inode_stamp_status_change_time(struct mimfs_inode *inode);

/* =========================================================================
 * Link counting
 * =========================================================================
 * Both require the exclusive hold.
 */

/* Returns false when the count is already at the limit, leaving it unchanged. */
bool mimfs_inode_increment_hard_link_count(struct mimfs_inode *inode);

/* Returns the count remaining after the decrement. */
uint32_t mimfs_inode_decrement_hard_link_count(struct mimfs_inode *inode);

/* =========================================================================
 * Reference counting and destruction
 * ========================================================================= */

/*
 * Pin an inode so that it cannot be destroyed or its number reused.
 *
 * Must be called while the inode is known to be reachable, which in practice
 * means while holding the lock of the directory that names it, or while already
 * holding another reference. That requirement is what makes the pin race free:
 * the inode cannot be destroyed between the caller establishing that it exists
 * and the count being raised.
 */
void mimfs_inode_acquire_reference(struct mimfs_volume *volume, uint64_t inode_number);

/*
 * Drop a pin, and destroy the inode if that was the last thing keeping it alive.
 *
 * The caller must hold no lock on this inode, because destruction takes the
 * exclusive hold internally.
 */
void mimfs_inode_release_reference(struct mimfs_volume *volume, uint64_t inode_number);

/*
 * Destroy the inode if it has neither a name nor a reference left, releasing every
 * block it owns and returning it to the allocator.
 *
 * Safe to call when the inode is still referenced; it simply does nothing. The
 * caller must hold no lock on this inode.
 *
 * Returns true when the inode was destroyed.
 */
bool mimfs_inode_destroy_if_orphaned(struct mimfs_volume *volume,
                                     uint64_t inode_number);

/* =========================================================================
 * Reporting
 * ========================================================================= */

/* Copy out a consistent snapshot. Requires at least the shared hold. */
void mimfs_inode_capture_status(struct mimfs_volume *volume, uint64_t inode_number,
                                const struct mimfs_inode *inode,
                                struct mimfs_status_information *status_out);

/* The directory entry type tag matching an inode's mode. */
uint32_t mimfs_inode_entry_type_tag(const struct mimfs_inode *inode);

#endif /* MIMFS_INODE_H */
