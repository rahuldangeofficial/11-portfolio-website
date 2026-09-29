/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_bitmap.h
 * Purpose: Lock free allocation bitmap. Both the block allocator and the inode
 *          allocator are thin policy layers over these four operations.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * Representation
 * ---------------------------------------------------------------------------
 * The bitmap is an array of 64 bit words. Bit b of word w represents the object
 * numbered w * 64 + b, and a set bit means allocated.
 *
 * Two decisions make this fast:
 *
 *   * The scan works on whole words. A word that is all ones is rejected in one
 *     comparison, so a full region of 64 objects costs a single load and a
 *     single branch instead of 64 iterations. Inside a word the first free bit
 *     is located by counting trailing zeroes, which is one instruction.
 *
 *   * The claim is a single atomic fetch-or on the word. There is no lock
 *     anywhere in the allocator: the fetch-or both sets the bit and reports
 *     whether this thread was the one that set it. A thread that loses the race
 *     already holds the updated word, so it retries inside the same word
 *     without a second load.
 *
 * All bit counts handled here are multiples of 64, which is guaranteed by the
 * layout engine. That removes the partial trailing word from every loop, which
 * would otherwise need a mask on the hottest path in the filesystem.
 */

#ifndef MIMFS_BITMAP_H
#define MIMFS_BITMAP_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "mimfs_compiler.h"

/* Sentinel returned by the search functions when nothing was available. */
#define MIMFS_BITMAP_NO_FREE_BIT UINT64_MAX

/* Number of words needed to hold bit_count bits. */
MIMFS_ALWAYS_INLINE uint64_t mimfs_bitmap_word_count_for_bits(uint64_t bit_count)
{
    return mimfs_divide_rounding_up(bit_count, MIMFS_BITMAP_WORD_BIT_COUNT);
}

/* Number of bytes needed to hold bit_count bits. */
MIMFS_ALWAYS_INLINE uint64_t mimfs_bitmap_byte_count_for_bits(uint64_t bit_count)
{
    return mimfs_bitmap_word_count_for_bits(bit_count) * sizeof(uint64_t);
}

/*
 * Claim the first free bit at or after search_start_bit_index, wrapping around
 * to the beginning of the bitmap once.
 *
 * The hint is what gives the allocators their locality: a block allocation
 * starts its search next to the blocks the same file already owns, so a file
 * written sequentially receives a run of consecutive blocks even while other
 * threads allocate concurrently.
 *
 * Returns the index of the claimed bit, or MIMFS_BITMAP_NO_FREE_BIT when every
 * bit in the range is taken.
 */
uint64_t mimfs_bitmap_claim_first_free_bit(_Atomic uint64_t *bitmap_words,
                                           uint64_t bit_count,
                                           uint64_t search_start_bit_index);

/*
 * Release one bit. Releasing a bit that is already clear is a programming error
 * in the caller; the function reports it rather than corrupting the accounting,
 * which is how a double free is caught at the point it happens.
 *
 * Returns true when the bit was set and is now clear.
 */
bool mimfs_bitmap_release_bit(_Atomic uint64_t *bitmap_words, uint64_t bit_index);

/* Claim one specific bit. Returns false when it was already taken. */
bool mimfs_bitmap_claim_specific_bit(_Atomic uint64_t *bitmap_words,
                                     uint64_t bit_index);

/* Test one bit. The answer may be stale the instant it is returned; it is used
 * by consistency checks and by the descriptor table validity test, both of
 * which re-verify under the appropriate lock. */
bool mimfs_bitmap_bit_is_set(const _Atomic uint64_t *bitmap_words, uint64_t bit_index);

/* Clear the whole bitmap, marking every object free. Format time only. */
void mimfs_bitmap_clear_all_bits(_Atomic uint64_t *bitmap_words, uint64_t bit_count);

/* Count how many bits are set in the range. Diagnostics and self checks only. */
uint64_t mimfs_bitmap_count_set_bits_in_range(const _Atomic uint64_t *bitmap_words,
                                              uint64_t bit_count);

#endif /* MIMFS_BITMAP_H */
