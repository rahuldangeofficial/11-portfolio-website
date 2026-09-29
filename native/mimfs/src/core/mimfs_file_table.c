/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_file_table.c
 * Purpose: Implementation of the descriptor and open file description tables.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_file_table.h"

#include "mimfs_inode.h"
#include "support/mimfs_bitmap.h"

#define MIMFS_DESCRIPTOR_NOT_OPEN (-1)

/* True when the integer could name a slot of this volume's descriptor table. */
static bool mimfs_descriptor_is_in_range(struct mimfs_volume *volume, int descriptor)
{
    return descriptor >= 0 && (uint32_t)descriptor < (*volume).maximum_open_file_count;
}

/*
 * Claim a slot from one of the two tables.
 *
 * The search starts from the shared rotor rather than from zero, so that threads
 * opening files at the same time claim slots in different bitmap words instead of
 * contending on the first one.
 */
static uint64_t mimfs_claim_table_slot(struct mimfs_volume *volume,
                                       _Atomic uint64_t *slot_bitmap)
{
    const uint32_t slot_count = (*volume).maximum_open_file_count;

    const uint64_t search_hint = atomic_fetch_add_explicit(&(*volume).allocation_rotor,
                                                           1u, memory_order_relaxed) %
                                 slot_count;

    return mimfs_bitmap_claim_first_free_bit(slot_bitmap, slot_count, search_hint);
}

int mimfs_file_table_open_descriptor(struct mimfs_volume *volume, uint64_t inode_number,
                                     unsigned int retained_open_flags,
                                     uint64_t initial_file_offset)
{
    _Atomic uint64_t *description_bitmap =
        mimfs_open_file_description_allocation_bitmap(volume);

    const uint64_t description_index =
        mimfs_claim_table_slot(volume, description_bitmap);

    if (description_index == MIMFS_BITMAP_NO_FREE_BIT)
    {
        return -MIMFS_STATUS_TOO_MANY_OPEN_FILES;
    }

    _Atomic uint64_t *descriptor_bitmap = mimfs_descriptor_allocation_bitmap(volume);

    const uint64_t descriptor_index = mimfs_claim_table_slot(volume, descriptor_bitmap);

    if (descriptor_index == MIMFS_BITMAP_NO_FREE_BIT)
    {
        (void)mimfs_bitmap_release_bit(description_bitmap, description_index);
        return -MIMFS_STATUS_TOO_MANY_OPEN_FILES;
    }

    struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

    struct mimfs_open_file_description *description =
        mimfs_open_file_description_at(volume, description_index);

    /*
     * The slot is owned exclusively by this thread until the descriptor entry below
     * publishes it, so it is filled in without any synchronisation.
     */
    mimfs_spin_lock_initialize(&(*description).offset_lock);
    (*description).inode_number = (uint32_t)inode_number;
    (*description).generation_number = (*inode).generation_number;
    (*description).retained_open_flags = retained_open_flags;
    (*description).file_offset_in_bytes = initial_file_offset;

    atomic_store_explicit(&(*description).reference_count, 1u, memory_order_relaxed);

    struct mimfs_descriptor_table_entry *entry =
        mimfs_descriptor_table_entry_at(volume, descriptor_index);

    (*entry).descriptor_flags = 0u;

    /*
     * Storing the index is what makes the descriptor usable, so it happens last and
     * with release ordering: any thread that later reads a non negative index is
     * guaranteed to see the fully populated description behind it.
     */
    atomic_store_explicit(&(*entry).open_file_description_index,
                          (int32_t)description_index, memory_order_release);

    mimfs_atomic_increment(&(*volume).open_descriptor_count);

    return (int)descriptor_index;
}

struct mimfs_open_file_description *
mimfs_file_table_resolve_descriptor(struct mimfs_volume *volume, int descriptor)
{
    if (MIMFS_UNLIKELY(!mimfs_descriptor_is_in_range(volume, descriptor)))
    {
        return NULL;
    }

    struct mimfs_descriptor_table_entry *entry =
        mimfs_descriptor_table_entry_at(volume, (uint64_t)descriptor);

    const int32_t description_index = atomic_load_explicit(
        &(*entry).open_file_description_index, memory_order_acquire);

    if (MIMFS_UNLIKELY(description_index < 0 || (uint32_t)description_index >=
                                                    (*volume).maximum_open_file_count))
    {
        return NULL;
    }

    return mimfs_open_file_description_at(volume, (uint64_t)description_index);
}

