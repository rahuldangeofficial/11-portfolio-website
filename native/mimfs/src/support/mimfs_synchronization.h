/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_synchronization.h
 * Purpose: The two locks the filesystem is built on, and the atomic counter
 *          helpers the allocators need.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * Why not the platform locks
 * ---------------------------------------------------------------------------
 * A reader writer lock from the C library is between 56 and 200 bytes wide
 * depending on the platform. One of those per inode would inflate the inode
 * from four cache lines to more than a dozen, would push the hot metadata of
 * one inode across several lines, and would make the inode table several times
 * larger for the same volume. The locks below occupy eight and four bytes,
 * carry no destructor, and are correctly initialised by zeroing, which is what
 * lets a whole inode table be prepared with a single pass.
 *
 * ---------------------------------------------------------------------------
 * Lock ordering discipline
 * ---------------------------------------------------------------------------
 * Deadlock freedom is a property of the order in which locks are taken, so the
 * order is fixed for the whole filesystem and stated here once:
 *
 *   1. the volume rename serialisation lock
 *   2. directory inode locks, parent before child when both are held
 *   3. the inode lock of the object being operated on
 *   4. open file description locks
 *
 * Two rules follow from it and are enforced by review of every call site:
 *
 *   * Path resolution never holds two inode locks at once. It read locks one
 *     directory, takes a reference on the child, then unlocks before it
 *     descends. That is why walking a path can never participate in a cycle.
 *   * Any operation that must hold two directory locks at once, which is only
 *     rename, first takes the volume rename lock. That single lock collapses
 *     the whole class of parent-child inversion cycles that renaming a
 *     directory into its own descendant would otherwise create.
 *
 * None of these locks is recursive. Re-entering one from the same thread
 * deadlocks, by design, because a recursion counter would cost a thread
 * identity field per lock and would hide exactly the design mistakes that this
 * discipline is meant to prevent.
 */

#ifndef MIMFS_SYNCHRONIZATION_H
#define MIMFS_SYNCHRONIZATION_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "mimfs_compiler.h"

/* =========================================================================
 * Reader writer lock
 * =========================================================================
 * The entire state of the lock is one 64 bit word, partitioned as:
 *
 *   bits  0..31   number of readers currently inside the critical section
 *   bit   32      a writer is currently inside the critical section
 *   bits 33..63   number of writers waiting to get in
 *
 * Holding all three counters in one word is what makes every transition a
 * single atomic operation: a reader entering, a writer entering and a writer
 * queueing all commit with one compare and exchange, so there is no window in
 * which the lock is half acquired.
 *
 * Waiting writers block new readers. That gives the lock writer preference,
 * which is the correct bias here because the writers are the operations that
 * make progress on shared metadata; a reader-preferring lock lets a stream of
 * directory lookups starve the mkdir that is trying to complete.
 */

#define MIMFS_LOCK_READER_COUNT_UNIT   UINT64_C(0x0000000000000001)
#define MIMFS_LOCK_READER_COUNT_MASK   UINT64_C(0x00000000FFFFFFFF)
#define MIMFS_LOCK_WRITER_ACTIVE_FLAG  UINT64_C(0x0000000100000000)
#define MIMFS_LOCK_PENDING_WRITER_UNIT UINT64_C(0x0000000200000000)
#define MIMFS_LOCK_PENDING_WRITER_MASK UINT64_C(0xFFFFFFFE00000000)

struct mimfs_reader_writer_lock
{
    _Atomic uint64_t lock_state_word;
};

/* An all-zero word is a fully unlocked lock, so this is only documentation. */
#define MIMFS_READER_WRITER_LOCK_INITIALIZER { 0 }

void mimfs_reader_writer_lock_initialize(struct mimfs_reader_writer_lock *lock);

void mimfs_reader_writer_lock_acquire_shared(struct mimfs_reader_writer_lock *lock);
void mimfs_reader_writer_lock_release_shared(struct mimfs_reader_writer_lock *lock);

void mimfs_reader_writer_lock_acquire_exclusive(struct mimfs_reader_writer_lock *lock);
void mimfs_reader_writer_lock_release_exclusive(struct mimfs_reader_writer_lock *lock);

/* Non blocking variants, for the paths that have a useful fallback. */
bool mimfs_reader_writer_lock_try_acquire_shared(struct mimfs_reader_writer_lock *lock);
bool mimfs_reader_writer_lock_try_acquire_exclusive(
    struct mimfs_reader_writer_lock *lock);

/* True when the lock is held in either mode. Diagnostics only: the answer is
 * already stale by the time the caller reads it. */
bool mimfs_reader_writer_lock_is_held(const struct mimfs_reader_writer_lock *lock);

/* =========================================================================
 * Spin lock
 * =========================================================================
 * Four bytes, acquired with a test and test-and-set so that a contended lock
 * generates read traffic on a shared line instead of a stream of invalidations.
 * Used only for critical sections of a handful of instructions, which is the
 * only place a spin lock is the right answer: updating the offset of an open
 * file description, and claiming a slot in a table.
 */

struct mimfs_spin_lock
{
    _Atomic uint32_t is_locked;
};

#define MIMFS_SPIN_LOCK_INITIALIZER { 0 }

void mimfs_spin_lock_initialize(struct mimfs_spin_lock *lock);
void mimfs_spin_lock_acquire(struct mimfs_spin_lock *lock);
void mimfs_spin_lock_release(struct mimfs_spin_lock *lock);
bool mimfs_spin_lock_try_acquire(struct mimfs_spin_lock *lock);

/* =========================================================================
 * Atomic counter helpers
 * ========================================================================= */

/*
 * Decrement a counter only if it is greater than zero, and report whether the
 * decrement happened.
 *
 * This is the primitive that makes the allocators safe without a lock. A
 * thread that wants a block first claims one unit of the free counter of a
 * group; only if the claim succeeds does it go on to find a bit in that group's
 * bitmap. Two threads can therefore never both succeed on the last free unit,
 * and no thread ever has to hold a lock across the bitmap scan.
 */
bool mimfs_atomic_decrement_if_positive(_Atomic uint64_t *counter);

/* Symmetric helper, used when an allocation has to be handed back. */
void mimfs_atomic_increment(_Atomic uint64_t *counter);

/*
 * Publish every store issued so far to any thread that later performs a
 * matching acquire. Used where a structure is filled in privately and then made
 * reachable by other threads.
 */
MIMFS_ALWAYS_INLINE void mimfs_publish_prior_stores(void)
{
    atomic_thread_fence(memory_order_release);
}

/* The matching side: observe every store published before the fence. */
MIMFS_ALWAYS_INLINE void mimfs_observe_published_stores(void)
{
    atomic_thread_fence(memory_order_acquire);
}

#endif /* MIMFS_SYNCHRONIZATION_H */
