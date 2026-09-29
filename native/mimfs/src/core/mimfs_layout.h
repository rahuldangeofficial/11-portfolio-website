/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_layout.h
 * Purpose: Every structure the volume is made of, and the accessors that turn a
 *          number into a pointer. This is the definition of the format.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * ---------------------------------------------------------------------------
 * Region layout of a volume
 * ---------------------------------------------------------------------------
 * A volume is one contiguous memory region, cut into regions that never move
 * for the lifetime of the format:
 *
 *   +--------------------------------------+ offset 0 (cache line aligned)
 *   | volume header, one block             |  geometry, counters, global locks
 *   +--------------------------------------+
 *   | open file description table          |  volatile, rebuilt on every mount
 *   +--------------------------------------+
 *   | descriptor table and its bitmap      |  volatile, rebuilt on every mount
 *   +--------------------------------------+
 *   | block group descriptor table         |  one cache line per group
 *   +--------------------------------------+
 *   | inode allocation bitmaps             |  all groups, back to back
 *   +--------------------------------------+
 *   | block allocation bitmaps             |  all groups, back to back
 *   +--------------------------------------+
 *   | inode table                          |  all groups, back to back
 *   +--------------------------------------+
 *   | data blocks                          |  the remainder
 *   +--------------------------------------+
 *
 * The metadata of all groups is gathered into shared regions rather than
 * interleaved with each group's data. Two properties follow. A sequential walk
 * of the inode table, which is what a whole-tree traversal performs, reads
 * contiguous memory instead of striding across the volume. And the data area is
 * one unbroken run, so a large file receives one long contiguous extent.
 *
 * Every region is located by a byte offset from the base of the volume, never by
 * an absolute pointer. The volume therefore contains no self references and can
 * be mapped at a different address by a different process, or relocated wholesale
 * with a single copy, and still mount.
 *
 * ---------------------------------------------------------------------------
 * Persistent state against volatile state
 * ---------------------------------------------------------------------------
 * Persistent: geometry, free counters, group descriptors, both bitmaps, the
 * inode table and the data blocks. These survive an unmount and are what a
 * later mount finds.
 *
 * Volatile: every lock word, every open reference count, the descriptor table
 * and the open file descriptions. Mounting rewrites all of it, which is what
 * makes a volume left behind by a process that died mid-operation mount cleanly
 * with no descriptors leaked and no lock stuck held.
 */

#ifndef MIMFS_LAYOUT_H
#define MIMFS_LAYOUT_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mimfs/mimfs_limits.h"
#include "mimfs/mimfs_types.h"
#include "support/mimfs_compiler.h"
#include "support/mimfs_synchronization.h"

/* =========================================================================
 * Identification
 * ========================================================================= */

/* "MIMF" in ASCII, readable in a memory dump of the first four bytes. */
#define MIMFS_VOLUME_MAGIC_NUMBER   0x4D494D46u

/*
 * Format revision. A mount refuses a revision it was not built for rather than
 * guessing at a layout, because misreading a geometry field is the one class of
 * bug that silently destroys data.
 */
#define MIMFS_FORMAT_REVISION       1u

/* Library version, reported through the public interface. */
#define MIMFS_LIBRARY_VERSION_MAJOR 1u
#define MIMFS_LIBRARY_VERSION_MINOR 0u
#define MIMFS_LIBRARY_VERSION_PATCH 0u

/* =========================================================================
 * Directory entry
 * =========================================================================
 * Exactly one cache line. A directory lookup walks these back to back, so the
 * fields are ordered by the sequence in which the comparison uses them: the
 * hash first, because a mismatching hash rejects the candidate without touching
 * the name at all, then the length, then the name itself.
 */
struct mimfs_directory_entry
{
    /* Inode named by this entry. Zero marks the slot as free. */
    uint32_t inode_number;

    /* Hash of the name, precomputed at insert time. Never zero when in use. */
    uint32_t name_hash;

    /* Length of the name in bytes, without a terminator. */
    uint16_t name_length;

    /* One of the MIMFS_ENTRY_TYPE_* tags, so a listing needs no inode read. */
    uint8_t entry_type;

    /* Must be zero. Reserved for a future flag byte. */
    uint8_t reserved_byte;

    /* The name, not NUL terminated; name_length is authoritative. */
    char name[MIMFS_MAXIMUM_FILE_NAME_LENGTH];
};

