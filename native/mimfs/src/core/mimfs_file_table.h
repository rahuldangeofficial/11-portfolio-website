/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_file_table.h
 * Purpose: Descriptors, the open file descriptions they point at, and the shared
 *          file offset each description carries.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * Two levels, for one reason
 * ---------------------------------------------------------------------------
 * A descriptor is a small integer that points at an open file description; the
 * description is what holds the offset, the flags and the inode. The indirection
 * exists so that duplicating a descriptor produces a second name for one
 * description, and therefore for one offset: a seek through either duplicate is seen
 * through both. Collapsing the two levels into one would make duplicates
 * independent, which is not what a duplicate is.
 *
 * ---------------------------------------------------------------------------
 * No lock on the table
 * ---------------------------------------------------------------------------
 * Slots in both tables are claimed from lock free bitmaps, and a claimed slot is
 * owned exclusively by the thread that claimed it, so filling it in needs no
 * synchronisation at all. A descriptor becomes usable only when its index field is
 * stored, which happens after the description behind it is complete.
 *
 * Closing exchanges that index field for the not-open marker. Exactly one thread can
 * receive the old value, so a descriptor closed concurrently from two threads is
 * released exactly once and the second caller is told the descriptor is not open.
 *
 * The only lock in this module is the small one inside each description, held for the
 * few instructions it takes to consume and advance the offset.
 */

#ifndef MIMFS_FILE_TABLE_H
#define MIMFS_FILE_TABLE_H

#include "mimfs_layout.h"

/*
 * Create a descriptor for an inode.
 *
 * The caller must already hold a reference on the inode; ownership of that reference
 * passes to the descriptor and is dropped when the descriptor, and every duplicate of
 * it, is closed.
 *
 * Returns the descriptor, or a negated status code, in which case the caller keeps
 * its reference and remains responsible for releasing it.
 */
int mimfs_file_table_open_descriptor(struct mimfs_volume *volume, uint64_t inode_number,
                                     unsigned int retained_open_flags,
                                     uint64_t initial_file_offset);

/* Close one descriptor, releasing the description and the inode reference when this
 * was the last descriptor pointing at it. */
int mimfs_file_table_close_descriptor(struct mimfs_volume *volume, int descriptor);

/* Produce a second descriptor sharing one description, and therefore one offset. */
int mimfs_file_table_duplicate_descriptor(struct mimfs_volume *volume, int descriptor);

/*
 * Resolve a descriptor to the description behind it.
 *
 * Returns NULL when the descriptor is not open. The returned description stays valid
 * for as long as the caller does not close that descriptor, because the reference the
 * descriptor holds keeps the description slot claimed.
 */
struct mimfs_open_file_description *
mimfs_file_table_resolve_descriptor(struct mimfs_volume *volume, int descriptor);

/* The inode a description is attached to, verified to still be the same inode that
 * was opened. Returns MIMFS_INVALID_INODE_NUMBER when the check fails. */
uint64_t mimfs_file_table_description_inode_number(
    struct mimfs_volume *volume, struct mimfs_open_file_description *description);

/*
 * Read the offset, and advance it by the given amount, as one indivisible step.
 *
 * This is what makes two threads reading through one descriptor see two disjoint
 * ranges of the file rather than the same bytes twice. The amount to advance has to
 * be decided before the transfer, so a transfer that turns out to be short corrects
 * the offset afterwards with mimfs_file_table_rewind_offset.
 */
uint64_t mimfs_file_table_claim_offset(struct mimfs_open_file_description *description,
                                       uint64_t byte_count);

/* Give back the part of a claimed range that was not transferred. */
void mimfs_file_table_rewind_offset(struct mimfs_open_file_description *description,
                                    uint64_t unused_byte_count);

/* Read the offset without changing it. */
uint64_t mimfs_file_table_read_offset(struct mimfs_open_file_description *description);

/*
 * Hold the offset lock across a sequence that has to read the offset, compute a new
 * one from it, and store it back.
 *
 * A directory stream needs this: its cursor advance depends on what the directory
 * actually contains, so it cannot be expressed as "claim this many bytes". The
 * caller must already hold the inode lock, which is the documented order, and must do
 * nothing inside the region that could block.
 */
void mimfs_file_table_offset_lock_acquire(
    struct mimfs_open_file_description *description);
void mimfs_file_table_offset_lock_release(
    struct mimfs_open_file_description *description);

/* Offset access for use inside that region only. */
uint64_t
mimfs_file_table_read_offset_locked(struct mimfs_open_file_description *description);
void mimfs_file_table_write_offset_locked(
    struct mimfs_open_file_description *description, uint64_t new_file_offset);

/* Set the offset outright, reporting the value it now holds. */
uint64_t mimfs_file_table_write_offset(struct mimfs_open_file_description *description,
                                       uint64_t new_file_offset);

/*
 * Close every descriptor still open on the volume. Used by unmount, so that an inode
 * whose last name was already removed is destroyed rather than left holding blocks
 * with no way to reach it.
 */
void mimfs_file_table_close_all_descriptors(struct mimfs_volume *volume);

#endif /* MIMFS_FILE_TABLE_H */