uint64_t mimfs_file_table_description_inode_number(
    struct mimfs_volume *volume, struct mimfs_open_file_description *description)
{
    const uint64_t inode_number = (*description).inode_number;

    if (MIMFS_UNLIKELY(!mimfs_inode_number_is_in_range(volume, inode_number)))
    {
        return MIMFS_INVALID_INODE_NUMBER;
    }

    struct mimfs_inode *inode = mimfs_inode_at(volume, inode_number);

    /*
     * The description holds a reference on this inode, so the inode cannot have been
     * destroyed and its number cannot have been reused. The generation comparison
     * therefore always succeeds, and it is kept because it is the check that would
     * catch a defect in the reference counting before that defect could turn into a
     * descriptor silently operating on an unrelated file.
     */
    if (MIMFS_UNLIKELY((*inode).generation_number != (*description).generation_number))
    {
        return MIMFS_INVALID_INODE_NUMBER;
    }

    return inode_number;
}

int mimfs_file_table_close_descriptor(struct mimfs_volume *volume, int descriptor)
{
    if (MIMFS_UNLIKELY(!mimfs_descriptor_is_in_range(volume, descriptor)))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    struct mimfs_descriptor_table_entry *entry =
        mimfs_descriptor_table_entry_at(volume, (uint64_t)descriptor);

    /*
     * The exchange is the close. Only the thread that takes the index away performs the
     * release, which is what makes two threads closing one descriptor safe.
     */
    const int32_t description_index =
        atomic_exchange_explicit(&(*entry).open_file_description_index,
                                 MIMFS_DESCRIPTOR_NOT_OPEN, memory_order_acq_rel);

    if (MIMFS_UNLIKELY(description_index < 0 || (uint32_t)description_index >=
                                                    (*volume).maximum_open_file_count))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    (void)mimfs_bitmap_release_bit(mimfs_descriptor_allocation_bitmap(volume),
                                   (uint64_t)descriptor);

    struct mimfs_open_file_description *description =
        mimfs_open_file_description_at(volume, (uint64_t)description_index);

    /* Read out what the release will need before the slot can be reclaimed. */
    const uint64_t inode_number = (*description).inode_number;

    const uint32_t reference_count_before_release = atomic_fetch_sub_explicit(
        &(*description).reference_count, 1u, memory_order_acq_rel);

    if (reference_count_before_release == 1u)
    {
        /*
         * The last descriptor is gone, so the description slot goes back and the inode
         * reference it held is dropped. Dropping that reference is what destroys a file
         * whose last name was removed while it was open.
         */
        (void)mimfs_bitmap_release_bit(
            mimfs_open_file_description_allocation_bitmap(volume),
            (uint64_t)description_index);

        mimfs_inode_release_reference(volume, inode_number);
    }

    /* The counter is only reported through the statistics call, so a decrement that
     * momentarily lags behind is harmless; it is still kept exact. */
    if (atomic_load_explicit(&(*volume).open_descriptor_count, memory_order_relaxed) >
        0u)
    {
        (void)mimfs_atomic_decrement_if_positive(&(*volume).open_descriptor_count);
    }

    return MIMFS_STATUS_SUCCESS;
}