/* =========================================================================
 * Inode
 * =========================================================================
 * Exactly four cache lines, divided by how the fields are used rather than by
 * what they mean:
 *
 *   line 0  the lock, and the metadata that almost every operation touches
 *   line 1  the block map roots, read by every data transfer
 *   line 2  reserved, so that the format can grow without moving any inode
 *   line 3  the timestamps, which only metadata operations touch
 *
 * The consequence of the split is that a read of a small file touches lines 0
 * and 1 and leaves the timestamps out of cache entirely, while a stat call
 * touches lines 0 and 3 and leaves the pointer array out.
 */
struct mimfs_inode
{
    /* ---- line 0: lock and hot metadata ---------------------------------- */

    /*
     * Guards everything in this inode, and guards the block map reachable from
     * it. Placed first so that the lock word and the fields the lock holder
     * immediately reads land in the same line: taking the lock warms the data.
     */
    struct mimfs_reader_writer_lock content_lock;

    /*
     * Number of open descriptions plus in-flight path resolutions referring to
     * this inode. Volatile, atomic, and independent of the lock, because a
     * reference is taken and dropped by threads that do not hold the lock.
     *
     * The inode is destroyed when, and only when, both this count and the hard
     * link count have reached zero. That is what makes a file whose last name
     * was removed while it was still open stay alive until its last descriptor
     * closes.
     */
    _Atomic uint32_t open_reference_count;

    /* File type in the high bits, permissions in the low bits. */
    uint16_t file_mode;

    /* Number of directory entries naming this inode. */
    uint16_t hard_link_count;

    /* Length of the file in bytes. For a directory, its allocated extent. */
    uint64_t file_size_in_bytes;

    /* Blocks charged to this inode, data and indirect metadata together. */
    uint64_t allocated_block_count;

    uint32_t owner_user_identifier;
    uint32_t owner_group_identifier;

    /* Reserved for per-inode flags. Must be zero. */
    uint32_t inode_flags;

    /*
     * Incremented every time this inode number is handed out again. A stale
     * reference can be detected by comparing the generation it captured against
     * the current one, which is what stops a descriptor from following an inode
     * number into a completely unrelated file after a reuse.
     */
    uint32_t generation_number;

    unsigned char reserved_line_zero[16];

    /* ---- line 1: block map roots ---------------------------------------- */

    /*
     * Slots 0 to 11 address the first twelve blocks with no indirection at all,
     * which covers every file up to 48 kilobytes, the overwhelming majority.
     * Slots 12, 13 and 14 are the roots of the single, double and triple
     * indirect trees, extending the reach to four terabytes.
     *
     * For a symbolic link short enough to fit, these 60 bytes hold the target
     * text itself and no block is allocated at all.
     */
    union
    {
        uint32_t block_pointer_slot[MIMFS_INODE_BLOCK_POINTER_SLOT_COUNT];
        char inline_symbolic_link_target[MIMFS_INLINE_SYMBOLIC_LINK_CAPACITY_BYTES];
    } block_map_root;

    /*
     * Group the inode was allocated from. Kept next to the block map because the
     * allocator reads it to place new blocks near the ones already owned.
     */
    uint32_t home_block_group_index;

    /* ---- line 2: reserved ----------------------------------------------- */

    unsigned char reserved_line_two[64];

    /* ---- line 3: timestamps -------------------------------------------- */

    /*
     * Atomic, and the only field of an inode that is. It is the one field written
     * while the inode lock is held merely shared, because stamping the time of a read
     * must not force every read of every file to serialise behind an exclusive hold
     * for the sake of one advisory value.
     *
     * That makes concurrent readers write it at the same time. Declaring it atomic and
     * accessing it with relaxed ordering is what turns that from undefined behaviour
     * into a defined outcome: the field always holds one of the times a reader actually
     * observed, chosen arbitrarily among them. Relaxed ordering compiles to the same
     * plain load and store on every supported target, so the guarantee is free.
     */
    _Atomic int64_t last_access_time_nanoseconds;

    int64_t last_modification_time_nanoseconds;
    int64_t last_status_change_time_nanoseconds;
    int64_t creation_time_nanoseconds;

    unsigned char reserved_line_three[32];
};

