/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_bitmap.c
 * Purpose: Implementation of the lock free allocation bitmap.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_bitmap.h"

/*
 * Split a bit index into the word that holds it and the bit position inside
 * that word. Both are shifts and masks because the word width is a power of
 * two, so the division the naive form would need never appears in the object
 * code.
 */
#define MIMFS_BITMAP_WORD_INDEX_OF(bit_index)   ((bit_index) >> 6)
#define MIMFS_BITMAP_BIT_POSITION_OF(bit_index) ((uint32_t)((bit_index) & 63u))
#define MIMFS_BITMAP_BIT_MASK_OF(bit_index)                                            \
    (UINT64_C(1) << MIMFS_BITMAP_BIT_POSITION_OF(bit_index))

/*
 * Claim a free bit inside one word, considering only positions at or after
 * first_allowed_bit_position.
 *
 * Returns the position claimed, or 64 when the word offered nothing. The loop
 * runs at most once per bit that another thread steals from under it, and every
 * iteration is a counted-trailing-zeroes plus a fetch-or, with no reload of the
 * word: the failed fetch-or already returned the current value.
 */
static uint32_t mimfs_claim_free_bit_within_word(_Atomic uint64_t *word_pointer,
                                                 uint32_t first_allowed_bit_position)
{
    /*
     * Mask away the positions before the hint so that the trailing zero count
     * cannot point at a bit the caller asked to skip.
     */
    const uint64_t allowed_position_mask =
        (first_allowed_bit_position == 0u) ? UINT64_MAX
                                           : (UINT64_MAX << first_allowed_bit_position);

    uint64_t observed_word = atomic_load_explicit(word_pointer, memory_order_relaxed);

    for (;;)
    {
        const uint64_t free_and_allowed_positions =
            ~observed_word & allowed_position_mask;

        if (free_and_allowed_positions == 0u)
        {
            return MIMFS_BITMAP_WORD_BIT_COUNT;
        }

        const uint32_t candidate_bit_position =
            mimfs_count_trailing_zero_bits(free_and_allowed_positions);
        const uint64_t candidate_bit_mask = UINT64_C(1) << candidate_bit_position;

        /*
         * The fetch-or is the whole claim. Acquire-release ordering makes the
         * bytes of the object that is being handed out visible to this thread,
         * and makes this thread's initialisation of the object visible to the
         * thread that later frees and re-allocates it.
         */
        const uint64_t previous_word = atomic_fetch_or_explicit(
            word_pointer, candidate_bit_mask, memory_order_acq_rel);

        if (MIMFS_LIKELY((previous_word & candidate_bit_mask) == 0u))
        {
            return candidate_bit_position;
        }

        /* Another thread took that exact bit first. Continue from the word it
         * left behind, which already reflects every concurrent claim. */
        observed_word = previous_word | candidate_bit_mask;
    }
}

uint64_t mimfs_bitmap_claim_first_free_bit(_Atomic uint64_t *bitmap_words,
                                           uint64_t bit_count,
                                           uint64_t search_start_bit_index)
{
    if (MIMFS_UNLIKELY(bitmap_words == NULL || bit_count == 0u))
    {
        return MIMFS_BITMAP_NO_FREE_BIT;
    }

    if (search_start_bit_index >= bit_count)
    {
        search_start_bit_index = 0u;
    }

    const uint64_t word_count = mimfs_bitmap_word_count_for_bits(bit_count);
    const uint64_t first_word_index =
        MIMFS_BITMAP_WORD_INDEX_OF(search_start_bit_index);
    const uint32_t first_bit_position =
        MIMFS_BITMAP_BIT_POSITION_OF(search_start_bit_index);

    /*
     * The sweep visits every word exactly once, starting at the hint and
     * wrapping through the beginning. word_count + 1 iterations are needed
     * because the word containing the hint is examined twice: once from the
     * hint position to its end, and once from its start when the wrap completes.
     */
    for (uint64_t visited_word_count = 0u; visited_word_count <= word_count;
         visited_word_count++)
    {
        const uint64_t word_index =
            (first_word_index + visited_word_count) % word_count;
        const uint32_t first_allowed_bit_position =
            (visited_word_count == 0u) ? first_bit_position : 0u;

        _Atomic uint64_t *word_pointer = bitmap_words + word_index;

        /* Reject a fully allocated word in one comparison. */
        if (atomic_load_explicit(word_pointer, memory_order_relaxed) == UINT64_MAX)
        {
            continue;
        }

        const uint32_t claimed_bit_position =
            mimfs_claim_free_bit_within_word(word_pointer, first_allowed_bit_position);

        if (MIMFS_LIKELY(claimed_bit_position < MIMFS_BITMAP_WORD_BIT_COUNT))
        {
            const uint64_t claimed_bit_index =
                word_index * MIMFS_BITMAP_WORD_BIT_COUNT + claimed_bit_position;

            /*
             * The layout engine guarantees bit_count is a whole number of words,
             * so this can only fail on a corrupted volume. Handing the bit back
             * keeps the accounting exact even in that case.
             */
            if (MIMFS_UNLIKELY(claimed_bit_index >= bit_count))
            {
                (void)mimfs_bitmap_release_bit(bitmap_words, claimed_bit_index);
                continue;
            }

            return claimed_bit_index;
        }
    }

    return MIMFS_BITMAP_NO_FREE_BIT;
}

