/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_name.c
 * Purpose: Implementation of path component validation and hashing.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_name.h"

#include "mimfs/mimfs_limits.h"

/* Offset basis and prime of the 64 bit FNV-1a construction. */
#define MIMFS_NAME_HASH_OFFSET_BASIS UINT64_C(0xCBF29CE484222325)
#define MIMFS_NAME_HASH_PRIME        UINT64_C(0x00000100000001B3)

bool mimfs_name_characters_are_valid(const char *characters, size_t length)
{
    if (MIMFS_UNLIKELY(characters == NULL))
    {
        return false;
    }

    if (MIMFS_UNLIKELY(length == 0u || length > MIMFS_MAXIMUM_FILE_NAME_LENGTH))
    {
        return false;
    }

    for (size_t index = 0u; index < length; index++)
    {
        const char character = *(characters + index);

        /*
         * A separator inside a component would let a caller smuggle a second
         * path level past the resolver, and an embedded NUL would make the name
         * stored in the directory differ from the name every later comparison
         * sees. Both are rejected here rather than anywhere downstream.
         */
        if (MIMFS_UNLIKELY(character == '/' || character == '\0'))
        {
            return false;
        }
    }

    return true;
}

uint32_t mimfs_name_compute_hash(const char *characters, size_t length)
{
    uint64_t accumulator = MIMFS_NAME_HASH_OFFSET_BASIS;

    for (size_t index = 0u; index < length; index++)
    {
        accumulator ^= (uint64_t)(unsigned char)*(characters + index);
        accumulator *= MIMFS_NAME_HASH_PRIME;
    }

    /*
     * Fold the 64 bit accumulator into 32 bits by mixing the halves, so that
     * entropy concentrated in the high word is not discarded by a plain
     * truncation.
     */
    uint32_t folded_hash = (uint32_t)(accumulator ^ (accumulator >> 32));

    /* Reserve zero as the "no hash" marker. */
    return folded_hash != 0u ? folded_hash : 1u;
}

bool mimfs_name_reference_initialize(struct mimfs_name_reference *reference,
                                     const char *characters, size_t length)
{
    if (MIMFS_UNLIKELY(reference == NULL))
    {
        return false;
    }

    if (!mimfs_name_characters_are_valid(characters, length))
    {
        return false;
    }

    (*reference).characters = characters;
    (*reference).length = (uint32_t)length;
    (*reference).hash = mimfs_name_compute_hash(characters, length);

    return true;
}
