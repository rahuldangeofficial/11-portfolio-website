/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    examples/mimfs_shell.c
 * Purpose: An interactive shell over one volume, so that the filesystem can be
 *          driven by hand.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * This is the shortest useful demonstration of the interface: it touches nearly
 * every entry point, and each command maps to exactly one of them, so the source
 * doubles as a worked example of how each is called.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mimfs/mimfs.h"

#define MIMFS_SHELL_REGION_SIZE_BYTES (32u * 1024u * 1024u)
#define MIMFS_SHELL_INPUT_LINE_SIZE   8192

/* The volume lives in a static region, which is how an embedded caller would
 * provide it: no dynamic allocation anywhere in the program. */
static unsigned char mimfs_shell_memory_region[MIMFS_SHELL_REGION_SIZE_BYTES];

/* One open descriptor per shell slot, so that a session can hold several files
 * open at once and observe how they interact. */
#define MIMFS_SHELL_SLOT_COUNT 8
static int mimfs_shell_descriptor_slots[MIMFS_SHELL_SLOT_COUNT];

static void mimfs_shell_report_status(const char *operation_name, int status)
{
    if (status < 0)
    {
        printf("%s failed: %s\n", operation_name, mimfs_describe_status(status));
        return;
    }

    printf("%s: done\n", operation_name);
}

/* Split a line into at most three whitespace separated words, in place. */
static int mimfs_shell_split_line(char *line, char **word_array, int maximum_word_count)
{
    int word_count = 0;
    char *cursor = line;

    while (*cursor != '\0' && word_count < maximum_word_count)
    {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n')
        {
            *cursor = '\0';
            cursor++;
        }

        if (*cursor == '\0')
        {
            break;
        }

        *(word_array + word_count) = cursor;
        word_count++;

        /*
         * The last word runs to the end of the line, separators and all, so that a
         * command taking free text such as "write" receives it whole.
         */
        if (word_count == maximum_word_count)
        {
            char *line_end = cursor + strlen(cursor);

            while (line_end > cursor && *(line_end - 1) == '\n')
            {
                line_end--;
                *line_end = '\0';
            }

            break;
        }

        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t' && *cursor != '\n')
        {
            cursor++;
        }
    }

    return word_count;
}

static void mimfs_shell_print_help(void)
{
    printf("commands:\n"
           "  ls <path>                    list a directory\n"
           "  stat <path>                  report on a file, link or directory\n"
           "  mkdir <path>                 create a directory\n"
           "  rmdir <path>                 remove an empty directory\n"
           "  touch <path>                 create an empty file\n"
           "  write <path> <text>          replace a file with the given text\n"
           "  append <path> <text>         add the given text to the end of a file\n"
           "  cat <path>                   print the contents of a file\n"
           "  rm <path>                    remove one name\n"
           "  ln <existing> <new>          create a hard link\n"
           "  symlink <target> <path>      create a symbolic link\n"
           "  readlink <path>              print the target of a symbolic link\n"
           "  mv <old> <new>               move a name\n"
           "  truncate <path> <length>     set the length of a file\n"
           "  chmod <path> <octal>         set the permission bits\n"
           "  open <slot> <path>           open a file into a descriptor slot\n"
           "  close <slot>                 close a descriptor slot\n"
           "  read <slot> <count>          read from a slot, advancing its offset\n"
           "  seek <slot> <offset>         move the offset of a slot\n"
           "  df                           report volume usage\n"
           "  help                         print this list\n"
           "  quit                         leave\n");
}

static void mimfs_shell_list_directory(struct mimfs_volume *volume, const char *path)
{
    const int stream = mimfs_open_directory(volume, path);

    if (stream < 0)
    {
        printf("ls failed: %s\n", mimfs_describe_status(stream));
        return;
    }

    struct mimfs_directory_entry_information entry;
    unsigned int entry_count = 0u;

    while (mimfs_read_directory(volume, stream, &entry) == 1)
    {
        const char *type_label = "file";

        if (entry.entry_type == MIMFS_ENTRY_TYPE_DIRECTORY)
        {
            type_label = "dir ";
        }
        else if (entry.entry_type == MIMFS_ENTRY_TYPE_SYMBOLIC_LINK)
        {
            type_label = "link";
        }

        printf("  %s  inode %-6u  %s\n", type_label, entry.inode_number, entry.name);
        entry_count++;
    }

    printf("  %u entries\n", entry_count);

    (void)mimfs_close_directory(volume, stream);
}