int mimfs_file_table_duplicate_descriptor(struct mimfs_volume *volume, int descriptor)
{
    if (MIMFS_UNLIKELY(!mimfs_descriptor_is_in_range(volume, descriptor)))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    struct mimfs_descriptor_table_entry *source_entry =
        mimfs_descriptor_table_entry_at(volume, (uint64_t)descriptor);

    const int32_t description_index = atomic_load_explicit(
        &(*source_entry).open_file_description_index, memory_order_acquire);

    if (MIMFS_UNLIKELY(description_index < 0 || (uint32_t)description_index >=
                                                    (*volume).maximum_open_file_count))
    {
        return -MIMFS_STATUS_BAD_DESCRIPTOR;
    }

    struct mimfs_open_file_description *description =
        mimfs_open_file_description_at(volume, (uint64_t)description_index);

    const uint64_t new_descriptor_index =
        mimfs_claim_table_slot(volume, mimfs_descriptor_allocation_bitmap(volume));

    if (new_descriptor_index == MIMFS_BITMAP_NO_FREE_BIT)
    {
        return -MIMFS_STATUS_TOO_MANY_OPEN_FILES;
    }

    /*
     * The reference is raised before the new descriptor is published, so the
     * description cannot be released between the two steps by a close of the original
     * descriptor.
     */
    atomic_fetch_add_explicit(&(*description).reference_count, 1u,
                              memory_order_acq_rel);

    struct mimfs_descriptor_table_entry *new_entry =
        mimfs_descriptor_table_entry_at(volume, new_descriptor_index);

    (*new_entry).descriptor_flags = 0u;

    atomic_store_explicit(&(*new_entry).open_file_description_index, description_index,
                          memory_order_release);

    mimfs_atomic_increment(&(*volume).open_descriptor_count);

    return (int)new_descriptor_index;
}

/* =========================================================================
 * The shared file offset
 * ========================================================================= */

uint64_t mimfs_file_table_claim_offset(struct mimfs_open_file_description *description,
                                       uint64_t byte_count)
{
    mimfs_spin_lock_acquire(&(*description).offset_lock);

    const uint64_t claimed_offset = (*description).file_offset_in_bytes;

    (*description).file_offset_in_bytes = claimed_offset + byte_count;

    mimfs_spin_lock_release(&(*description).offset_lock);

    return claimed_offset;
}

void mimfs_file_table_rewind_offset(struct mimfs_open_file_description *description,
                                    uint64_t unused_byte_count)
{
    if (unused_byte_count == 0u)
    {
        return;
    }

    mimfs_spin_lock_acquire(&(*description).offset_lock);

    if ((*description).file_offset_in_bytes >= unused_byte_count)
    {
        (*description).file_offset_in_bytes -= unused_byte_count;
    }
    else
    {
        (*description).file_offset_in_bytes = 0u;
    }

    mimfs_spin_lock_release(&(*description).offset_lock);
}

uint64_t mimfs_file_table_read_offset(struct mimfs_open_file_description *description)
{
    mimfs_spin_lock_acquire(&(*description).offset_lock);

    const uint64_t current_offset = (*description).file_offset_in_bytes;

    mimfs_spin_lock_release(&(*description).offset_lock);

    return current_offset;
}

void mimfs_file_table_offset_lock_acquire(
    struct mimfs_open_file_description *description)
{
    mimfs_spin_lock_acquire(&(*description).offset_lock);
}

void mimfs_file_table_offset_lock_release(
    struct mimfs_open_file_description *description)
{
    mimfs_spin_lock_release(&(*description).offset_lock);
}

uint64_t
mimfs_file_table_read_offset_locked(struct mimfs_open_file_description *description)
{
    return (*description).file_offset_in_bytes;
}

void mimfs_file_table_write_offset_locked(
    struct mimfs_open_file_description *description, uint64_t new_file_offset)
{
    (*description).file_offset_in_bytes = new_file_offset;
}

uint64_t mimfs_file_table_write_offset(struct mimfs_open_file_description *description,
                                       uint64_t new_file_offset)
{
    mimfs_spin_lock_acquire(&(*description).offset_lock);

    (*description).file_offset_in_bytes = new_file_offset;

    mimfs_spin_lock_release(&(*description).offset_lock);

    return new_file_offset;
}

/* =========================================================================
 * Bulk close
 * ========================================================================= */

void mimfs_file_table_close_all_descriptors(struct mimfs_volume *volume)
{
    const uint32_t descriptor_count = (*volume).maximum_open_file_count;

    for (uint32_t descriptor = 0u; descriptor < descriptor_count; descriptor++)
    {
        (void)mimfs_file_table_close_descriptor(volume, (int)descriptor);
    }
}
