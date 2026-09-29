/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    tests/mimfs_test_directory.c
 * Purpose: Cases covering directory creation, listing, growth and removal.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_test_harness.h"

#include <string.h>

/* Count the entries of a directory, excluding "." and "..". */
static unsigned int mimfs_test_count_directory_entries(struct mimfs_volume *volume,
                                                       const char *path)
{
    const int descriptor = mimfs_open_directory(volume, path);

    if (descriptor < 0)
    {
        return 0u;
    }

    unsigned int entry_count = 0u;
    struct mimfs_directory_entry_information entry;

    while (mimfs_read_directory(volume, descriptor, &entry) == 1)
    {
        if (strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0)
        {
            entry_count++;
        }
    }

    (void)mimfs_close_directory(volume, descriptor);

    return entry_count;
}

static void mimfs_test_directory_creation_and_nesting(void)
{
    mimfs_test_begin_case("directories nest and report the expected link counts");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/one", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/one/two", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/one/two/three", 0700),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_status_information status;

    /* An empty directory has two links: its own "." and its name in its parent. */
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/one/two/three", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(MIMFS_MODE_IS_DIRECTORY(status.file_mode));
    MIMFS_CHECK_EQUAL(status.hard_link_count, 2);
    MIMFS_CHECK_EQUAL(status.file_mode & MIMFS_PERMISSION_MASK, 0700);

    /* One subdirectory raises the parent's count by one, for the child's "..". */
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/one/two", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.hard_link_count, 3);

    /* Creating the same directory twice is refused. */
    MIMFS_CHECK_STATUS(mimfs_create_directory(volume, "/one", 0755),
                       MIMFS_STATUS_ALREADY_EXISTS);

    /* A path whose middle component does not exist. */
    MIMFS_CHECK_STATUS(mimfs_create_directory(volume, "/absent/child", 0755),
                       MIMFS_STATUS_NO_SUCH_ENTRY);

    printf("ok\n");
}

static void mimfs_test_path_resolution_oddities(void)
{
    mimfs_test_begin_case("redundant separators, dot and dot dot resolve correctly");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/alpha", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/alpha/beta", 0755),
                      MIMFS_STATUS_SUCCESS);

    const int descriptor = mimfs_open(volume, "/alpha/beta/file",
                                      MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    struct mimfs_status_information direct_status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/alpha/beta/file", &direct_status),
                      MIMFS_STATUS_SUCCESS);

    /* Every one of these names the same file. */
    const char *equivalent_paths[] = {
        "///alpha//beta///file", "/alpha/./beta/./file", "/alpha/beta/../beta/file",
        "/alpha/../alpha/beta/file",
        "/../alpha/beta/file" /* the root's parent is the root */
    };

    const size_t path_count = sizeof(equivalent_paths) / sizeof(equivalent_paths[0]);

    for (size_t path_index = 0u; path_index < path_count; path_index++)
    {
        struct mimfs_status_information status;

        MIMFS_CHECK_EQUAL(
            mimfs_get_status(volume, *(equivalent_paths + path_index), &status),
            MIMFS_STATUS_SUCCESS);
        MIMFS_CHECK_EQUAL(status.inode_number, direct_status.inode_number);
    }

    /* A trailing separator asserts that the target is a directory. */
    MIMFS_CHECK_STATUS(mimfs_get_status(volume, "/alpha/beta/file/", &direct_status),
                       MIMFS_STATUS_NOT_A_DIRECTORY);

    struct mimfs_status_information directory_status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/alpha/beta/", &directory_status),
                      MIMFS_STATUS_SUCCESS);

    /* And a file cannot be a path component. */
    MIMFS_CHECK_STATUS(
        mimfs_get_status(volume, "/alpha/beta/file/deeper", &directory_status),
        MIMFS_STATUS_NOT_A_DIRECTORY);

    printf("ok\n");
}