static void mimfs_shell_print_status(struct mimfs_volume *volume, const char *path)
{
    struct mimfs_status_information status;

    const int query_status = mimfs_get_link_status(volume, path, &status);

    if (query_status < 0)
    {
        printf("stat failed: %s\n", mimfs_describe_status(query_status));
        return;
    }

    const char *type_label = "regular file";

    if (MIMFS_MODE_IS_DIRECTORY(status.file_mode))
    {
        type_label = "directory";
    }
    else if (MIMFS_MODE_IS_SYMBOLIC_LINK(status.file_mode))
    {
        type_label = "symbolic link";
    }

    printf("  inode        %u\n", status.inode_number);
    printf("  type         %s\n", type_label);
    printf("  permissions  %04o\n", status.file_mode & MIMFS_PERMISSION_MASK);
    printf("  links        %u\n", status.hard_link_count);
    printf("  owner        %u:%u\n", status.owner_user_identifier,
           status.owner_group_identifier);
    printf("  size         %llu bytes\n",
           (unsigned long long)status.file_size_in_bytes);
    printf("  blocks       %llu of %u bytes\n",
           (unsigned long long)status.allocated_block_count,
           status.block_size_in_bytes);
    printf("  modified     %lld nanoseconds since the epoch\n",
           (long long)status.last_modification_time_nanoseconds);
}

static void mimfs_shell_print_usage(struct mimfs_volume *volume)
{
    struct mimfs_volume_statistics statistics;

    if (mimfs_query_volume_statistics(volume, &statistics) < 0)
    {
        printf("df failed\n");
        return;
    }

    const unsigned long long used_block_count =
        (unsigned long long)(statistics.total_block_count -
                             statistics.free_block_count);

    printf("  block size          %u bytes\n", statistics.block_size_in_bytes);
    printf("  blocks              %llu total, %llu used, %llu free, %llu available\n",
           (unsigned long long)statistics.total_block_count, used_block_count,
           (unsigned long long)statistics.free_block_count,
           (unsigned long long)statistics.available_block_count);
    printf("  inodes              %llu total, %llu free\n",
           (unsigned long long)statistics.total_inode_count,
           (unsigned long long)statistics.free_inode_count);
    printf("  block groups        %u of %u blocks and %u inodes each\n",
           statistics.block_group_count, statistics.blocks_per_block_group,
           statistics.inodes_per_block_group);
    printf("  descriptors         %u open of %u\n", statistics.open_file_count,
           statistics.maximum_open_file_count);
    printf("  largest file        %llu bytes\n",
           (unsigned long long)statistics.maximum_file_size_in_bytes);
    printf("  longest name        %u bytes\n", statistics.maximum_name_length);
}

static void mimfs_shell_write_text(struct mimfs_volume *volume, const char *path,
                                   const char *text, bool append_instead_of_replace)
{
    unsigned int open_flags = MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE;

    open_flags |= append_instead_of_replace ? MIMFS_OPEN_APPEND : MIMFS_OPEN_TRUNCATE;

    const int descriptor = mimfs_open(volume, path, open_flags, 0644);

    if (descriptor < 0)
    {
        printf("write failed: %s\n", mimfs_describe_status(descriptor));
        return;
    }

    const long bytes_written = mimfs_write(volume, descriptor, text, strlen(text));

    if (bytes_written < 0)
    {
        printf("write failed: %s\n", mimfs_describe_status((int)bytes_written));
    }
    else
    {
        printf("wrote %ld bytes\n", bytes_written);
    }

    (void)mimfs_close(volume, descriptor);
}

static void mimfs_shell_print_file(struct mimfs_volume *volume, const char *path)
{
    const int descriptor = mimfs_open(volume, path, MIMFS_OPEN_READ_ONLY, 0);

    if (descriptor < 0)
    {
        printf("cat failed: %s\n", mimfs_describe_status(descriptor));
        return;
    }

    char transfer_buffer[4096];
    long bytes_read;

    while ((bytes_read = mimfs_read(volume, descriptor, transfer_buffer,
                                    sizeof(transfer_buffer))) > 0)
    {
        (void)fwrite(transfer_buffer, 1u, (size_t)bytes_read, stdout);
    }

    if (bytes_read < 0)
    {
        printf("\ncat failed: %s\n", mimfs_describe_status((int)bytes_read));
    }
    else
    {
        printf("\n");
    }

    (void)mimfs_close(volume, descriptor);
}

