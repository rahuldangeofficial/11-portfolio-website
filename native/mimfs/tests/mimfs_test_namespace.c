/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    tests/mimfs_test_namespace.c
 * Purpose: Cases covering hard links, symbolic links, renaming and metadata.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_test_harness.h"

#include <string.h>

/* Create a file holding the given text, and close it. */
static void mimfs_test_write_file(struct mimfs_volume *volume, const char *path,
                                  const char *text)
{
    const int descriptor =
        mimfs_open(volume, path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

    MIMFS_CHECK(descriptor >= 0);

    if (descriptor < 0)
    {
        return;
    }

    const size_t text_length = strlen(text);

    MIMFS_CHECK_EQUAL(mimfs_write(volume, descriptor, text, text_length),
                      (long)text_length);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);
}

/* Read a whole file back into a buffer, as a NUL terminated string. */
static void mimfs_test_read_file(struct mimfs_volume *volume, const char *path,
                                 char *buffer, size_t buffer_size)
{
    memset(buffer, 0, buffer_size);

    const int descriptor = mimfs_open(volume, path, MIMFS_OPEN_READ_ONLY, 0);

    MIMFS_CHECK(descriptor >= 0);

    if (descriptor < 0)
    {
        return;
    }

    const long bytes_read = mimfs_read(volume, descriptor, buffer, buffer_size - 1u);

    MIMFS_CHECK(bytes_read >= 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);
}

static void mimfs_test_hard_links(void)
{
    mimfs_test_begin_case("hard links share one inode and one content");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    mimfs_test_write_file(volume, "/original", "shared bytes");

    MIMFS_CHECK_EQUAL(mimfs_create_hard_link(volume, "/original", "/second-name"),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_status_information original_status;
    struct mimfs_status_information linked_status;

    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/original", &original_status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/second-name", &linked_status),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(original_status.inode_number, linked_status.inode_number);
    MIMFS_CHECK_EQUAL(original_status.hard_link_count, 2);

    char read_buffer[32];
    mimfs_test_read_file(volume, "/second-name", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "shared bytes") == 0);

    /* Writing through one name is visible through the other. */
    const int descriptor = mimfs_open(volume, "/original", MIMFS_OPEN_WRITE_ONLY, 0);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_write_at_offset(volume, descriptor, "SHARED", 6, 0u), 6);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    mimfs_test_read_file(volume, "/second-name", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "SHARED bytes") == 0);

    /* Removing one name leaves the other, and the content, untouched. */
    MIMFS_CHECK_EQUAL(mimfs_remove_link(volume, "/original"), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/second-name", &linked_status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(linked_status.hard_link_count, 1);
    mimfs_test_read_file(volume, "/second-name", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "SHARED bytes") == 0);

    /* A directory may not be hard linked; that would put a cycle in the tree. */
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/a-directory", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_STATUS(mimfs_create_hard_link(volume, "/a-directory", "/an-alias"),
                       MIMFS_STATUS_NOT_PERMITTED);

    /* Linking onto an existing name is refused. */
    MIMFS_CHECK_STATUS(mimfs_create_hard_link(volume, "/second-name", "/a-directory"),
                       MIMFS_STATUS_ALREADY_EXISTS);

    /* And linking from something that is not there. */
    MIMFS_CHECK_STATUS(mimfs_create_hard_link(volume, "/absent", "/whatever"),
                       MIMFS_STATUS_NO_SUCH_ENTRY);

    printf("ok\n");
}

