/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    tests/mimfs_test_file.c
 * Purpose: Cases covering descriptors, transfers, offsets, holes and truncation.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_test_harness.h"

#include <string.h>

static void mimfs_test_open_flag_validation(void)
{
    mimfs_test_begin_case("open rejects contradictory and unknown flags");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    /* Exclusive without create says nothing. */
    MIMFS_CHECK_STATUS(mimfs_open(volume, "/f", MIMFS_OPEN_EXCLUSIVE, 0644),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    /* Truncating without the ability to write. */
    MIMFS_CHECK_STATUS(
        mimfs_open(volume, "/f", MIMFS_OPEN_READ_ONLY | MIMFS_OPEN_TRUNCATE, 0644),
        MIMFS_STATUS_INVALID_ARGUMENT);

    /* A directory opened for writing. */
    MIMFS_CHECK_STATUS(
        mimfs_open(volume, "/", MIMFS_OPEN_DIRECTORY | MIMFS_OPEN_READ_WRITE, 0),
        MIMFS_STATUS_INVALID_ARGUMENT);

    /* A bit this build does not define. */
    MIMFS_CHECK_STATUS(mimfs_open(volume, "/f", 0x80000000u, 0644),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    /* Opening something that is not there, without asking for it to be created. */
    MIMFS_CHECK_STATUS(mimfs_open(volume, "/absent", MIMFS_OPEN_READ_ONLY, 0),
                       MIMFS_STATUS_NO_SUCH_ENTRY);

    /* A relative path, which this filesystem has no working directory to resolve. */
    MIMFS_CHECK_STATUS(mimfs_open(volume, "relative", MIMFS_OPEN_READ_ONLY, 0),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    printf("ok\n");
}

static void mimfs_test_exclusive_creation(void)
{
    mimfs_test_begin_case("exclusive creation succeeds once and then refuses");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const unsigned int flags =
        MIMFS_OPEN_CREATE | MIMFS_OPEN_EXCLUSIVE | MIMFS_OPEN_READ_WRITE;

    const int first_descriptor = mimfs_open(volume, "/only-once", flags, 0644);
    MIMFS_CHECK(first_descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, first_descriptor), MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_STATUS(mimfs_open(volume, "/only-once", flags, 0644),
                       MIMFS_STATUS_ALREADY_EXISTS);

    printf("ok\n");
}

static void mimfs_test_sequential_transfer_round_trip(void)
{
    mimfs_test_begin_case("bytes written sequentially read back identically");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    /*
     * Deliberately larger than the direct pointers can address and not a multiple of
     * the block size, so that the first block, a full run of middle blocks, a partial
     * tail block and the single indirect tree are all exercised by one transfer.
     */
    static char written_payload[300u * 1024u + 137u];
    static char read_payload[sizeof(written_payload)];

    for (size_t index = 0u; index < sizeof(written_payload); index++)
    {
        written_payload[index] = (char)(index * 31u + (index >> 8));
    }

    const int descriptor =
        mimfs_open(volume, "/payload", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);

    MIMFS_CHECK_EQUAL(
        mimfs_write(volume, descriptor, written_payload, sizeof(written_payload)),
        (long)sizeof(written_payload));

    MIMFS_CHECK_EQUAL(mimfs_seek(volume, descriptor, 0, MIMFS_SEEK_FROM_START), 0);

    memset(read_payload, 0, sizeof(read_payload));
    MIMFS_CHECK_EQUAL(
        mimfs_read(volume, descriptor, read_payload, sizeof(read_payload)),
        (long)sizeof(read_payload));

    MIMFS_CHECK(memcmp(written_payload, read_payload, sizeof(written_payload)) == 0);

    /* And nothing beyond the end. */
    char trailing_byte = 0x5A;
    MIMFS_CHECK_EQUAL(mimfs_read(volume, descriptor, &trailing_byte, 1), 0);
    MIMFS_CHECK_EQUAL(trailing_byte, 0x5A);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_transfers_crossing_every_indirection_level(void)
{
    mimfs_test_begin_case("a write past the double indirect boundary is readable");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int descriptor =
        mimfs_open(volume, "/deep", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);

    /*
     * The offsets chosen are the first byte of the region each indirection level
     * addresses, so a mistake in the index decomposition shows up as a mismatch at one
     * specific level rather than as a general failure.
     */
    const uint64_t block_size = 4096u;
    const uint64_t offsets_to_probe[] = {
        0u,                                        /* direct slot 0 */
        11u * block_size,                          /* the last direct slot */
        12u * block_size,                          /* first single indirect block */
        (12u + 1023u) * block_size,                /* last single indirect block */
        (12u + 1024u) * block_size,                /* first double indirect block */
        (12u + 1024u + 1024u) * block_size,        /* second row of the double tree */
        (12u + 1024u + 1024u * 1024u) * block_size /* first triple indirect block */
    };

    const size_t probe_count = sizeof(offsets_to_probe) / sizeof(offsets_to_probe[0]);

    for (size_t probe_index = 0u; probe_index < probe_count; probe_index++)
    {
        char marker[16];

        /* The whole buffer is written to the file, so the whole buffer has to be
         * defined; the formatted text alone leaves its tail untouched. */
        memset(marker, 0, sizeof(marker));
        snprintf(marker, sizeof(marker), "probe-%02zu", probe_index);

        MIMFS_CHECK_EQUAL(mimfs_write_at_offset(volume, descriptor, marker,
                                                sizeof(marker),
                                                *(offsets_to_probe + probe_index)),
                          (long)sizeof(marker));
    }

    for (size_t probe_index = 0u; probe_index < probe_count; probe_index++)
    {
        char expected_marker[16];
        char actual_marker[16];

        memset(expected_marker, 0, sizeof(expected_marker));
        snprintf(expected_marker, sizeof(expected_marker), "probe-%02zu", probe_index);
        memset(actual_marker, 0, sizeof(actual_marker));

        MIMFS_CHECK_EQUAL(mimfs_read_at_offset(volume, descriptor, actual_marker,
                                               sizeof(actual_marker),
                                               *(offsets_to_probe + probe_index)),
                          (long)sizeof(actual_marker));

        MIMFS_CHECK(memcmp(expected_marker, actual_marker, sizeof(actual_marker)) == 0);
    }

    /* Only the blocks actually touched may have been allocated, plus the indirect
     * blocks needed to reach them. A sparse file must not have become a dense one. */
    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status_by_descriptor(volume, descriptor, &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(status.allocated_block_count < 32u);
    MIMFS_CHECK(status.file_size_in_bytes > 4u * 1024u * 1024u * 1024u);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok (%llu blocks for a %llu byte file)\n",
           (unsigned long long)status.allocated_block_count,
           (unsigned long long)status.file_size_in_bytes);
}

static void mimfs_test_holes_read_as_zeroes(void)
{
    mimfs_test_begin_case("a hole reads as zeroes and consumes no block");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int descriptor =
        mimfs_open(volume, "/sparse", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);

    /* Write one byte a long way in, leaving everything before it a hole. */
    const uint64_t distant_offset = 1024u * 1024u;
    MIMFS_CHECK_EQUAL(mimfs_write_at_offset(volume, descriptor, "!", 1, distant_offset),
                      1);

    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status_by_descriptor(volume, descriptor, &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.file_size_in_bytes, distant_offset + 1u);

    /* One data block for the byte, plus one indirect block to reach it. */
    MIMFS_CHECK(status.allocated_block_count <= 4u);

    static char read_buffer[8192];
    memset(read_buffer, 0xFF, sizeof(read_buffer));

    MIMFS_CHECK_EQUAL(
        mimfs_read_at_offset(volume, descriptor, read_buffer, sizeof(read_buffer), 0u),
        (long)sizeof(read_buffer));

    for (size_t index = 0u; index < sizeof(read_buffer); index++)
    {
        if (*(read_buffer + index) != 0)
        {
            MIMFS_CHECK(false);
            break;
        }
    }

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_seek_behaviour(void)
{
    mimfs_test_begin_case("seek from start, current and end all land correctly");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int descriptor = mimfs_open(volume, "/seekable",
                                      MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);

    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, "0123456789", 10), 10);

    MIMFS_CHECK_EQUAL(mimfs_seek(volume, descriptor, 3, MIMFS_SEEK_FROM_START), 3);
    MIMFS_CHECK_EQUAL(mimfs_seek(volume, descriptor, 2, MIMFS_SEEK_FROM_CURRENT), 5);
    MIMFS_CHECK_EQUAL(mimfs_seek(volume, descriptor, -2, MIMFS_SEEK_FROM_END), 8);
    MIMFS_CHECK_EQUAL(mimfs_seek(volume, descriptor, 0, MIMFS_SEEK_FROM_END), 10);

    char read_buffer[4];
    MIMFS_CHECK_EQUAL(mimfs_seek(volume, descriptor, 4, MIMFS_SEEK_FROM_START), 4);
    MIMFS_CHECK_EQUAL(mimfs_read(volume, descriptor, read_buffer, 3), 3);
    MIMFS_CHECK(memcmp(read_buffer, "456", 3) == 0);

    /* Seeking before the start of a file names no byte. */
    MIMFS_CHECK_STATUS(mimfs_seek(volume, descriptor, -1, MIMFS_SEEK_FROM_START),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    /* An unrecognised origin. */
    MIMFS_CHECK_STATUS(mimfs_seek(volume, descriptor, 0, 99),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_append_always_lands_at_the_end(void)
{
    mimfs_test_begin_case("append writes at the end regardless of the offset");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int descriptor =
        mimfs_open(volume, "/log",
                   MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE | MIMFS_OPEN_APPEND, 0644);
    MIMFS_CHECK(descriptor >= 0);

    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, "first", 5), 5);

    /* Rewinding must not affect where an appending write goes. */
    MIMFS_CHECK_EQUAL(mimfs_seek(volume, descriptor, 0, MIMFS_SEEK_FROM_START), 0);
    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, "second", 6), 6);

    char read_buffer[16];
    memset(read_buffer, 0, sizeof(read_buffer));
    MIMFS_CHECK_EQUAL(mimfs_read_at_offset(volume, descriptor, read_buffer, 11, 0u),
                      11);
    MIMFS_CHECK(memcmp(read_buffer, "firstsecond", 11) == 0);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_truncation(void)
{
    mimfs_test_begin_case("truncation shortens, extends and zeroes correctly");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int descriptor =
        mimfs_open(volume, "/trimmed", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);

    static char payload[40u * 1024u];
    memset(payload, 'F', sizeof(payload));

    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, payload, sizeof(payload)),
                      (long)sizeof(payload));

    struct mimfs_status_information before_status;
    MIMFS_CHECK_EQUAL(
        mimfs_get_status_by_descriptor(volume, descriptor, &before_status),
        MIMFS_STATUS_SUCCESS);

    /* Shorten to a length that is not a whole number of blocks. */
    const uint64_t shortened_length = 5000u;
    MIMFS_CHECK_EQUAL(
        mimfs_truncate_by_descriptor(volume, descriptor, shortened_length),
        MIMFS_STATUS_SUCCESS);

    struct mimfs_status_information after_status;
    MIMFS_CHECK_EQUAL(mimfs_get_status_by_descriptor(volume, descriptor, &after_status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(after_status.file_size_in_bytes, shortened_length);
    MIMFS_CHECK(after_status.allocated_block_count <
                before_status.allocated_block_count);

    /* Grow it again. The part beyond the old end must read as zeroes, not as the
     * bytes that used to be there. */
    MIMFS_CHECK_EQUAL(mimfs_truncate_by_descriptor(volume, descriptor, 8192u),
                      MIMFS_STATUS_SUCCESS);

    static char read_buffer[8192];
    memset(read_buffer, 0x11, sizeof(read_buffer));
    MIMFS_CHECK_EQUAL(mimfs_read_at_offset(volume, descriptor, read_buffer, 8192u, 0u),
                      8192);

    for (size_t index = 0u; index < shortened_length; index++)
    {
        if (*(read_buffer + index) != 'F')
        {
            MIMFS_CHECK(false);
            break;
        }
    }

    for (size_t index = shortened_length; index < 8192u; index++)
    {
        if (*(read_buffer + index) != 0)
        {
            MIMFS_CHECK(false);
            break;
        }
    }

    /* Truncating to zero releases everything. */
    MIMFS_CHECK_EQUAL(mimfs_truncate_by_descriptor(volume, descriptor, 0u),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status_by_descriptor(volume, descriptor, &after_status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(after_status.file_size_in_bytes, 0);
    MIMFS_CHECK_EQUAL(after_status.allocated_block_count, 0);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_open_with_truncate_discards_content(void)
{
    mimfs_test_begin_case("opening with truncate discards the previous content");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    int descriptor =
        mimfs_open(volume, "/reset", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, "stale content", 13), 13);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    descriptor =
        mimfs_open(volume, "/reset", MIMFS_OPEN_READ_WRITE | MIMFS_OPEN_TRUNCATE, 0);
    MIMFS_CHECK(descriptor >= 0);

    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status_by_descriptor(volume, descriptor, &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.file_size_in_bytes, 0);

    char read_buffer[8];
    MIMFS_CHECK_EQUAL(mimfs_read(volume, descriptor, read_buffer, sizeof(read_buffer)),
                      0);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_access_mode_is_enforced(void)
{
    mimfs_test_begin_case("a descriptor refuses the access it was not opened for");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    int descriptor =
        mimfs_open(volume, "/guarded", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, "content", 7), 7);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    descriptor = mimfs_open(volume, "/guarded", MIMFS_OPEN_READ_ONLY, 0);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_STATUS(mimfs_write(volume, descriptor, "denied", 6),
                       MIMFS_STATUS_BAD_DESCRIPTOR);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    descriptor = mimfs_open(volume, "/guarded", MIMFS_OPEN_WRITE_ONLY, 0);
    MIMFS_CHECK(descriptor >= 0);

    char read_buffer[8];
    MIMFS_CHECK_STATUS(mimfs_read(volume, descriptor, read_buffer, sizeof(read_buffer)),
                       MIMFS_STATUS_BAD_DESCRIPTOR);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_descriptor_validity(void)
{
    mimfs_test_begin_case("closed and out of range descriptors are refused");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    char read_buffer[8];

    MIMFS_CHECK_STATUS(mimfs_read(volume, -1, read_buffer, sizeof(read_buffer)),
                       MIMFS_STATUS_BAD_DESCRIPTOR);
    MIMFS_CHECK_STATUS(mimfs_read(volume, 100000, read_buffer, sizeof(read_buffer)),
                       MIMFS_STATUS_BAD_DESCRIPTOR);
    MIMFS_CHECK_STATUS(mimfs_close(volume, 5), MIMFS_STATUS_BAD_DESCRIPTOR);

    const int descriptor =
        mimfs_open(volume, "/once", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    /* Closing twice is refused rather than releasing anything a second time. */
    MIMFS_CHECK_STATUS(mimfs_close(volume, descriptor), MIMFS_STATUS_BAD_DESCRIPTOR);
    MIMFS_CHECK_STATUS(mimfs_read(volume, descriptor, read_buffer, sizeof(read_buffer)),
                       MIMFS_STATUS_BAD_DESCRIPTOR);

    printf("ok\n");
}

static void mimfs_test_duplicated_descriptors_share_one_offset(void)
{
    mimfs_test_begin_case("duplicated descriptors share one file offset");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int first_descriptor =
        mimfs_open(volume, "/shared", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(first_descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_write(volume, first_descriptor, "abcdefgh", 8), 8);

    const int second_descriptor = mimfs_duplicate_descriptor(volume, first_descriptor);
    MIMFS_CHECK(second_descriptor >= 0);
    MIMFS_CHECK(second_descriptor != first_descriptor);

    MIMFS_CHECK_EQUAL(mimfs_seek(volume, first_descriptor, 2, MIMFS_SEEK_FROM_START),
                      2);

    /* The seek through one descriptor must be visible through the other. */
    char read_buffer[4];
    MIMFS_CHECK_EQUAL(mimfs_read(volume, second_descriptor, read_buffer, 3), 3);
    MIMFS_CHECK(memcmp(read_buffer, "cde", 3) == 0);

    /* And the offset advanced by that read must be visible through the first. */
    MIMFS_CHECK_EQUAL(mimfs_seek(volume, first_descriptor, 0, MIMFS_SEEK_FROM_CURRENT),
                      5);

    /* Closing one duplicate leaves the other fully usable. */
    MIMFS_CHECK_EQUAL(mimfs_close(volume, first_descriptor), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_read(volume, second_descriptor, read_buffer, 3), 3);
    MIMFS_CHECK(memcmp(read_buffer, "fgh", 3) == 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, second_descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_descriptor_table_exhaustion(void)
{
    mimfs_test_begin_case("the descriptor table reports exhaustion and recovers");

    struct mimfs_format_parameters parameters;
    parameters.bytes_per_inode_ratio = 4096u;
    parameters.maximum_open_file_count = 64u;
    parameters.reserved_block_percentage = 0u;
    parameters.blocks_per_block_group = 0u;

    struct mimfs_volume *volume = mimfs_test_create_volume_with(&parameters);
    MIMFS_CHECK(volume != NULL);

    if (volume == NULL)
    {
        return;
    }

    int descriptors[64];
    unsigned int opened_count = 0u;

    for (unsigned int index = 0u; index < 64u; index++)
    {
        const int descriptor =
            mimfs_open(volume, "/hot", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

        if (descriptor < 0)
        {
            MIMFS_CHECK_STATUS(descriptor, MIMFS_STATUS_TOO_MANY_OPEN_FILES);
            break;
        }

        *(descriptors + opened_count) = descriptor;
        opened_count++;
    }

    MIMFS_CHECK_EQUAL(opened_count, 64);

    MIMFS_CHECK_STATUS(mimfs_open(volume, "/hot", MIMFS_OPEN_READ_ONLY, 0),
                       MIMFS_STATUS_TOO_MANY_OPEN_FILES);

    for (unsigned int index = 0u; index < opened_count; index++)
    {
        MIMFS_CHECK_EQUAL(mimfs_close(volume, *(descriptors + index)),
                          MIMFS_STATUS_SUCCESS);
    }

    /* Every slot must be reusable afterwards. */
    const int reopened = mimfs_open(volume, "/hot", MIMFS_OPEN_READ_ONLY, 0);
    MIMFS_CHECK(reopened >= 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, reopened), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_removed_but_open_file_survives(void)
{
    mimfs_test_begin_case("a file removed while open stays readable until closed");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int descriptor =
        mimfs_open(volume, "/doomed", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, "still here", 10), 10);

    struct mimfs_volume_statistics before_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &before_statistics),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(mimfs_remove_link(volume, "/doomed"), MIMFS_STATUS_SUCCESS);

    /* The name is gone from the namespace at once. */
    struct mimfs_status_information status;
    MIMFS_CHECK_STATUS(mimfs_get_status(volume, "/doomed", &status),
                       MIMFS_STATUS_NO_SUCH_ENTRY);

    /* The content is not. */
    char read_buffer[16];
    memset(read_buffer, 0, sizeof(read_buffer));
    MIMFS_CHECK_EQUAL(mimfs_read_at_offset(volume, descriptor, read_buffer, 10, 0u),
                      10);
    MIMFS_CHECK(memcmp(read_buffer, "still here", 10) == 0);

    /* The blocks are still charged while the descriptor is open. */
    struct mimfs_volume_statistics during_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &during_statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(during_statistics.free_block_count,
                      before_statistics.free_block_count);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    /* And released the moment the last descriptor goes. */
    struct mimfs_volume_statistics after_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &after_statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(after_statistics.free_block_count > before_statistics.free_block_count);
    MIMFS_CHECK(after_statistics.free_inode_count > before_statistics.free_inode_count);

    printf("ok\n");
}

void mimfs_test_run_file_cases(void)
{
    printf("files\n");

    mimfs_test_open_flag_validation();
    mimfs_test_exclusive_creation();
    mimfs_test_sequential_transfer_round_trip();
    mimfs_test_transfers_crossing_every_indirection_level();
    mimfs_test_holes_read_as_zeroes();
    mimfs_test_seek_behaviour();
    mimfs_test_append_always_lands_at_the_end();
    mimfs_test_truncation();
    mimfs_test_open_with_truncate_discards_content();
    mimfs_test_access_mode_is_enforced();
    mimfs_test_descriptor_validity();
    mimfs_test_duplicated_descriptors_share_one_offset();
    mimfs_test_descriptor_table_exhaustion();
    mimfs_test_removed_but_open_file_survives();
}