/* =========================================================================
 * Block group descriptor
 * =========================================================================
 * One cache line per group, so a thread allocating from group A never
 * invalidates the line a thread allocating from group B is updating. This is
 * the single most important piece of padding in the whole format: without it,
 * the free counters of sixteen groups would share one line and the lock free
 * allocator would be slower than a locked one.
 */
struct mimfs_block_group_descriptor
{
    /* Data blocks belonging to this group; the last group may hold fewer. */
    uint32_t block_count_in_group;

    /* Inodes belonging to this group. */
    uint32_t inode_count_in_group;

    /* First block number of this group, in volume-wide numbering. */
    uint64_t first_block_number;

    /* First inode number of this group, in volume-wide numbering. */
    uint64_t first_inode_number;

    /*
     * Free counters. A thread claims a unit here before it touches the bitmap,
     * so two threads can never both succeed on the last free object and no
     * bitmap scan ever has to run under a lock.
     */
    _Atomic uint64_t free_block_count;
    _Atomic uint64_t free_inode_count;

    /*
     * Where the next search in this group should start. Purely a hint, updated
     * without synchronisation because a stale value costs a few wasted word
     * comparisons and nothing else.
     */
    _Atomic uint64_t next_block_search_hint;
    _Atomic uint64_t next_inode_search_hint;

    unsigned char reserved_padding[8];
};

/* =========================================================================
 * Open file description
 * =========================================================================
 * The object a descriptor points at. Duplicating a descriptor produces a second
 * reference to one description, which is what makes two duplicated descriptors
 * share a single file offset, exactly as the established interface requires.
 */
struct mimfs_open_file_description
{
    /*
     * Guards the offset. A spin lock rather than an atomic offset because a read
     * has to consume a range of the offset and advance it as one indivisible
     * step; an atomic add alone would let two threads reading through one
     * descriptor both start from the same place.
     */
    struct mimfs_spin_lock offset_lock;

    /* Descriptors currently pointing at this description. */
    _Atomic uint32_t reference_count;

    /* Inode this description is attached to. */
    uint32_t inode_number;

    /* Generation of that inode when the description was created. */
    uint32_t generation_number;

    /* The subset of the open flags that outlive the open call. */
    uint32_t retained_open_flags;

    /* Shared file offset, in bytes, or the entry cursor of a directory stream. */
    uint64_t file_offset_in_bytes;

    unsigned char reserved_padding[32];
};

/* =========================================================================
 * Descriptor table entry
 * =========================================================================
 * Deliberately tiny and separate from the description it points at, so that a
 * table of many thousands of descriptors stays a compact array that a validity
 * check can scan cheaply.
 */
struct mimfs_descriptor_table_entry
{
    /*
     * Index into the description table, or -1 when the descriptor is not open.
     *
     * Atomic, and closing a descriptor is an atomic exchange of this field for -1.
     * That is what makes closing the same descriptor from two threads safe rather
     * than a double release: exactly one of them receives the non negative index and
     * therefore exactly one of them performs the release.
     */
    _Atomic int32_t open_file_description_index;

    /* Per-descriptor flags. Reserved; must be zero. */
    uint32_t descriptor_flags;
};

/* =========================================================================
 * Volume header
 * =========================================================================
 * The first object in the region. Split into three parts by write frequency:
 * the geometry, which is written once at format time and read constantly; the
 * global counters, one per cache line because they are written by every
 * allocation; and the global locks.
 */
struct mimfs_volume
{
    /* ---- immutable geometry, read-only after format --------------------- */

    MIMFS_CACHE_LINE_ALIGNED uint32_t magic_number;
    uint32_t format_revision;

    /* Total bytes of the region, counted from the aligned base. */
    uint64_t volume_size_in_bytes;

    /* Bytes skipped at the front of the caller's region to reach alignment. */
    uint64_t alignment_padding_bytes;

    uint32_t block_size_in_bytes;
    uint32_t inode_size_in_bytes;

    uint64_t total_block_count;
    uint64_t total_inode_count;

    uint32_t block_group_count;
    uint32_t blocks_per_block_group;
    uint32_t inodes_per_block_group;
    uint32_t maximum_open_file_count;

    /*
     * Base two logarithms of the two group sizes. Both group sizes are powers of
     * two precisely so that mapping an object number to the group that owns it
     * is a shift. Without them the mapping would be an integer division, and it
     * sits on the path of every single allocation.
     */
    uint32_t blocks_per_block_group_log2;
    uint32_t inodes_per_block_group_log2;