static void mimfs_test_directory_removal(void)
{
    mimfs_test_begin_case("only empty directories can be removed");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/parent", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/parent/child", 0755),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_STATUS(mimfs_remove_directory(volume, "/parent"),
                       MIMFS_STATUS_DIRECTORY_NOT_EMPTY);

    MIMFS_CHECK_EQUAL(mimfs_remove_directory(volume, "/parent/child"),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/parent", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.hard_link_count, 2);

    MIMFS_CHECK_EQUAL(mimfs_remove_directory(volume, "/parent"), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_STATUS(mimfs_get_status(volume, "/parent", &status),
                       MIMFS_STATUS_NO_SUCH_ENTRY);

    /* A directory holding a file is not empty either. */
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/occupied", 0755),
                      MIMFS_STATUS_SUCCESS);
    const int descriptor = mimfs_open(volume, "/occupied/f",
                                      MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_STATUS(mimfs_remove_directory(volume, "/occupied"),
                       MIMFS_STATUS_DIRECTORY_NOT_EMPTY);

    /* The reserved names may not be removed. */
    MIMFS_CHECK_STATUS(mimfs_remove_directory(volume, "/occupied/."),
                       MIMFS_STATUS_INVALID_ARGUMENT);
    MIMFS_CHECK_STATUS(mimfs_remove_directory(volume, "/occupied/.."),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    /* Nor may the root. */
    MIMFS_CHECK_STATUS(mimfs_remove_directory(volume, "/"),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    /* A file is not removed by the directory removal operation. */
    MIMFS_CHECK_STATUS(mimfs_remove_directory(volume, "/occupied/f"),
                       MIMFS_STATUS_NOT_A_DIRECTORY);

    /* And a directory is not removed by the link removal operation. */
    MIMFS_CHECK_STATUS(mimfs_remove_link(volume, "/occupied"),
                       MIMFS_STATUS_IS_A_DIRECTORY);

    printf("ok\n");
}

static void mimfs_test_directory_grows_beyond_one_block(void)
{
    mimfs_test_begin_case("a directory grows past one block and lists everything");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/many", 0755),
                      MIMFS_STATUS_SUCCESS);

    /* One block holds sixty four entries, two of which are the dot entries, so this
     * count spans four blocks and exercises the growth path repeatedly. */
    const unsigned int entry_count_to_create = 250u;

    for (unsigned int index = 0u; index < entry_count_to_create; index++)
    {
        char path[80];
        snprintf(path, sizeof(path), "/many/entry-%04u", index);

        const int descriptor =
            mimfs_open(volume, path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
        MIMFS_CHECK(descriptor >= 0);
        MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);
    }

    MIMFS_CHECK_EQUAL(mimfs_test_count_directory_entries(volume, "/many"),
                      entry_count_to_create);

    /* Every one of them must still resolve. */
    for (unsigned int index = 0u; index < entry_count_to_create; index++)
    {
        char path[80];
        snprintf(path, sizeof(path), "/many/entry-%04u", index);

        struct mimfs_status_information status;
        MIMFS_CHECK_EQUAL(mimfs_get_status(volume, path, &status),
                          MIMFS_STATUS_SUCCESS);
    }

    /* Removing them all must shrink the directory back down and leave it removable. */
    for (unsigned int index = 0u; index < entry_count_to_create; index++)
    {
        char path[80];
        snprintf(path, sizeof(path), "/many/entry-%04u", index);
        MIMFS_CHECK_EQUAL(mimfs_remove_link(volume, path), MIMFS_STATUS_SUCCESS);
    }

    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/many", &status), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.file_size_in_bytes, 4096);

    MIMFS_CHECK_EQUAL(mimfs_remove_directory(volume, "/many"), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_directory_stream_behaviour(void)
{
    mimfs_test_begin_case("directory streams report types and can be rewound");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/mixed", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/mixed/subdirectory", 0755),
                      MIMFS_STATUS_SUCCESS);

    const int descriptor = mimfs_open(volume, "/mixed/regular",
                                      MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "regular", "/mixed/link"),
                      MIMFS_STATUS_SUCCESS);

    const int stream = mimfs_open_directory(volume, "/mixed");
    MIMFS_CHECK(stream >= 0);

    unsigned int directory_count = 0u;
    unsigned int regular_count = 0u;
    unsigned int link_count = 0u;

    struct mimfs_directory_entry_information entry;

    while (mimfs_read_directory(volume, stream, &entry) == 1)
    {
        MIMFS_CHECK_EQUAL(entry.name_length, strlen(entry.name));

        switch (entry.entry_type)
        {
        case MIMFS_ENTRY_TYPE_DIRECTORY:
            directory_count++;
            break;
        case MIMFS_ENTRY_TYPE_REGULAR:
            regular_count++;
            break;
        case MIMFS_ENTRY_TYPE_SYMBOLIC_LINK:
            link_count++;
            break;
        default:
            MIMFS_CHECK(false);
            break;
        }
    }

    /* ".", "..", and the subdirectory. */
    MIMFS_CHECK_EQUAL(directory_count, 3);
    MIMFS_CHECK_EQUAL(regular_count, 1);
    MIMFS_CHECK_EQUAL(link_count, 1);

    /* The stream is exhausted, and rewinding restarts it. */
    MIMFS_CHECK_EQUAL(mimfs_read_directory(volume, stream, &entry), 0);
    MIMFS_CHECK_EQUAL(mimfs_rewind_directory(volume, stream), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_read_directory(volume, stream, &entry), 1);

    /* Byte transfers are not how a directory is read. */
    char read_buffer[16];
    MIMFS_CHECK_STATUS(mimfs_read(volume, stream, read_buffer, sizeof(read_buffer)),
                       MIMFS_STATUS_IS_A_DIRECTORY);

    MIMFS_CHECK_EQUAL(mimfs_close_directory(volume, stream), MIMFS_STATUS_SUCCESS);

    /* Nor is a directory stream how a file is read. */
    const int file_descriptor =
        mimfs_open(volume, "/mixed/regular", MIMFS_OPEN_READ_ONLY, 0);
    MIMFS_CHECK(file_descriptor >= 0);
    MIMFS_CHECK_STATUS(mimfs_read_directory(volume, file_descriptor, &entry),
                       MIMFS_STATUS_NOT_A_DIRECTORY);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, file_descriptor), MIMFS_STATUS_SUCCESS);

    /* And opening a file as a directory is refused. */
    MIMFS_CHECK_STATUS(mimfs_open_directory(volume, "/mixed/regular"),
                       MIMFS_STATUS_NOT_A_DIRECTORY);

    printf("ok\n");
}