/* True when the slot number names one of the shell's descriptor slots. */
static bool mimfs_shell_slot_is_valid(int slot_number)
{
    return slot_number >= 0 && slot_number < MIMFS_SHELL_SLOT_COUNT;
}

int main(void)
{
    for (int slot_index = 0; slot_index < MIMFS_SHELL_SLOT_COUNT; slot_index++)
    {
        *(mimfs_shell_descriptor_slots + slot_index) = -1;
    }

    int format_status = MIMFS_STATUS_SUCCESS;

    struct mimfs_volume *volume =
        mimfs_format(mimfs_shell_memory_region, sizeof(mimfs_shell_memory_region), NULL,
                     &format_status);

    if (volume == NULL)
    {
        printf("unable to format the volume: %s\n",
               mimfs_describe_status(format_status));
        return 1;
    }

    printf("MIMFS %s, %u MiB volume mounted at /\n", mimfs_library_version_string(),
           (unsigned int)(sizeof(mimfs_shell_memory_region) / (1024u * 1024u)));
    printf("type help for the command list\n\n");

    char input_line[MIMFS_SHELL_INPUT_LINE_SIZE];

    for (;;)
    {
        printf("mimfs> ");
        (void)fflush(stdout);

        if (fgets(input_line, sizeof(input_line), stdin) == NULL)
        {
            printf("\n");
            break;
        }

        char *word_array[3] = { NULL, NULL, NULL };
        const int word_count = mimfs_shell_split_line(input_line, word_array, 3);

        if (word_count == 0)
        {
            continue;
        }

        const char *command = *(word_array + 0);
        const char *first_argument = word_count > 1 ? *(word_array + 1) : NULL;
        const char *second_argument = word_count > 2 ? *(word_array + 2) : NULL;

        if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0)
        {
            break;
        }

        if (strcmp(command, "help") == 0)
        {
            mimfs_shell_print_help();
        }
        else if (strcmp(command, "df") == 0)
        {
            mimfs_shell_print_usage(volume);
        }
        else if (first_argument == NULL)
        {
            printf("%s needs a path; type help\n", command);
        }
        else if (strcmp(command, "ls") == 0)
        {
            mimfs_shell_list_directory(volume, first_argument);
        }
        else if (strcmp(command, "stat") == 0)
        {
            mimfs_shell_print_status(volume, first_argument);
        }
        else if (strcmp(command, "mkdir") == 0)
        {
            mimfs_shell_report_status(
                "mkdir", mimfs_create_directory(volume, first_argument, 0755));
        }
        else if (strcmp(command, "rmdir") == 0)
        {
            mimfs_shell_report_status("rmdir",
                                      mimfs_remove_directory(volume, first_argument));
        }
        else if (strcmp(command, "touch") == 0)
        {
            const int descriptor =
                mimfs_open(volume, first_argument,
                           MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

            if (descriptor < 0)
            {
                printf("touch failed: %s\n", mimfs_describe_status(descriptor));
            }
            else
            {
                (void)mimfs_close(volume, descriptor);
                printf("touch: done\n");
            }
        }
        else if (strcmp(command, "cat") == 0)
        {
            mimfs_shell_print_file(volume, first_argument);
        }
        else if (strcmp(command, "rm") == 0)
        {
            mimfs_shell_report_status("rm", mimfs_remove_link(volume, first_argument));
        }
        else if (strcmp(command, "readlink") == 0)
        {
            char target_text[MIMFS_MAXIMUM_PATH_LENGTH];

            const long target_length = mimfs_read_symbolic_link(
                volume, first_argument, target_text, sizeof(target_text) - 1u);

            if (target_length < 0)
            {
                printf("readlink failed: %s\n",
                       mimfs_describe_status((int)target_length));
            }
            else
            {
                *(target_text + target_length) = '\0';
                printf("  %s\n", target_text);
            }
        }
        else if (strcmp(command, "close") == 0)
        {
            const int slot_number = atoi(first_argument);

            if (!mimfs_shell_slot_is_valid(slot_number))
            {
                printf("slot must be between 0 and %d\n", MIMFS_SHELL_SLOT_COUNT - 1);
            }
            else
            {
                mimfs_shell_report_status(
                    "close",
                    mimfs_close(volume, *(mimfs_shell_descriptor_slots + slot_number)));

                *(mimfs_shell_descriptor_slots + slot_number) = -1;
            }
        }
        else if (second_argument == NULL)
        {
            printf("%s needs two arguments; type help\n", command);
        }
        else if (strcmp(command, "write") == 0)
        {
            mimfs_shell_write_text(volume, first_argument, second_argument, false);
        }
        else if (strcmp(command, "append") == 0)
        {
            mimfs_shell_write_text(volume, first_argument, second_argument, true);
        }
        else if (strcmp(command, "ln") == 0)
        {
            mimfs_shell_report_status(
                "ln", mimfs_create_hard_link(volume, first_argument, second_argument));
        }
        else if (strcmp(command, "symlink") == 0)
        {
            mimfs_shell_report_status(
                "symlink",
                mimfs_create_symbolic_link(volume, first_argument, second_argument));
        }
        else if (strcmp(command, "mv") == 0)
        {
            mimfs_shell_report_status(
                "mv", mimfs_rename(volume, first_argument, second_argument));
        }
        else if (strcmp(command, "truncate") == 0)
        {
            mimfs_shell_report_status(
                "truncate",
                mimfs_truncate_by_path(volume, first_argument,
                                       (uint64_t)strtoull(second_argument, NULL, 10)));
        }
        else if (strcmp(command, "chmod") == 0)
        {
            mimfs_shell_report_status(
                "chmod", mimfs_change_permissions(
                             volume, first_argument,
                             (unsigned int)strtoul(second_argument, NULL, 8)));
        }
        else if (strcmp(command, "open") == 0)
        {
            const int slot_number = atoi(first_argument);

            if (!mimfs_shell_slot_is_valid(slot_number))
            {
                printf("slot must be between 0 and %d\n", MIMFS_SHELL_SLOT_COUNT - 1);
            }
            else
            {
                const int descriptor =
                    mimfs_open(volume, second_argument,
                               MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

                if (descriptor < 0)
                {
                    printf("open failed: %s\n", mimfs_describe_status(descriptor));
                }
                else
                {
                    *(mimfs_shell_descriptor_slots + slot_number) = descriptor;
                    printf("slot %d holds descriptor %d\n", slot_number, descriptor);
                }
            }
        }
        else if (strcmp(command, "read") == 0)
        {
            const int slot_number = atoi(first_argument);
            const size_t requested_count = (size_t)strtoull(second_argument, NULL, 10);

            if (!mimfs_shell_slot_is_valid(slot_number))
            {
                printf("slot must be between 0 and %d\n", MIMFS_SHELL_SLOT_COUNT - 1);
            }
            else
            {
                char transfer_buffer[4096];

                const size_t count_to_read =
                    requested_count < sizeof(transfer_buffer) - 1u
                        ? requested_count
                        : sizeof(transfer_buffer) - 1u;

                const long bytes_read =
                    mimfs_read(volume, *(mimfs_shell_descriptor_slots + slot_number),
                               transfer_buffer, count_to_read);

                if (bytes_read < 0)
                {
                    printf("read failed: %s\n", mimfs_describe_status((int)bytes_read));
                }
                else
                {
                    *(transfer_buffer + bytes_read) = '\0';
                    printf("%ld bytes: %s\n", bytes_read, transfer_buffer);
                }
            }
        }
        else if (strcmp(command, "seek") == 0)
        {
            const int slot_number = atoi(first_argument);

            if (!mimfs_shell_slot_is_valid(slot_number))
            {
                printf("slot must be between 0 and %d\n", MIMFS_SHELL_SLOT_COUNT - 1);
            }
            else
            {
                const long long resulting_offset = mimfs_seek(
                    volume, *(mimfs_shell_descriptor_slots + slot_number),
                    strtoll(second_argument, NULL, 10), MIMFS_SEEK_FROM_START);

                if (resulting_offset < 0)
                {
                    printf("seek failed: %s\n",
                           mimfs_describe_status((int)resulting_offset));
                }
                else
                {
                    printf("offset is now %lld\n", resulting_offset);
                }
            }
        }
        else
        {
            printf("unknown command %s; type help\n", command);
        }
    }

    (void)mimfs_unmount(volume);

    printf("unmounted\n");

    return 0;
}