static void mimfs_test_symbolic_links(void)
{
    mimfs_test_begin_case("symbolic links resolve, and can be inspected unfollowed");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/data", 0755),
                      MIMFS_STATUS_SUCCESS);
    mimfs_test_write_file(volume, "/data/target", "link target content");

    /* An absolute target. */
    MIMFS_CHECK_EQUAL(
        mimfs_create_symbolic_link(volume, "/data/target", "/absolute-link"),
        MIMFS_STATUS_SUCCESS);

    /* A relative target, resolved against the directory holding the link. */
    MIMFS_CHECK_EQUAL(
        mimfs_create_symbolic_link(volume, "target", "/data/relative-link"),
        MIMFS_STATUS_SUCCESS);

    /* A relative target that climbs out of its own directory. */
    MIMFS_CHECK_EQUAL(
        mimfs_create_symbolic_link(volume, "../data/target", "/data/climbing-link"),
        MIMFS_STATUS_SUCCESS);

    char read_buffer[64];

    mimfs_test_read_file(volume, "/absolute-link", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "link target content") == 0);

    mimfs_test_read_file(volume, "/data/relative-link", read_buffer,
                         sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "link target content") == 0);

    mimfs_test_read_file(volume, "/data/climbing-link", read_buffer,
                         sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "link target content") == 0);

    /* A link in the middle of a path is followed. */
    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "/data", "/data-link"),
                      MIMFS_STATUS_SUCCESS);
    mimfs_test_read_file(volume, "/data-link/target", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "link target content") == 0);

    /* Following reports the target; not following reports the link itself. */
    struct mimfs_status_information followed_status;
    struct mimfs_status_information unfollowed_status;

    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/absolute-link", &followed_status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(MIMFS_MODE_IS_REGULAR_FILE(followed_status.file_mode));

    MIMFS_CHECK_EQUAL(
        mimfs_get_link_status(volume, "/absolute-link", &unfollowed_status),
        MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(MIMFS_MODE_IS_SYMBOLIC_LINK(unfollowed_status.file_mode));
    MIMFS_CHECK(followed_status.inode_number != unfollowed_status.inode_number);

    /* The target text reads back exactly. */
    char target_text[64];
    memset(target_text, 0, sizeof(target_text));
    const long target_length = mimfs_read_symbolic_link(
        volume, "/absolute-link", target_text, sizeof(target_text) - 1u);
    MIMFS_CHECK_EQUAL(target_length, (long)strlen("/data/target"));
    MIMFS_CHECK(strcmp(target_text, "/data/target") == 0);

    /* A short target is stored inside the inode and costs no block. */
    MIMFS_CHECK_EQUAL(unfollowed_status.allocated_block_count, 0);

    /* Reading a link on something that is not a link. */
    MIMFS_CHECK_STATUS(mimfs_read_symbolic_link(volume, "/data/target", target_text,
                                                sizeof(target_text)),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    /* A dangling link resolves to nothing, yet still exists as a link. */
    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "/nowhere", "/dangling"),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_STATUS(mimfs_get_status(volume, "/dangling", &followed_status),
                       MIMFS_STATUS_NO_SUCH_ENTRY);
    MIMFS_CHECK_EQUAL(mimfs_get_link_status(volume, "/dangling", &unfollowed_status),
                      MIMFS_STATUS_SUCCESS);

    /* Removing a link removes the link, never the target. */
    MIMFS_CHECK_EQUAL(mimfs_remove_link(volume, "/absolute-link"),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/data/target", &followed_status),
                      MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_long_symbolic_link_target(void)
{
    mimfs_test_begin_case("a target too long for the inode falls back to a block");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    /* Build a target that is longer than the inline capacity but still a valid path.
     * A chain of directories keeps every component within the name limit. */
    char long_target[512];
    size_t write_position = 0u;

    for (unsigned int depth = 0u; depth < 12u; depth++)
    {
        write_position +=
            (size_t)snprintf(long_target + write_position,
                             sizeof(long_target) - write_position, "/level%02u", depth);
    }

    MIMFS_CHECK(strlen(long_target) > MIMFS_INLINE_SYMBOLIC_LINK_CAPACITY_BYTES);

    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, long_target, "/long-link"),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_link_status(volume, "/long-link", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.file_size_in_bytes, strlen(long_target));
    MIMFS_CHECK_EQUAL(status.allocated_block_count, 1);

    char target_text[512];
    memset(target_text, 0, sizeof(target_text));
    MIMFS_CHECK_EQUAL(mimfs_read_symbolic_link(volume, "/long-link", target_text,
                                               sizeof(target_text) - 1u),
                      (long)strlen(long_target));
    MIMFS_CHECK(strcmp(target_text, long_target) == 0);

    /* Removing it must release the block it needed. */
    struct mimfs_volume_statistics before_statistics;
    struct mimfs_volume_statistics after_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &before_statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_remove_link(volume, "/long-link"), MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &after_statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(after_statistics.free_block_count,
                      before_statistics.free_block_count + 1u);

    printf("ok\n");
}

static void mimfs_test_symbolic_link_loops(void)
{
    mimfs_test_begin_case("symbolic link loops are detected rather than followed");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    /* A link pointing at itself. */
    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "/self", "/self"),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_status_information status;
    MIMFS_CHECK_STATUS(mimfs_get_status(volume, "/self", &status),
                       MIMFS_STATUS_TOO_MANY_SYMBOLIC_LINKS);

    /* A pair pointing at each other. */
    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "/pong", "/ping"),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "/ping", "/pong"),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_STATUS(mimfs_get_status(volume, "/ping", &status),
                       MIMFS_STATUS_TOO_MANY_SYMBOLIC_LINKS);

    /* A chain within the limit still resolves. */
    mimfs_test_write_file(volume, "/chain-end", "reached");

    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "/chain-end", "/chain-3"),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "/chain-3", "/chain-2"),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_symbolic_link(volume, "/chain-2", "/chain-1"),
                      MIMFS_STATUS_SUCCESS);

    char read_buffer[32];
    mimfs_test_read_file(volume, "/chain-1", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "reached") == 0);

    /* A loop is still reportable as a link, and still removable. */
    MIMFS_CHECK_EQUAL(mimfs_get_link_status(volume, "/self", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_remove_link(volume, "/self"), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

static void mimfs_test_rename_files(void)
{
    mimfs_test_begin_case("renaming a file moves it and can replace a destination");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/from", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/to", 0755),
                      MIMFS_STATUS_SUCCESS);

    mimfs_test_write_file(volume, "/from/moving", "moving content");

    struct mimfs_status_information before_status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/from/moving", &before_status),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(mimfs_rename(volume, "/from/moving", "/to/moved"),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_status_information after_status;
    MIMFS_CHECK_STATUS(mimfs_get_status(volume, "/from/moving", &after_status),
                       MIMFS_STATUS_NO_SUCH_ENTRY);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/to/moved", &after_status),
                      MIMFS_STATUS_SUCCESS);

    /* The same inode, moved rather than copied. */
    MIMFS_CHECK_EQUAL(after_status.inode_number, before_status.inode_number);

    char read_buffer[32];
    mimfs_test_read_file(volume, "/to/moved", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "moving content") == 0);

    /* Renaming onto an existing file replaces it and releases the old one. */
    mimfs_test_write_file(volume, "/to/victim", "about to be replaced");

    struct mimfs_volume_statistics before_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &before_statistics),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(mimfs_rename(volume, "/to/moved", "/to/victim"),
                      MIMFS_STATUS_SUCCESS);

    mimfs_test_read_file(volume, "/to/victim", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "moving content") == 0);

    struct mimfs_volume_statistics after_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &after_statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(after_statistics.free_inode_count > before_statistics.free_inode_count);

    /* Renaming a name onto itself is a success that changes nothing. */
    MIMFS_CHECK_EQUAL(mimfs_rename(volume, "/to/victim", "/to/victim"),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/to/victim", &after_status),
                      MIMFS_STATUS_SUCCESS);

    /* Renaming something that is not there. */
    MIMFS_CHECK_STATUS(mimfs_rename(volume, "/to/absent", "/to/elsewhere"),
                       MIMFS_STATUS_NO_SUCH_ENTRY);

    printf("ok\n");
}

