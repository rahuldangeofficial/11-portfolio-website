/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    tests/mimfs_test_harness.h
 * Purpose: A very small assertion and reporting harness, so that the test files
 *          contain assertions about the filesystem and nothing else.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 */

#ifndef MIMFS_TEST_HARNESS_H
#define MIMFS_TEST_HARNESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "mimfs/mimfs.h"

/* Running totals, shared by every test file and reported once at the end. */
extern unsigned long mimfs_test_check_count;
extern unsigned long mimfs_test_failure_count;
extern const char *mimfs_test_current_case_name;

void mimfs_test_begin_case(const char *case_name);
void mimfs_test_report_failure(const char *file_name, int line_number,
                               const char *expression_text, const char *detail_text);
int mimfs_test_summarize(void);

/*
 * Assert a condition. Reports the file, the line, the expression and the name of the
 * case, then keeps going, because a run that stops at the first failure hides how
 * many other things are broken.
 */
#define MIMFS_CHECK(condition)                                                         \
    do                                                                                 \
    {                                                                                  \
        mimfs_test_check_count++;                                                      \
        if (!(condition))                                                              \
        {                                                                              \
            mimfs_test_report_failure(__FILE__, __LINE__, #condition, NULL);           \
        }                                                                              \
    } while (0)

/* Assert that two integers are equal, reporting both values on failure. */
#define MIMFS_CHECK_EQUAL(actual, expected)                                            \
    do                                                                                 \
    {                                                                                  \
        mimfs_test_check_count++;                                                      \
        const long long actual_value_ = (long long)(actual);                           \
        const long long expected_value_ = (long long)(expected);                       \
        if (actual_value_ != expected_value_)                                          \
        {                                                                              \
            char detail_text_[160];                                                    \
            snprintf(detail_text_, sizeof(detail_text_),                               \
                     "expected %lld, actually got %lld", expected_value_,              \
                     actual_value_);                                                   \
            mimfs_test_report_failure(__FILE__, __LINE__, #actual, detail_text_);      \
        }                                                                              \
    } while (0)

/* Assert that a call failed with a particular status. */
#define MIMFS_CHECK_STATUS(actual, expected_status)                                    \
    do                                                                                 \
    {                                                                                  \
        mimfs_test_check_count++;                                                      \
        const long long actual_value_ = (long long)(actual);                           \
        if (actual_value_ != -(long long)(expected_status))                            \
        {                                                                              \
            char detail_text_[200];                                                    \
            snprintf(detail_text_, sizeof(detail_text_),                               \
                     "expected failure \"%s\", actually got %lld (%s)",                \
                     mimfs_describe_status((int)(expected_status)), actual_value_,     \
                     mimfs_describe_status((int)actual_value_));                       \
            mimfs_test_report_failure(__FILE__, __LINE__, #actual, detail_text_);      \
        }                                                                              \
    } while (0)

/* ---- shared fixtures ---------------------------------------------------- */

/*
 * Size of the memory region the tests format. Large enough to exercise several block
 * groups and the double indirect level of the block map, small enough to stay
 * comfortable as a static object.
 */
#define MIMFS_TEST_REGION_SIZE_BYTES (64u * 1024u * 1024u)

/* Format a fresh volume in the shared region and return it. */
struct mimfs_volume *mimfs_test_create_volume(void);

/* Format a fresh volume with explicit parameters. */
struct mimfs_volume *
mimfs_test_create_volume_with(const struct mimfs_format_parameters *parameters);

/* The raw region behind the fixtures, for tests that mount it a second time. */
void *mimfs_test_region_base(void);
size_t mimfs_test_region_size(void);

/* ---- the test cases ----------------------------------------------------- */

void mimfs_test_run_volume_cases(void);
void mimfs_test_run_file_cases(void);
void mimfs_test_run_directory_cases(void);
void mimfs_test_run_namespace_cases(void);
void mimfs_test_run_concurrency_cases(void);

#endif /* MIMFS_TEST_HARNESS_H */