    /*
     * Blocks in the final group, which is the only group allowed to be short.
     * Letting the tail group be partial is what lets a volume of any size use
     * essentially all of the memory it was given, instead of rounding the whole
     * data area down to a multiple of the group span.
     */
    uint32_t block_count_in_final_group;

    uint32_t reserved_geometry_field;

    /* Blocks held back from ordinary allocation. */
    uint64_t reserved_block_count;

    /* Byte offsets of every region, measured from the base of this structure. */
    uint64_t open_file_description_table_offset;
    uint64_t descriptor_table_offset;
    uint64_t descriptor_allocation_bitmap_offset;
    uint64_t block_group_descriptor_table_offset;
    uint64_t inode_allocation_bitmap_offset;
    uint64_t block_allocation_bitmap_offset;
    uint64_t inode_table_offset;
    uint64_t data_block_region_offset;

    /* ---- global counters, one per line ---------------------------------- */

    /*
     * Volume-wide totals, kept alongside the per-group counters so that a
     * statistics call does not have to sum every group. Each sits alone on a
     * cache line: they are the two most frequently written words in the whole
     * volume, and sharing a line between them would serialise block allocation
     * against inode allocation for no reason.
     */
    MIMFS_CACHE_LINE_ALIGNED _Atomic uint64_t free_block_count;
    MIMFS_CACHE_LINE_ALIGNED _Atomic uint64_t free_inode_count;

    /*
     * Rotates the starting group of an allocation that has no locality hint of
     * its own, spreading unrelated threads across groups instead of piling them
     * onto group zero.
     */
    MIMFS_CACHE_LINE_ALIGNED _Atomic uint64_t allocation_rotor;

    /* Descriptors currently open, for the statistics call. */
    MIMFS_CACHE_LINE_ALIGNED _Atomic uint64_t open_descriptor_count;

    /* ---- global locks --------------------------------------------------- */

    /*
     * Held for the duration of a rename. Renaming is the only operation that
     * must hold two directory locks at once, and the pair it needs is chosen by
     * the caller's paths rather than by any fixed order, so no per-inode
     * ordering rule can make it safe on its own. One volume-wide lock, taken
     * only by rename, removes the entire class of cycles at the cost of
     * serialising an operation that is rare in every real workload.
     */
    MIMFS_CACHE_LINE_ALIGNED struct mimfs_reader_writer_lock rename_serialization_lock;

    /* True once mounted; cleared by unmount so a stale handle is detectable. */
    MIMFS_CACHE_LINE_ALIGNED _Atomic uint32_t mount_state;
};

#define MIMFS_MOUNT_STATE_UNMOUNTED 0u
#define MIMFS_MOUNT_STATE_MOUNTED   1u

/* =========================================================================
 * Compile time verification of the format
 * =========================================================================
 * These assertions are the format specification in executable form. If a field
 * is ever added to a structure without adjusting its reserved area, the build
 * stops here rather than silently shifting every object in the volume.
 */

_Static_assert(sizeof(struct mimfs_directory_entry) == MIMFS_DIRECTORY_ENTRY_SIZE_BYTES,
               "A directory entry must occupy exactly one cache line.");

_Static_assert(sizeof(struct mimfs_inode) == MIMFS_INODE_SIZE_BYTES,
               "An inode must occupy exactly four cache lines.");

_Static_assert(sizeof(struct mimfs_block_group_descriptor) ==
                   MIMFS_CACHE_LINE_SIZE_BYTES,
               "A group descriptor must occupy exactly one cache line so that "
               "allocations in different groups cannot share a line.");

_Static_assert(sizeof(struct mimfs_open_file_description) ==
                   MIMFS_CACHE_LINE_SIZE_BYTES,
               "An open file description must occupy exactly one cache line.");

_Static_assert(sizeof(struct mimfs_inode) % MIMFS_CACHE_LINE_SIZE_BYTES == 0u,
               "The inode table must never place one inode across two lines.");

_Static_assert(MIMFS_BLOCK_SIZE_BYTES % sizeof(struct mimfs_directory_entry) == 0u,
               "A block must hold a whole number of directory entries.");

_Static_assert(MIMFS_BLOCK_SIZE_BYTES % MIMFS_BLOCK_POINTER_SIZE_BYTES == 0u,
               "A block must hold a whole number of block pointers.");