static void mimfs_test_rename_directories(void)
{
    mimfs_test_begin_case(
        "renaming a directory carries its subtree and fixes its parent");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/outer", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/outer/inner", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/outer/inner/deepest", 0755),
                      MIMFS_STATUS_SUCCESS);
    mimfs_test_write_file(volume, "/outer/inner/deepest/leaf", "deep content");

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/destination", 0755),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(mimfs_rename(volume, "/outer/inner", "/destination/relocated"),
                      MIMFS_STATUS_SUCCESS);

    /* The whole subtree came along. */
    char read_buffer[32];
    mimfs_test_read_file(volume, "/destination/relocated/deepest/leaf", read_buffer,
                         sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "deep content") == 0);

    /* And ".." now names the new parent. */
    struct mimfs_status_information destination_status;
    struct mimfs_status_information parent_through_dot_dot;

    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/destination", &destination_status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(
        mimfs_get_status(volume, "/destination/relocated/..", &parent_through_dot_dot),
        MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(parent_through_dot_dot.inode_number,
                      destination_status.inode_number);

    /* The two parents' link counts followed the move. */
    struct mimfs_status_information old_parent_status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/outer", &old_parent_status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(old_parent_status.hard_link_count, 2);
    MIMFS_CHECK_EQUAL(destination_status.hard_link_count, 3);

    /* A directory may not be moved inside itself. */
    MIMFS_CHECK_STATUS(
        mimfs_rename(volume, "/destination", "/destination/relocated/absurd"),
        MIMFS_STATUS_INVALID_ARGUMENT);
    MIMFS_CHECK_STATUS(mimfs_rename(volume, "/destination", "/destination/child"),
                       MIMFS_STATUS_INVALID_ARGUMENT);

    /* The two ends of a rename must agree on being a directory. */
    mimfs_test_write_file(volume, "/a-file", "content");
    MIMFS_CHECK_STATUS(mimfs_rename(volume, "/destination", "/a-file"),
                       MIMFS_STATUS_NOT_A_DIRECTORY);
    MIMFS_CHECK_STATUS(mimfs_rename(volume, "/a-file", "/destination"),
                       MIMFS_STATUS_IS_A_DIRECTORY);

    /* Replacing a directory works only when the destination is empty. */
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/empty-target", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/full-target", 0755),
                      MIMFS_STATUS_SUCCESS);
    mimfs_test_write_file(volume, "/full-target/occupant", "x");
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/mover", 0755),
                      MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_STATUS(mimfs_rename(volume, "/mover", "/full-target"),
                       MIMFS_STATUS_DIRECTORY_NOT_EMPTY);
    MIMFS_CHECK_EQUAL(mimfs_rename(volume, "/mover", "/empty-target"),
                      MIMFS_STATUS_SUCCESS);

    /* Renaming within one directory is just a change of name. */
    MIMFS_CHECK_EQUAL(mimfs_rename(volume, "/a-file", "/renamed-in-place"),
                      MIMFS_STATUS_SUCCESS);
    mimfs_test_read_file(volume, "/renamed-in-place", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "content") == 0);

    printf("ok\n");
}

