/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_synchronization.c
 * Purpose: Implementation of the reader writer lock, the spin lock and the
 *          atomic counter helpers declared in mimfs_synchronization.h.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_synchronization.h"

#include <sched.h>

/*
 * Adaptive backoff shared by both locks.
 *
 * The first attempts spin on the CPU wait hint, because a critical section in
 * this filesystem is a few hundred cycles long and the holder is almost always
 * running on another core right now. Once the spin budget is exhausted the
 * assumption has been disproved: the holder is most likely descheduled, and
 * burning a full quantum waiting for a thread that is not running wastes the
 * core. From that point the waiter hands the core back on every attempt.
 *
 * The counter is passed by pointer so that one backoff state spans the whole
 * retry loop of one acquisition rather than restarting on every iteration.
 */
static void mimfs_backoff_once(uint32_t *spin_attempt_counter)
{
    if (MIMFS_LIKELY(*spin_attempt_counter < MIMFS_LOCK_SPIN_ITERATIONS_BEFORE_YIELD))
    {
        (*spin_attempt_counter)++;
        MIMFS_CPU_SPIN_WAIT_HINT();
        return;
    }

    sched_yield();
}

/* =========================================================================
 * Reader writer lock
 * ========================================================================= */

void mimfs_reader_writer_lock_initialize(struct mimfs_reader_writer_lock *lock)
{
    atomic_store_explicit(&(*lock).lock_state_word, 0u, memory_order_relaxed);
}

void mimfs_reader_writer_lock_acquire_shared(struct mimfs_reader_writer_lock *lock)
{
    uint32_t spin_attempt_counter = 0u;

    for (;;)
    {
        uint64_t observed_state =
            atomic_load_explicit(&(*lock).lock_state_word, memory_order_relaxed);

        /*
         * A reader may enter only when no writer holds the lock and no writer
         * is queued. Deferring to queued writers is what bounds the wait of a
         * writer under a continuous stream of readers.
         */
        if (MIMFS_LIKELY((observed_state & (MIMFS_LOCK_WRITER_ACTIVE_FLAG |
                                            MIMFS_LOCK_PENDING_WRITER_MASK)) == 0u))
        {
            uint64_t desired_state = observed_state + MIMFS_LOCK_READER_COUNT_UNIT;

            /*
             * The acquire ordering on success is what makes every store the
             * previous writer performed visible inside this critical section.
             */
            if (MIMFS_LIKELY(atomic_compare_exchange_weak_explicit(
                    &(*lock).lock_state_word, &observed_state, desired_state,
                    memory_order_acquire, memory_order_relaxed)))
            {
                return;
            }

            /* Lost the race to another thread; retry without backing off, the
             * lock was free a nanosecond ago and very likely still is. */
            continue;
        }

        mimfs_backoff_once(&spin_attempt_counter);
    }
}

bool mimfs_reader_writer_lock_try_acquire_shared(struct mimfs_reader_writer_lock *lock)
{
    uint64_t observed_state =
        atomic_load_explicit(&(*lock).lock_state_word, memory_order_relaxed);

    if ((observed_state &
         (MIMFS_LOCK_WRITER_ACTIVE_FLAG | MIMFS_LOCK_PENDING_WRITER_MASK)) != 0u)
    {
        return false;
    }

    return atomic_compare_exchange_strong_explicit(
        &(*lock).lock_state_word, &observed_state,
        observed_state + MIMFS_LOCK_READER_COUNT_UNIT, memory_order_acquire,
        memory_order_relaxed);
}

void mimfs_reader_writer_lock_release_shared(struct mimfs_reader_writer_lock *lock)
{
    /*
     * Release ordering hands every store this reader made, if any, to whoever
     * acquires next. A reader is not supposed to store into the protected
     * object, but the ordering costs nothing on the architectures that matter
     * and it keeps the lock correct for the read-mostly counters that are
     * updated under a shared hold, such as access timestamps.
     */
    atomic_fetch_sub_explicit(&(*lock).lock_state_word, MIMFS_LOCK_READER_COUNT_UNIT,
                              memory_order_release);
}