_Static_assert(sizeof((*(struct mimfs_inode *)0).block_map_root) ==
                   MIMFS_INLINE_SYMBOLIC_LINK_CAPACITY_BYTES,
               "Inline symbolic link capacity must equal the pointer array size.");

_Static_assert(MIMFS_MAXIMUM_FILE_NAME_LENGTH == 52u,
               "The name field size is derived from the cache line size; update "
               "the documented limit together with the entry header.");

/* =========================================================================
 * Region accessors
 * =========================================================================
 * Each of these turns a number into a pointer and does nothing else. They are
 * the only code in the filesystem that performs address arithmetic on the
 * volume, so a change to the region layout is confined to this block.
 */

/*
 * Reinterpret a byte address inside the volume as a typed object.
 *
 * Routing the conversion through void * is deliberate rather than lazy. It records
 * that the alignment is guaranteed by construction and not merely hoped for: the
 * layout engine starts every region on a cache line boundary, and every object
 * inside a region sits a whole multiple of its own size from that boundary, so a
 * typed access is always naturally aligned. The compiler cannot see that proof and a
 * direct cast would therefore be reported as an alignment increase. Confining the
 * conversion to this one macro means there is exactly one place to audit the claim.
 */
#define MIMFS_VIEW_BYTES_AS(pointer_type, byte_address)                                \
    ((pointer_type)(void *)(byte_address))

MIMFS_ALWAYS_INLINE unsigned char *
mimfs_volume_base_address(struct mimfs_volume *volume)
{
    return MIMFS_VIEW_BYTES_AS(unsigned char *, volume);
}

MIMFS_ALWAYS_INLINE unsigned char *mimfs_volume_region_at(struct mimfs_volume *volume,
                                                          uint64_t region_offset)
{
    return mimfs_volume_base_address(volume) + region_offset;
}

/* First byte of the data block whose volume-wide number is block_number. */
MIMFS_ALWAYS_INLINE unsigned char *mimfs_block_address(struct mimfs_volume *volume,
                                                       uint64_t block_number)
{
    return mimfs_volume_region_at(volume, (*volume).data_block_region_offset) +
           (block_number << MIMFS_BLOCK_SIZE_LOG2);
}

/* The block reinterpreted as an array of block pointers, for indirect blocks. */
MIMFS_ALWAYS_INLINE uint32_t *mimfs_block_as_pointer_array(struct mimfs_volume *volume,
                                                           uint64_t block_number)
{
    return MIMFS_VIEW_BYTES_AS(uint32_t *, mimfs_block_address(volume, block_number));
}

/* The block reinterpreted as an array of directory entry slots. */
MIMFS_ALWAYS_INLINE struct mimfs_directory_entry *
mimfs_block_as_directory_entry_array(struct mimfs_volume *volume, uint64_t block_number)
{
    return MIMFS_VIEW_BYTES_AS(struct mimfs_directory_entry *,
                               mimfs_block_address(volume, block_number));
}

/*
 * The inode with the given volume-wide number. Inode numbering starts at one,
 * so the table index is one less; number zero has no slot at all, which is what
 * makes zero usable as the "no inode" sentinel everywhere else.
 */
MIMFS_ALWAYS_INLINE struct mimfs_inode *mimfs_inode_at(struct mimfs_volume *volume,
                                                       uint64_t inode_number)
{
    struct mimfs_inode *inode_table = MIMFS_VIEW_BYTES_AS(
        struct mimfs_inode *,
        mimfs_volume_region_at(volume, (*volume).inode_table_offset));

    return inode_table + (inode_number - 1u);
}

MIMFS_ALWAYS_INLINE struct mimfs_block_group_descriptor *
mimfs_block_group_descriptor_at(struct mimfs_volume *volume, uint64_t group_index)
{
    struct mimfs_block_group_descriptor *table = MIMFS_VIEW_BYTES_AS(
        struct mimfs_block_group_descriptor *,
        mimfs_volume_region_at(volume, (*volume).block_group_descriptor_table_offset));

    return table + group_index;
}

MIMFS_ALWAYS_INLINE _Atomic uint64_t *
mimfs_block_allocation_bitmap(struct mimfs_volume *volume)
{
    return MIMFS_VIEW_BYTES_AS(
        _Atomic uint64_t *,
        mimfs_volume_region_at(volume, (*volume).block_allocation_bitmap_offset));
}