static void mimfs_test_metadata_mutation(void)
{
    mimfs_test_begin_case("permissions, ownership and timestamps can be set and read");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    mimfs_test_write_file(volume, "/subject", "content");

    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/subject", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.file_mode & MIMFS_PERMISSION_MASK, 0644);
    MIMFS_CHECK(MIMFS_MODE_IS_REGULAR_FILE(status.file_mode));

    MIMFS_CHECK_EQUAL(mimfs_change_permissions(volume, "/subject", 0600),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/subject", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.file_mode & MIMFS_PERMISSION_MASK, 0600);

    /* The type bits are never writable through the permission interface. */
    MIMFS_CHECK(MIMFS_MODE_IS_REGULAR_FILE(status.file_mode));
    MIMFS_CHECK_EQUAL(mimfs_change_permissions(volume, "/subject", 0177777u),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/subject", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(MIMFS_MODE_IS_REGULAR_FILE(status.file_mode));

    MIMFS_CHECK_EQUAL(mimfs_change_ownership(volume, "/subject", 1000u, 2000u),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/subject", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.owner_user_identifier, 1000);
    MIMFS_CHECK_EQUAL(status.owner_group_identifier, 2000);

    const int64_t chosen_access_time = 1500000000000000000LL;
    const int64_t chosen_modification_time = 1600000000000000000LL;

    MIMFS_CHECK_EQUAL(mimfs_set_times(volume, "/subject", chosen_access_time,
                                      chosen_modification_time),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/subject", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.last_access_time_nanoseconds, chosen_access_time);
    MIMFS_CHECK_EQUAL(status.last_modification_time_nanoseconds,
                      chosen_modification_time);

    /* Writing must move the modification time forward again. */
    const int descriptor = mimfs_open(volume, "/subject", MIMFS_OPEN_WRITE_ONLY, 0);
    MIMFS_CHECK(descriptor >= 0);
    MIMFS_CHECK_EQUAL(mimfs_write_at_offset(volume, descriptor, "z", 1, 0u), 1);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, descriptor), MIMFS_STATUS_SUCCESS);

    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/subject", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK(status.last_modification_time_nanoseconds > chosen_modification_time);

    /* Operations on a path that is not there. */
    MIMFS_CHECK_STATUS(mimfs_change_permissions(volume, "/absent", 0600),
                       MIMFS_STATUS_NO_SUCH_ENTRY);

    printf("ok\n");
}

static void mimfs_test_truncate_by_path(void)
{
    mimfs_test_begin_case("truncation by path is refused on a directory");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    mimfs_test_write_file(volume, "/shrinkable", "0123456789");
    MIMFS_CHECK_EQUAL(mimfs_truncate_by_path(volume, "/shrinkable", 4u),
                      MIMFS_STATUS_SUCCESS);

    char read_buffer[16];
    mimfs_test_read_file(volume, "/shrinkable", read_buffer, sizeof(read_buffer));
    MIMFS_CHECK(strcmp(read_buffer, "0123") == 0);

    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/untouchable", 0755),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_STATUS(mimfs_truncate_by_path(volume, "/untouchable", 0u),
                       MIMFS_STATUS_IS_A_DIRECTORY);

    printf("ok\n");
}

static void mimfs_test_status_descriptions_are_present(void)
{
    mimfs_test_begin_case("every status code has a description");

    /* Both the negated and the positive form must describe the same condition. */
    MIMFS_CHECK(strcmp(mimfs_describe_status(MIMFS_STATUS_NO_SUCH_ENTRY),
                       mimfs_describe_status(-MIMFS_STATUS_NO_SUCH_ENTRY)) == 0);

    MIMFS_CHECK(strcmp(mimfs_describe_status(MIMFS_STATUS_SUCCESS), "success") == 0);
    MIMFS_CHECK(strcmp(mimfs_describe_status(9999), "unrecognised status code") == 0);

    MIMFS_CHECK(mimfs_library_version_string() != NULL);
    MIMFS_CHECK(mimfs_library_version_number() >= (1u << 16));

    printf("ok\n");
}

void mimfs_test_run_namespace_cases(void)
{
    printf("namespace\n");

    mimfs_test_hard_links();
    mimfs_test_symbolic_links();
    mimfs_test_long_symbolic_link_target();
    mimfs_test_symbolic_link_loops();
    mimfs_test_rename_files();
    mimfs_test_rename_directories();
    mimfs_test_metadata_mutation();
    mimfs_test_truncate_by_path();
    mimfs_test_status_descriptions_are_present();
}