void mimfs_reader_writer_lock_acquire_exclusive(struct mimfs_reader_writer_lock *lock)
{
    uint32_t spin_attempt_counter = 0u;

    /*
     * Announce the intent before competing for the lock. From this moment no
     * new reader can enter, so the reader count can only fall, which is what
     * turns the loop below into a bounded wait rather than a gamble.
     */
    atomic_fetch_add_explicit(&(*lock).lock_state_word, MIMFS_LOCK_PENDING_WRITER_UNIT,
                              memory_order_relaxed);

    for (;;)
    {
        uint64_t observed_state =
            atomic_load_explicit(&(*lock).lock_state_word, memory_order_relaxed);

        if (MIMFS_LIKELY((observed_state & (MIMFS_LOCK_READER_COUNT_MASK |
                                            MIMFS_LOCK_WRITER_ACTIVE_FLAG)) == 0u))
        {
            /*
             * One transition both leaves the waiting queue and takes the lock.
             * Doing it in two steps would open a window in which the lock looks
             * free with no writer queued, letting a reader slip in front.
             */
            uint64_t desired_state = (observed_state - MIMFS_LOCK_PENDING_WRITER_UNIT) |
                                     MIMFS_LOCK_WRITER_ACTIVE_FLAG;

            if (MIMFS_LIKELY(atomic_compare_exchange_weak_explicit(
                    &(*lock).lock_state_word, &observed_state, desired_state,
                    memory_order_acquire, memory_order_relaxed)))
            {
                return;
            }

            continue;
        }

        mimfs_backoff_once(&spin_attempt_counter);
    }
}

bool mimfs_reader_writer_lock_try_acquire_exclusive(
    struct mimfs_reader_writer_lock *lock)
{
    uint64_t observed_state = 0u;

    /*
     * A try-acquire must not queue, because a failed attempt that left a
     * pending writer behind would block readers for no reason. It therefore
     * succeeds only against a completely idle lock.
     */
    return atomic_compare_exchange_strong_explicit(
        &(*lock).lock_state_word, &observed_state, MIMFS_LOCK_WRITER_ACTIVE_FLAG,
        memory_order_acquire, memory_order_relaxed);
}

void mimfs_reader_writer_lock_release_exclusive(struct mimfs_reader_writer_lock *lock)
{
    /*
     * Subtracting the flag rather than masking it off keeps the pending writer
     * count untouched, which a plain store of zero would destroy.
     */
    atomic_fetch_sub_explicit(&(*lock).lock_state_word, MIMFS_LOCK_WRITER_ACTIVE_FLAG,
                              memory_order_release);
}

bool mimfs_reader_writer_lock_is_held(const struct mimfs_reader_writer_lock *lock)
{
    uint64_t observed_state =
        atomic_load_explicit(&(*lock).lock_state_word, memory_order_relaxed);

    return (observed_state &
            (MIMFS_LOCK_READER_COUNT_MASK | MIMFS_LOCK_WRITER_ACTIVE_FLAG)) != 0u;
}

/* =========================================================================
 * Spin lock
 * ========================================================================= */

void mimfs_spin_lock_initialize(struct mimfs_spin_lock *lock)
{
    atomic_store_explicit(&(*lock).is_locked, 0u, memory_order_relaxed);
}

void mimfs_spin_lock_acquire(struct mimfs_spin_lock *lock)
{
    uint32_t spin_attempt_counter = 0u;

    for (;;)
    {
        /*
         * The exchange is attempted only when a plain load says the lock looks
         * free. That is the test and test-and-set pattern: while the lock is
         * held, every waiter sits in a shared read of one line instead of
         * firing a read-for-ownership that steals the line from the holder.
         */
        if (MIMFS_LIKELY(
                atomic_load_explicit(&(*lock).is_locked, memory_order_relaxed) == 0u))
        {
            if (MIMFS_LIKELY(atomic_exchange_explicit(&(*lock).is_locked, 1u,
                                                      memory_order_acquire) == 0u))
            {
                return;
            }
        }

        mimfs_backoff_once(&spin_attempt_counter);
    }
}

bool mimfs_spin_lock_try_acquire(struct mimfs_spin_lock *lock)
{
    if (atomic_load_explicit(&(*lock).is_locked, memory_order_relaxed) != 0u)
    {
        return false;
    }

    return atomic_exchange_explicit(&(*lock).is_locked, 1u, memory_order_acquire) == 0u;
}

void mimfs_spin_lock_release(struct mimfs_spin_lock *lock)
{
    atomic_store_explicit(&(*lock).is_locked, 0u, memory_order_release);
}

/* =========================================================================
 * Atomic counter helpers
 * ========================================================================= */

bool mimfs_atomic_decrement_if_positive(_Atomic uint64_t *counter)
{
    uint64_t observed_value = atomic_load_explicit(counter, memory_order_relaxed);

    while (observed_value != 0u)
    {
        /*
         * Compare and exchange rather than an unconditional fetch and subtract:
         * a counter that momentarily went negative would wrap to an enormous
         * positive value in unsigned arithmetic and hand out space the volume
         * does not have.
         */
        if (atomic_compare_exchange_weak_explicit(
                counter, &observed_value, observed_value - 1u, memory_order_acq_rel,
                memory_order_relaxed))
        {
            return true;
        }

        /* observed_value has been refreshed by the failed exchange. */
    }

    return false;
}

void mimfs_atomic_increment(_Atomic uint64_t *counter)
{
    atomic_fetch_add_explicit(counter, 1u, memory_order_acq_rel);
}
