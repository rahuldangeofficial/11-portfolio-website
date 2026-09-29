/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    tests/mimfs_test_main.c
 * Purpose: Runs every group of cases and reports the outcome.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#include "mimfs_test_harness.h"

int main(void)
{
    printf("MIMFS %s test suite\n\n", mimfs_library_version_string());

    /*
     * The order runs from the least dependent group to the most. A failure in the
     * volume group makes the later groups meaningless, so seeing it first tells the
     * reader where to look without having to scan the whole report.
     */
    mimfs_test_run_volume_cases();
    mimfs_test_run_file_cases();
    mimfs_test_run_directory_cases();
    mimfs_test_run_namespace_cases();
    mimfs_test_run_concurrency_cases();

    return mimfs_test_summarize();
}
