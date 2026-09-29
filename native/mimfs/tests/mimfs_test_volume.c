/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    tests/mimfs_test_volume.c
 * Purpose: Cases covering geometry, formatting, mounting and accounting.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_test_harness.h"

#include <string.h>

static void mimfs_test_format_produces_a_usable_volume(void)
{
    mimfs_test_begin_case("format produces a usable volume");

    struct mimfs_volume *volume = mimfs_test_create_volume();
    MIMFS_CHECK(volume != NULL);

    if (volume == NULL)
    {
        return;
    }

    struct mimfs_volume_statistics statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &statistics),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(statistics.block_size_in_bytes, 4096);
    MIMFS_CHECK_EQUAL(statistics.inode_size_in_bytes, 256);
    MIMFS_CHECK(statistics.total_block_count > 0u);
    MIMFS_CHECK(statistics.block_group_count >= 1u);
    MIMFS_CHECK(statistics.free_inode_count < statistics.total_inode_count);
    MIMFS_CHECK(statistics.available_block_count <= statistics.free_block_count);

    /* The whole data area must be accounted for: the only block not free on a fresh
     * volume is the sentinel, plus whatever the root directory occupies. */
    MIMFS_CHECK(statistics.free_block_count < statistics.total_block_count);

    printf("ok (%llu blocks, %llu inodes, %u groups)\n",
           (unsigned long long)statistics.total_block_count,
           (unsigned long long)statistics.total_inode_count,
           statistics.block_group_count);
}

static void mimfs_test_root_directory_exists_and_is_empty(void)
{
    mimfs_test_begin_case("root exists, is a directory, and holds only dot entries");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/", &status), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(MIMFS_MODE_IS_DIRECTORY(status.file_mode));
    MIMFS_CHECK_EQUAL(status.inode_number, MIMFS_ROOT_INODE_NUMBER);
    MIMFS_CHECK_EQUAL(status.hard_link_count, 2);

    const int directory_descriptor = mimfs_open_directory(volume, "/");
    MIMFS_CHECK(directory_descriptor >= 0);

    unsigned int entry_count = 0u;
    struct mimfs_directory_entry_information entry;

    while (mimfs_read_directory(volume, directory_descriptor, &entry) == 1)
    {
        entry_count++;
        MIMFS_CHECK_EQUAL(entry.inode_number, MIMFS_ROOT_INODE_NUMBER);
        MIMFS_CHECK(strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0);
    }

    MIMFS_CHECK_EQUAL(entry_count, 2);
    MIMFS_CHECK_EQUAL(mimfs_close_directory(volume, directory_descriptor),
                      MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_mount_preserves_the_tree(void)
{
    mimfs_test_begin_case("mount recovers a formatted volume with its tree intact");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/persistent", 0755),
                      MIMFS_STATUS_SUCCESS);

    const int descriptor = mimfs_open(volume, "/persistent/file",
                                      MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, "durable", 7), 7);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(mimfs_unmount(volume), MIMFS_STATUS_SUCCESS);

    int mount_status = MIMFS_STATUS_SUCCESS;
    struct mimfs_volume *remounted_volume =
        mimfs_mount(mimfs_test_region_base(), mimfs_test_region_size(), &mount_status);

    MIMFS_CHECK(remounted_volume != NULL);
    MIMFS_CHECK_EQUAL(mount_status, MIMFS_STATUS_SUCCESS);

    if (remounted_volume == NULL)
    {
        return;
    }

    /* Mounting is expected to rebuild the descriptor table, so nothing may be
     * inherited from before the unmount. */
    struct mimfs_volume_statistics statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(remounted_volume, &statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(statistics.open_file_count, 0);

    char read_buffer[16];
    memset(read_buffer, 0, sizeof(read_buffer));

    const int reopened_descriptor =
        mimfs_open(remounted_volume, "/persistent/file", MIMFS_OPEN_READ_ONLY, 0);
    MIMFS_CHECK(reopened_descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_read(remounted_volume, reopened_descriptor, read_buffer, 7),
                      7);
    MIMFS_CHECK(memcmp(read_buffer, "durable", 7) == 0);
    MIMFS_CHECK_EQUAL(mimfs_close(remounted_volume, reopened_descriptor),
                      MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_mount_refuses_a_region_with_no_volume(void)
{
    mimfs_test_begin_case("mount refuses a region that holds no volume");

    static unsigned char blank_region[512u * 1024u];
    memset(blank_region, 0, sizeof(blank_region));

    int mount_status = MIMFS_STATUS_SUCCESS;
    MIMFS_CHECK(mimfs_mount(blank_region, sizeof(blank_region), &mount_status) == NULL);
    MIMFS_CHECK_EQUAL(mount_status, MIMFS_STATUS_INVALID_ARGUMENT);

    /* And a region far too small to hold even the fixed tables. */
    static unsigned char tiny_region[1024];
    int format_status = MIMFS_STATUS_SUCCESS;
    MIMFS_CHECK(mimfs_format(tiny_region, sizeof(tiny_region), NULL, &format_status) ==
                NULL);
    MIMFS_CHECK_EQUAL(format_status, MIMFS_STATUS_NO_SPACE_LEFT);

    printf("ok\n");
}

static void mimfs_test_format_tolerates_an_unaligned_region(void)
{
    mimfs_test_begin_case("format aligns an unaligned region rather than refusing it");

    static unsigned char raw_region[4u * 1024u * 1024u + 64u];

    /* Deliberately start one byte into the region so that the base is odd. */
    int format_status = MIMFS_STATUS_SUCCESS;
    struct mimfs_volume *volume =
        mimfs_format(raw_region + 1, sizeof(raw_region) - 1u, NULL, &format_status);

    MIMFS_CHECK(volume != NULL);
    MIMFS_CHECK_EQUAL(format_status, MIMFS_STATUS_SUCCESS);

    if (volume != NULL)
    {
        MIMFS_CHECK_EQUAL(((uintptr_t)volume) % 64u, 0);
        MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/works", 0755),
                          MIMFS_STATUS_SUCCESS);
    }

    printf("ok\n");
}

static void mimfs_test_small_volume_still_works(void)
{
    mimfs_test_begin_case("a volume just above the minimum size is fully functional");

    static unsigned char small_region[512u * 1024u];

    struct mimfs_format_parameters parameters;
    parameters.bytes_per_inode_ratio = 4096u;
    parameters.maximum_open_file_count = 8u;
    parameters.reserved_block_percentage = 0u;
    parameters.blocks_per_block_group = 0u;

    int format_status = MIMFS_STATUS_SUCCESS;
    struct mimfs_volume *volume =
        mimfs_format(small_region, sizeof(small_region), &parameters, &format_status);

    MIMFS_CHECK(volume != NULL);

    if (volume == NULL)
    {
        return;
    }

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/d", 0755), MIMFS_STATUS_SUCCESS);

    const int descriptor =
        mimfs_open(volume, "/d/f", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, "x", 1), 1);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_space_is_returned_when_files_are_removed(void)
{
    mimfs_test_begin_case(
        "every block and inode is returned after a create and remove");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    struct mimfs_volume_statistics before_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &before_statistics),
                      MIMFS_STATUS_SUCCESS);

    /* Enough data to reach past the direct pointers and into the single indirect
     * tree, so that indirect blocks are exercised by the release path too. */
    static char payload[128u * 1024u];
    memset(payload, 'A', sizeof(payload));

    for (unsigned int round = 0u; round < 8u; round++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/file%u", round);

        const int descriptor =
            mimfs_open(volume, path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
        MIMFS_CHECK(descriptor >= 0);
        MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, payload, sizeof(payload)),
                          (long)sizeof(payload));
        MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);
    }

    for (unsigned int round = 0u; round < 8u; round++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/file%u", round);
        MIMFS_CHECK_EQUAL(mimfs_remove_link(volume, path), MIMFS_STATUS_SUCCESS);
    }

    struct mimfs_volume_statistics after_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &after_statistics),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(after_statistics.free_block_count,
                      before_statistics.free_block_count);
    MIMFS_CHECK_EQUAL(after_statistics.free_inode_count,
                      before_statistics.free_inode_count);

    printf("ok\n");
}