MIMFS_ALWAYS_INLINE _Atomic uint64_t *
mimfs_inode_allocation_bitmap(struct mimfs_volume *volume)
{
    return MIMFS_VIEW_BYTES_AS(
        _Atomic uint64_t *,
        mimfs_volume_region_at(volume, (*volume).inode_allocation_bitmap_offset));
}

/*
 * The descriptor and description allocation bitmaps live back to back in one region.
 * The descriptor count is a multiple of the bitmap word width, so the second bitmap
 * begins on a word boundary and each can be treated as an independent bitmap with
 * its own local indices.
 */
MIMFS_ALWAYS_INLINE _Atomic uint64_t *
mimfs_descriptor_allocation_bitmap(struct mimfs_volume *volume)
{
    return MIMFS_VIEW_BYTES_AS(
        _Atomic uint64_t *,
        mimfs_volume_region_at(volume, (*volume).descriptor_allocation_bitmap_offset));
}

MIMFS_ALWAYS_INLINE _Atomic uint64_t *
mimfs_open_file_description_allocation_bitmap(struct mimfs_volume *volume)
{
    return mimfs_descriptor_allocation_bitmap(volume) +
           ((*volume).maximum_open_file_count / MIMFS_BITMAP_WORD_BIT_COUNT);
}

MIMFS_ALWAYS_INLINE struct mimfs_open_file_description *
mimfs_open_file_description_at(struct mimfs_volume *volume, uint64_t description_index)
{
    struct mimfs_open_file_description *table = MIMFS_VIEW_BYTES_AS(
        struct mimfs_open_file_description *,
        mimfs_volume_region_at(volume, (*volume).open_file_description_table_offset));

    return table + description_index;
}

MIMFS_ALWAYS_INLINE struct mimfs_descriptor_table_entry *
mimfs_descriptor_table_entry_at(struct mimfs_volume *volume, uint64_t descriptor)
{
    struct mimfs_descriptor_table_entry *table = MIMFS_VIEW_BYTES_AS(
        struct mimfs_descriptor_table_entry *,
        mimfs_volume_region_at(volume, (*volume).descriptor_table_offset));

    return table + descriptor;
}

/* =========================================================================
 * Numbering helpers
 * ========================================================================= */

/* Group that owns a block. One shift, because the group span is a power of two. */
MIMFS_ALWAYS_INLINE uint32_t mimfs_group_index_of_block(struct mimfs_volume *volume,
                                                        uint64_t block_number)
{
    return (uint32_t)(block_number >> (*volume).blocks_per_block_group_log2);
}

/* Group that owns an inode. Inode numbering starts at one, so the number is
 * biased down before the shift. */
MIMFS_ALWAYS_INLINE uint32_t mimfs_group_index_of_inode(struct mimfs_volume *volume,
                                                        uint64_t inode_number)
{
    return (uint32_t)((inode_number - 1u) >> (*volume).inodes_per_block_group_log2);
}

/* True when a block number could name a block of this volume. */
MIMFS_ALWAYS_INLINE bool mimfs_block_number_is_in_range(struct mimfs_volume *volume,
                                                        uint64_t block_number)
{
    return block_number != MIMFS_UNALLOCATED_BLOCK_NUMBER &&
           block_number < (*volume).total_block_count;
}

/* True when an inode number could name an inode of this volume. */
MIMFS_ALWAYS_INLINE bool mimfs_inode_number_is_in_range(struct mimfs_volume *volume,
                                                        uint64_t inode_number)
{
    return inode_number >= MIMFS_ROOT_INODE_NUMBER &&
           inode_number <= (*volume).total_inode_count;
}

/* True when the handle refers to a mounted volume of a format this build
 * understands. Every public entry point starts with this check. */
MIMFS_ALWAYS_INLINE bool mimfs_volume_is_usable(struct mimfs_volume *volume)
{
    if (MIMFS_UNLIKELY(volume == NULL))
    {
        return false;
    }

    if (MIMFS_UNLIKELY((*volume).magic_number != MIMFS_VOLUME_MAGIC_NUMBER))
    {
        return false;
    }

    if (MIMFS_UNLIKELY((*volume).format_revision != MIMFS_FORMAT_REVISION))
    {
        return false;
    }

    return atomic_load_explicit(&(*volume).mount_state, memory_order_acquire) ==
           MIMFS_MOUNT_STATE_MOUNTED;
}

#endif /* MIMFS_LAYOUT_H */
