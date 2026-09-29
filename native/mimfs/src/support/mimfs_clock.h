/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_clock.h
 * Purpose: The one source of timestamps for the whole filesystem.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * Isolating the clock behind one function has two payoffs. Inode timestamp
 * handling contains no platform conditional, and a test can substitute a
 * deterministic clock to assert on timestamp behaviour without waiting for real
 * time to pass.
 */

#ifndef MIMFS_CLOCK_H
#define MIMFS_CLOCK_H

#include <stdint.h>

/*
 * Nanoseconds since the start of the Unix epoch, from the real time clock.
 *
 * Nanoseconds in a signed 64 bit integer covers the years 1678 to 2262, which
 * is why the format stores time this way rather than as a seconds and
 * nanoseconds pair: one aligned load, no normalisation, no year 2038 boundary.
 */
int64_t mimfs_clock_current_time_nanoseconds(void);

/*
 * Install a clock for testing. Passing NULL restores the real time clock.
 * The override is process wide and is intended to be installed before any
 * volume is mounted.
 */
typedef int64_t (*mimfs_clock_source_function)(void);
void mimfs_clock_install_source_for_testing(mimfs_clock_source_function clock_source);

#endif /* MIMFS_CLOCK_H */