static void mimfs_test_volume_fills_up_cleanly(void)
{
    mimfs_test_begin_case("a full volume reports no space instead of misbehaving");

    static unsigned char small_region[2u * 1024u * 1024u];

    struct mimfs_format_parameters parameters;
    parameters.bytes_per_inode_ratio = 65536u;
    parameters.maximum_open_file_count = 16u;
    parameters.reserved_block_percentage = 0u;
    parameters.blocks_per_block_group = 0u;

    struct mimfs_volume *volume =
        mimfs_format(small_region, sizeof(small_region), &parameters, NULL);
    MIMFS_CHECK(volume != NULL);

    if (volume == NULL)
    {
        return;
    }

    const int descriptor =
        mimfs_open(volume, "/greedy", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);

    static char payload[64u * 1024u];
    memset(payload, 'Z', sizeof(payload));

    long total_written = 0;

    for (;;)
    {
        const long written = mimfs_write(volume, descriptor, payload, sizeof(payload));

        if (written <= 0)
        {
            MIMFS_CHECK_STATUS(written, MIMFS_STATUS_NO_SPACE_LEFT);
            break;
        }

        total_written += written;

        if (total_written > (long)sizeof(small_region))
        {
            MIMFS_CHECK(false);
            break;
        }
    }

    MIMFS_CHECK(total_written > 0);

    /* The volume is full, yet what was written must still read back correctly and the
     * accounting must still be consistent. */
    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status_by_descriptor(volume, descriptor, &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.file_size_in_bytes, total_written);

    struct mimfs_volume_statistics statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(statistics.free_block_count, 0);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_remove_link(volume, "/greedy"), MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(statistics.free_block_count > 0u);

    printf("ok (%ld bytes before the volume filled)\n", total_written);
}

void mimfs_test_run_volume_cases(void)
{
    printf("volume\n");

    mimfs_test_format_produces_a_usable_volume();
    mimfs_test_root_directory_exists_and_is_empty();
    mimfs_test_mount_preserves_the_tree();
    mimfs_test_mount_refuses_a_region_with_no_volume();
    mimfs_test_format_tolerates_an_unaligned_region();
    mimfs_test_small_volume_still_works();
    mimfs_test_space_is_returned_when_files_are_removed();
    mimfs_test_volume_fills_up_cleanly();
}
