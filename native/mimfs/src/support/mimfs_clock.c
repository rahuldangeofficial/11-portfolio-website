/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_clock.c
 * Purpose: Implementation of the timestamp source.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_clock.h"

#include <stdatomic.h>
#include <time.h>

#define MIMFS_NANOSECONDS_PER_SECOND INT64_C(1000000000)

/*
 * Held atomically because a test may install a clock while other threads are
 * already running, and because a torn read of a function pointer would be a
 * jump to nowhere.
 */
static _Atomic(mimfs_clock_source_function) mimfs_installed_clock_source = NULL;

static int64_t mimfs_read_real_time_clock(void)
{
    struct timespec current_time;

    /*
     * The real time clock is the correct choice for file timestamps even though
     * it can step backwards, because these values are reported to users and
     * compared against timestamps produced elsewhere on the system. A monotonic
     * clock would be immune to steps but would carry no calendar meaning.
     */
    if (clock_gettime(CLOCK_REALTIME, &current_time) != 0)
    {
        return 0;
    }

    return (int64_t)current_time.tv_sec * MIMFS_NANOSECONDS_PER_SECOND +
           (int64_t)current_time.tv_nsec;
}

int64_t mimfs_clock_current_time_nanoseconds(void)
{
    const mimfs_clock_source_function installed_source =
        atomic_load_explicit(&mimfs_installed_clock_source, memory_order_acquire);

    if (installed_source != NULL)
    {
        return installed_source();
    }

    return mimfs_read_real_time_clock();
}

void mimfs_clock_install_source_for_testing(mimfs_clock_source_function clock_source)
{
    atomic_store_explicit(&mimfs_installed_clock_source, clock_source,
                          memory_order_release);
}