static void mimfs_test_name_limits(void)
{
    mimfs_test_begin_case("name and path length limits are enforced exactly");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    char longest_valid_path[2u + MIMFS_MAXIMUM_FILE_NAME_LENGTH];
    *(longest_valid_path + 0) = '/';
    memset(longest_valid_path + 1, 'n', MIMFS_MAXIMUM_FILE_NAME_LENGTH);
    *(longest_valid_path + 1 + MIMFS_MAXIMUM_FILE_NAME_LENGTH) = '\0';

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, longest_valid_path, 0755),
                      MIMFS_STATUS_SUCCESS);

    /* Which must be findable by the same name it was created with. */
    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, longest_valid_path, &status),
                      MIMFS_STATUS_SUCCESS);

    char one_character_too_long[3u + MIMFS_MAXIMUM_FILE_NAME_LENGTH];
    *(one_character_too_long + 0) = '/';
    memset(one_character_too_long + 1, 'n', MIMFS_MAXIMUM_FILE_NAME_LENGTH + 1u);
    *(one_character_too_long + 2 + MIMFS_MAXIMUM_FILE_NAME_LENGTH) = '\0';

    MIMFS_CHECK_STATUS(mimfs_create_directory(volume, one_character_too_long, 0755),
                       MIMFS_STATUS_NAME_TOO_LONG);

    /* A path longer than the resolver accepts. */
    static char enormous_path[MIMFS_MAXIMUM_PATH_LENGTH + 16u];
    memset(enormous_path, 'p', sizeof(enormous_path) - 1u);
    *(enormous_path + 0) = '/';
    *(enormous_path + sizeof(enormous_path) - 1u) = '\0';

    MIMFS_CHECK_STATUS(mimfs_get_status(volume, enormous_path, &status),
                       MIMFS_STATUS_NAME_TOO_LONG);

    /* Names that differ only past the length limit must not collide. */
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/short", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/shorter", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/short", &status),
                      MIMFS_STATUS_SUCCESS);
    const uint32_t short_inode_number = status.inode_number;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/shorter", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(status.inode_number != short_inode_number);

    printf("ok\n");
}

void mimfs_test_run_directory_cases(void)
{
    printf("directories\n");

    mimfs_test_directory_creation_and_nesting();
    mimfs_test_path_resolution_oddities();
    mimfs_test_directory_removal();
    mimfs_test_directory_grows_beyond_one_block();
    mimfs_test_directory_stream_behaviour();
    mimfs_test_name_limits();
}