bool mimfs_bitmap_release_bit(_Atomic uint64_t *bitmap_words, uint64_t bit_index)
{
    if (MIMFS_UNLIKELY(bitmap_words == NULL))
    {
        return false;
    }

    _Atomic uint64_t *word_pointer =
        bitmap_words + MIMFS_BITMAP_WORD_INDEX_OF(bit_index);
    const uint64_t bit_mask = MIMFS_BITMAP_BIT_MASK_OF(bit_index);

    const uint64_t previous_word =
        atomic_fetch_and_explicit(word_pointer, ~bit_mask, memory_order_acq_rel);

    /* A clear bit here means the object was freed twice. */
    return (previous_word & bit_mask) != 0u;
}

bool mimfs_bitmap_claim_specific_bit(_Atomic uint64_t *bitmap_words, uint64_t bit_index)
{
    if (MIMFS_UNLIKELY(bitmap_words == NULL))
    {
        return false;
    }

    _Atomic uint64_t *word_pointer =
        bitmap_words + MIMFS_BITMAP_WORD_INDEX_OF(bit_index);
    const uint64_t bit_mask = MIMFS_BITMAP_BIT_MASK_OF(bit_index);

    const uint64_t previous_word =
        atomic_fetch_or_explicit(word_pointer, bit_mask, memory_order_acq_rel);

    return (previous_word & bit_mask) == 0u;
}

bool mimfs_bitmap_bit_is_set(const _Atomic uint64_t *bitmap_words, uint64_t bit_index)
{
    if (MIMFS_UNLIKELY(bitmap_words == NULL))
    {
        return false;
    }

    const _Atomic uint64_t *word_pointer =
        bitmap_words + MIMFS_BITMAP_WORD_INDEX_OF(bit_index);

    return (atomic_load_explicit(word_pointer, memory_order_acquire) &
            MIMFS_BITMAP_BIT_MASK_OF(bit_index)) != 0u;
}

void mimfs_bitmap_clear_all_bits(_Atomic uint64_t *bitmap_words, uint64_t bit_count)
{
    if (MIMFS_UNLIKELY(bitmap_words == NULL))
    {
        return;
    }

    const uint64_t word_count = mimfs_bitmap_word_count_for_bits(bit_count);

    for (uint64_t word_index = 0u; word_index < word_count; word_index++)
    {
        atomic_store_explicit(bitmap_words + word_index, 0u, memory_order_relaxed);
    }
}

uint64_t mimfs_bitmap_count_set_bits_in_range(const _Atomic uint64_t *bitmap_words,
                                              uint64_t bit_count)
{
    if (MIMFS_UNLIKELY(bitmap_words == NULL))
    {
        return 0u;
    }

    const uint64_t word_count = mimfs_bitmap_word_count_for_bits(bit_count);
    uint64_t total_set_bit_count = 0u;

    for (uint64_t word_index = 0u; word_index < word_count; word_index++)
    {
        total_set_bit_count += mimfs_count_set_bits(
            atomic_load_explicit(bitmap_words + word_index, memory_order_relaxed));
    }

    return total_set_bit_count;
}
