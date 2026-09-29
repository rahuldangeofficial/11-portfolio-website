/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/support/mimfs_name.h
 * Purpose: Validation, hashing and comparison of single path components.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * Every rule about what a file name may contain lives in this one module, so
 * the namespace layer never inspects a character itself and there is exactly one
 * definition of a valid name to audit.
 */

#ifndef MIMFS_NAME_H
#define MIMFS_NAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mimfs_compiler.h"

/*
 * Fully qualify a component: a pointer into the caller's path buffer plus a
 * length, so that a component can be handled without copying it out of the
 * path, and a precomputed hash so that a directory scan compares one integer
 * before it touches any characters.
 */
struct mimfs_name_reference
{
    const char *characters;
    uint32_t length;
    uint32_t hash;
};

/*
 * Reject a name that no directory may hold: empty, longer than the entry can
 * store, or containing a path separator or an embedded NUL.
 *
 * "." and ".." are valid names at this level. They are given their meaning by
 * the resolver, not by the character rules, which is what allows the two
 * entries every directory contains to be created through the ordinary insert
 * path.
 */
bool mimfs_name_characters_are_valid(const char *characters, size_t length);

/*
 * Build a reference to a component, computing its hash and validating it.
 * Returns false and leaves the reference untouched when the name is not valid.
 */
bool mimfs_name_reference_initialize(struct mimfs_name_reference *reference,
                                     const char *characters, size_t length);

/*
 * Hash of a name.
 *
 * The function is the 64 bit FNV-1a mixer folded to 32 bits. It is chosen over
 * anything stronger because the only property required is that unequal names
 * rarely collide within one directory, and it is chosen over anything weaker
 * because a hash that ignores character position turns a directory of names
 * that are permutations of each other into a linear scan of full comparisons.
 * One multiply and one exclusive or per byte is cheap enough that hashing a
 * component costs less than the single cache miss it saves.
 *
 * The value zero is never returned, so a stored hash of zero can be used to
 * mean "not hashed" without stealing a bit.
 */
uint32_t mimfs_name_compute_hash(const char *characters, size_t length);

/*
 * Compare a candidate stored in a directory entry against a reference.
 * The hash is compared first by the caller; this performs the definitive
 * character comparison.
 */
MIMFS_ALWAYS_INLINE bool
mimfs_name_matches(const char *candidate_characters, uint32_t candidate_length,
                   const struct mimfs_name_reference *reference)
{
    if (candidate_length != (*reference).length)
    {
        return false;
    }

    const char *left = candidate_characters;
    const char *right = (*reference).characters;

    for (uint32_t index = 0u; index < candidate_length; index++)
    {
        if (*(left + index) != *(right + index))
        {
            return false;
        }
    }

    return true;
}

/* True when the component is "." or "..", the two names that always resolve
 * relative to the directory that holds them. */
MIMFS_ALWAYS_INLINE bool mimfs_name_is_current_directory(const char *characters,
                                                         uint32_t length)
{
    return length == 1u && *(characters + 0) == '.';
}

MIMFS_ALWAYS_INLINE bool mimfs_name_is_parent_directory(const char *characters,
                                                        uint32_t length)
{
    return length == 2u && *(characters + 0) == '.' && *(characters + 1) == '.';
}

#endif /* MIMFS_NAME_H */
