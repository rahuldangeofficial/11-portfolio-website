/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    tests/mimfs_test_harness.c
 * Purpose: Implementation of the test harness and of the shared fixtures.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_test_harness.h"

#include <stdlib.h>

unsigned long mimfs_test_check_count = 0u;
unsigned long mimfs_test_failure_count = 0u;
const char *mimfs_test_current_case_name = "none";

/*
 * The region is a single static object rather than a heap allocation, because that is
 * how this filesystem is meant to be used: given a fixed region it did not allocate.
 * Testing it the same way keeps the tests honest about that contract.
 */
static unsigned char mimfs_test_memory_region[MIMFS_TEST_REGION_SIZE_BYTES];

void mimfs_test_begin_case(const char *case_name)
{
    mimfs_test_current_case_name = case_name;
    printf("  %-62s", case_name);
    fflush(stdout);
}

void mimfs_test_report_failure(const char *file_name, int line_number,
                               const char *expression_text, const char *detail_text)
{
    mimfs_test_failure_count++;

    printf("\n    FAIL %s:%d in %s\n         %s\n", file_name, line_number,
           mimfs_test_current_case_name, expression_text);

    if (detail_text != NULL)
    {
        printf("         %s\n", detail_text);
    }

    fflush(stdout);
}

int mimfs_test_summarize(void)
{
    printf("\n%lu checks, %lu failures\n", mimfs_test_check_count,
           mimfs_test_failure_count);

    return mimfs_test_failure_count == 0u ? 0 : 1;
}

void *mimfs_test_region_base(void)
{
    return mimfs_test_memory_region;
}

size_t mimfs_test_region_size(void)
{
    return sizeof(mimfs_test_memory_region);
}

struct mimfs_volume *
mimfs_test_create_volume_with(const struct mimfs_format_parameters *parameters)
{
    int format_status = MIMFS_STATUS_SUCCESS;

    struct mimfs_volume *volume =
        mimfs_format(mimfs_test_memory_region, sizeof(mimfs_test_memory_region),
                     parameters, &format_status);

    if (volume == NULL)
    {
        printf("\n    unable to format the test volume: %s\n",
               mimfs_describe_status(format_status));
    }

    return volume;
}

struct mimfs_volume *mimfs_test_create_volume(void)
{
    struct mimfs_format_parameters parameters;

    parameters.bytes_per_inode_ratio = 4096u;
    parameters.maximum_open_file_count = 256u;
    parameters.reserved_block_percentage = 2u;
    parameters.blocks_per_block_group = 0u;

    return mimfs_test_create_volume_with(&parameters);
}
