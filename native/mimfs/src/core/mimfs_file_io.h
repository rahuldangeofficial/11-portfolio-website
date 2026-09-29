/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    src/core/mimfs_file_io.h
 * Purpose: Byte level transfer to and from the contents of one inode, and the
 *          setting of its length.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * This module knows about offsets, block boundaries and holes. It knows nothing
 * about descriptors, paths or permissions, which is what keeps it usable both for
 * file contents and for the contents of a directory.
 *
 * ---------------------------------------------------------------------------
 * Holes
 * ---------------------------------------------------------------------------
 * A logical block with no physical block behind it is a hole, and a hole reads
 * back as zeroes without consuming a single block. This is not a curiosity: it is
 * what makes seeking far past the end of a file and writing one byte there cost
 * one block rather than the whole span, and it is why a file's length and the
 * blocks charged to it are two independent numbers.
 */

#ifndef MIMFS_FILE_IO_H
#define MIMFS_FILE_IO_H

#include "mimfs_layout.h"

/*
 * Copy bytes out of a file.
 *
 * Reads that begin at or past the end of the file transfer nothing. A read that
 * starts inside the file and extends past its end is silently shortened to the end,
 * which is the only circumstance under which a short read occurs.
 *
 * Requires at least the shared inode lock.
 *
 * Returns the number of bytes transferred, or a negated status code.
 */
long mimfs_file_read_at_offset(struct mimfs_volume *volume, struct mimfs_inode *inode,
                               uint64_t file_offset, void *destination_buffer,
                               size_t byte_count);

/*
 * Copy bytes into a file, extending it when the write goes past its current end.
 *
 * A write that runs out of space part way through returns the number of bytes it
 * managed to place rather than failing outright, so that a caller can react to a
 * full volume without having to work out how much of its data survived. A write
 * that cannot place a single byte returns the error.
 *
 * Requires the exclusive inode lock.
 *
 * Returns the number of bytes transferred, or a negated status code.
 */
long mimfs_file_write_at_offset(struct mimfs_volume *volume, struct mimfs_inode *inode,
                                uint64_t file_offset, const void *source_buffer,
                                size_t byte_count);

/*
 * Set the length of a file.
 *
 * Growing installs a hole, so it costs nothing and consumes no block. Shrinking
 * releases the blocks beyond the new length and clears the unused tail of the block
 * that now holds the end of the file, so that a later regrowth reads zeroes there
 * rather than the bytes that used to follow.
 *
 * Requires the exclusive inode lock.
 */
int mimfs_file_set_size(struct mimfs_volume *volume, struct mimfs_inode *inode,
                        uint64_t new_size_in_bytes);

/*
 * Append one whole block to a file and report its number, used by the directory
 * layer when it needs a fresh page of entry slots. The block is cleared, which for
 * a directory block means every slot in it reads as free.
 *
 * Requires the exclusive inode lock.
 *
 * Returns MIMFS_STATUS_SUCCESS, or a positive status code.
 */
int mimfs_file_append_cleared_block(struct mimfs_volume *volume,
                                    struct mimfs_inode *inode,
                                    uint64_t *appended_block_number_out);

/*
 * Store the target text of a symbolic link.
 *
 * A target short enough to fit is written into the inode itself, where the block
 * pointers would otherwise be, and costs no block at all. Longer targets fall back
 * to ordinary file content.
 *
 * Requires the exclusive inode lock.
 */
int mimfs_file_store_symbolic_link_target(struct mimfs_volume *volume,
                                          struct mimfs_inode *inode,
                                          const char *target_text,
                                          size_t target_length);

/*
 * Read back the target text of a symbolic link, from wherever it is stored.
 *
 * Requires at least the shared inode lock.
 *
 * Returns the number of bytes copied, or a negated status code.
 */
long mimfs_file_load_symbolic_link_target(struct mimfs_volume *volume,
                                          struct mimfs_inode *inode,
                                          char *destination_buffer,
                                          size_t buffer_size_in_bytes);

#endif /* MIMFS_FILE_IO_H */
