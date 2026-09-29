/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_compiler.h
 * Purpose: The single place where the code talks to the compiler and to the
 *          CPU directly. Branch hints, alignment control, prefetch and the
 *          spin-wait instruction are wrapped here so that no other translation
 *          unit contains a target specific line.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#ifndef MIMFS_COMPILER_H
#define MIMFS_COMPILER_H

#include <stddef.h>
#include <stdint.h>

#include "mimfs/mimfs_limits.h"

/* -------------------------------------------------------------------------
 * Branch prediction hints
 * -------------------------------------------------------------------------
 * Used only where the imbalance is a property of the design rather than of a
 * guess: argument validation that fails on a caller bug, allocation that fails
 * on a full volume, a bitmap word that is already taken. Marking those cold
 * keeps the fall-through path of every hot loop contiguous in the instruction
 * stream.
 */
#define MIMFS_LIKELY(condition)   __builtin_expect(!!(condition), 1)
#define MIMFS_UNLIKELY(condition) __builtin_expect(!!(condition), 0)

/* -------------------------------------------------------------------------
 * Layout control
 * ------------------------------------------------------------------------- */

/* Force an object or a structure member onto its own cache line. */
#define MIMFS_CACHE_LINE_ALIGNED  _Alignas(MIMFS_CACHE_LINE_SIZE_BYTES)

/*
 * Insert enough padding to push whatever follows onto the next cache line.
 * Used inside structures where two adjacent fields are written by different
 * threads, which is the textbook shape of false sharing.
 */
#define MIMFS_CACHE_LINE_PADDING(name, used_bytes)                                     \
    unsigned char name[MIMFS_CACHE_LINE_SIZE_BYTES -                                   \
                       ((used_bytes) % MIMFS_CACHE_LINE_SIZE_BYTES)]

/* -------------------------------------------------------------------------
 * Inlining
 * ------------------------------------------------------------------------- */

/*
 * Reserved for leaf helpers that exist purely to give a name to two or three
 * machine instructions. Letting the compiler decide would be fine at high
 * optimisation levels, but the test and coverage builds run unoptimised and
 * those helpers sit inside the innermost loops of the block map walk.
 */
#define MIMFS_ALWAYS_INLINE static inline __attribute__((always_inline))

/* Helper whose body is trivial but which is not worth forcing inline. */
#define MIMFS_INLINE        static inline

/* -------------------------------------------------------------------------
 * Prefetch
 * ------------------------------------------------------------------------- */

/*
 * Bring a line into the closest cache level ahead of the access that needs it.
 * Both forms are issued exactly one iteration ahead in the loops that use them,
 * which is the distance that hides the latency of an in-memory filesystem
 * without evicting the line the current iteration is still reading.
 */
#define MIMFS_PREFETCH_FOR_READ(address)                                               \
    __builtin_prefetch((const void *)(address), 0, 3)
#define MIMFS_PREFETCH_FOR_WRITE(address)                                              \
    __builtin_prefetch((const void *)(address), 1, 3)

/* -------------------------------------------------------------------------
 * Spin-wait hint
 * -------------------------------------------------------------------------
 * Tells the CPU that the current thread is in a busy wait. On x86 this drains
 * the pipeline of speculated loads and cuts the power drawn by the loop; on
 * AArch64 it hints the core to yield issue slots to the other hardware thread.
 * Both turn a tight retry loop from a memory bus hazard into a cheap wait.
 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define MIMFS_CPU_SPIN_WAIT_HINT() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
#define MIMFS_CPU_SPIN_WAIT_HINT() __asm__ __volatile__("yield" ::: "memory")
#else
#define MIMFS_CPU_SPIN_WAIT_HINT() __asm__ __volatile__("" ::: "memory")
#endif

/* -------------------------------------------------------------------------
 * Bit scanning
 * ------------------------------------------------------------------------- */

/*
 * Index of the lowest set bit of a non zero 64 bit word. The whole free space
 * search is built on this: one word of bitmap is examined per instruction pair
 * instead of one bit per iteration.
 */
MIMFS_ALWAYS_INLINE uint32_t mimfs_count_trailing_zero_bits(uint64_t nonzero_word)
{
    return (uint32_t)__builtin_ctzll(nonzero_word);
}

/* Population count, used by the consistency checks of the allocators. */
MIMFS_ALWAYS_INLINE uint32_t mimfs_count_set_bits(uint64_t word)
{
    return (uint32_t)__builtin_popcountll(word);
}

/* -------------------------------------------------------------------------
 * Alignment arithmetic
 * ------------------------------------------------------------------------- */

/*
 * Round a value up to the next multiple of a power of two. Expressed with the
 * two's complement identity, so it compiles to a negate, an and and an add
 * with no branch and no division.
 */
MIMFS_ALWAYS_INLINE uint64_t
mimfs_round_up_to_power_of_two_multiple(uint64_t value, uint64_t power_of_two)
{
    return (value + (power_of_two - 1u)) & ~(power_of_two - 1u);
}

MIMFS_ALWAYS_INLINE uint64_t
mimfs_round_down_to_power_of_two_multiple(uint64_t value, uint64_t power_of_two)
{
    return value & ~(power_of_two - 1u);
}

/* Align a raw address upwards, returning the aligned address. */
MIMFS_ALWAYS_INLINE uintptr_t mimfs_align_address_upwards(uintptr_t address,
                                                          uintptr_t alignment)
{
    return (address + (alignment - 1u)) & ~(alignment - 1u);
}

MIMFS_ALWAYS_INLINE int mimfs_value_is_power_of_two(uint64_t value)
{
    return value != 0u && (value & (value - 1u)) == 0u;
}

/* Ceiling division for unsigned values, without a branch. */
MIMFS_ALWAYS_INLINE uint64_t mimfs_divide_rounding_up(uint64_t numerator,
                                                      uint64_t denominator)
{
    return (numerator + denominator - 1u) / denominator;
}

MIMFS_ALWAYS_INLINE uint64_t mimfs_minimum_of(uint64_t left, uint64_t right)
{
    return left < right ? left : right;
}

MIMFS_ALWAYS_INLINE uint64_t mimfs_maximum_of(uint64_t left, uint64_t right)
{
    return left > right ? left : right;
}

#endif /* MIMFS_COMPILER_H */
