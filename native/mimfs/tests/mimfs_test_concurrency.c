/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    tests/mimfs_test_concurrency.c
 * Purpose: Cases that run many threads against one volume at once.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * These cases assert properties that hold no matter how the threads interleave,
 * rather than asserting a particular interleaving:
 *
 *   * accounting closes: every block and inode taken is given back
 *   * nothing is lost: every append that reported success is present in the file
 *   * nothing is duplicated: exactly one thread wins each exclusive creation
 *   * nothing tears: a reader either sees a whole record or none of it
 *
 * A concurrency failure is a probabilistic event, so the loop counts are set high
 * enough that a broken lock or a lost update shows up on essentially every run rather
 * than occasionally.
 */

#include "mimfs_test_harness.h"

#include <pthread.h>
#include <string.h>

#define MIMFS_TEST_THREAD_COUNT 8u

/* Context handed to every worker thread. */
struct mimfs_test_worker_context
{
    struct mimfs_volume *volume;
    unsigned int worker_index;
    unsigned int iteration_count;

    /* Filled in by the worker, examined by the main thread afterwards. */
    unsigned long success_count;
    unsigned long failure_count;
};

/* Run one worker function on every thread and wait for all of them. */
static void mimfs_test_run_workers(void *(*worker_function)(void *),
                                   struct mimfs_volume *volume,
                                   unsigned int iteration_count,
                                   struct mimfs_test_worker_context *contexts)
{
    pthread_t threads[MIMFS_TEST_THREAD_COUNT];

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        struct mimfs_test_worker_context *context = contexts + index;

        (*context).volume = volume;
        (*context).worker_index = index;
        (*context).iteration_count = iteration_count;
        (*context).success_count = 0u;
        (*context).failure_count = 0u;

        MIMFS_CHECK_EQUAL(
            pthread_create(threads + index, NULL, worker_function, context), 0);
    }

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        MIMFS_CHECK_EQUAL(pthread_join(*(threads + index), NULL), 0);
    }
}

/* ---- creating and removing files in parallel ---------------------------- */

static void *mimfs_test_create_and_remove_worker(void *raw_context)
{
    struct mimfs_test_worker_context *context =
        (struct mimfs_test_worker_context *)raw_context;

    struct mimfs_volume *volume = (*context).volume;

    for (unsigned int iteration = 0u; iteration < (*context).iteration_count;
         iteration++)
    {
        char path[80];
        snprintf(path, sizeof(path), "/parallel/worker%u-item%u",
                 (*context).worker_index, iteration);

        const int descriptor =
            mimfs_open(volume, path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

        if (descriptor < 0)
        {
            (*context).failure_count++;
            continue;
        }

        char payload[64];
        const int payload_length =
            snprintf(payload, sizeof(payload), "worker %u item %u",
                     (*context).worker_index, iteration);

        if (mimfs_write(volume, descriptor, payload, (size_t)payload_length) !=
            payload_length)
        {
            (*context).failure_count++;
        }

        (void)mimfs_close(volume, descriptor);

        /* Read it back through a fresh descriptor, so the path is resolved again while
         * other threads are modifying the same directory. */
        char read_buffer[64];
        memset(read_buffer, 0, sizeof(read_buffer));

        const int reader = mimfs_open(volume, path, MIMFS_OPEN_READ_ONLY, 0);

        if (reader < 0)
        {
            (*context).failure_count++;
            continue;
        }

        const long bytes_read =
            mimfs_read(volume, reader, read_buffer, sizeof(read_buffer));
        (void)mimfs_close(volume, reader);

        if (bytes_read != payload_length ||
            memcmp(read_buffer, payload, (size_t)payload_length) != 0)
        {
            (*context).failure_count++;
            continue;
        }

        if (mimfs_remove_link(volume, path) != MIMFS_STATUS_SUCCESS)
        {
            (*context).failure_count++;
            continue;
        }

        (*context).success_count++;
    }

    return NULL;
}

static void mimfs_test_parallel_create_and_remove(void)
{
    mimfs_test_begin_case("parallel create, write, read and remove closes accounting");

    struct mimfs_volume *volume = mimfs_test_create_volume();
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/parallel", 0755),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_volume_statistics before_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &before_statistics),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_test_worker_context contexts[MIMFS_TEST_THREAD_COUNT];

    mimfs_test_run_workers(mimfs_test_create_and_remove_worker, volume, 400u, contexts);

    unsigned long total_success_count = 0u;
    unsigned long total_failure_count = 0u;

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        total_success_count += (*(contexts + index)).success_count;
        total_failure_count += (*(contexts + index)).failure_count;
    }

    MIMFS_CHECK_EQUAL(total_failure_count, 0);
    MIMFS_CHECK_EQUAL(total_success_count, 400u * MIMFS_TEST_THREAD_COUNT);

    /* Every file was removed, so the volume must be exactly as it started. */
    struct mimfs_volume_statistics after_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &after_statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(after_statistics.free_block_count,
                      before_statistics.free_block_count);
    MIMFS_CHECK_EQUAL(after_statistics.free_inode_count,
                      before_statistics.free_inode_count);
    MIMFS_CHECK_EQUAL(after_statistics.open_file_count, 0);

    printf("ok (%lu operations across %u threads)\n", total_success_count,
           MIMFS_TEST_THREAD_COUNT);
}

/* ---- exclusive creation of one shared name ------------------------------ */

static void *mimfs_test_exclusive_race_worker(void *raw_context)
{
    struct mimfs_test_worker_context *context =
        (struct mimfs_test_worker_context *)raw_context;

    struct mimfs_volume *volume = (*context).volume;

    for (unsigned int iteration = 0u; iteration < (*context).iteration_count;
         iteration++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/contested-%u", iteration);

        const int descriptor = mimfs_open(
            volume, path,
            MIMFS_OPEN_CREATE | MIMFS_OPEN_EXCLUSIVE | MIMFS_OPEN_READ_WRITE, 0644);

        if (descriptor >= 0)
        {
            (*context).success_count++;
            (void)mimfs_close(volume, descriptor);
        }
        else if (descriptor == -MIMFS_STATUS_ALREADY_EXISTS)
        {
            (*context).failure_count++;
        }
        else
        {
            /* Any other outcome is a defect, and is counted as an unexpected success so
             * that the totals below no longer add up. */
            (*context).success_count += 1000u;
        }
    }

    return NULL;
}

static void mimfs_test_exclusive_creation_has_exactly_one_winner(void)
{
    mimfs_test_begin_case("exclusive creation of one name has exactly one winner");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const unsigned int contested_name_count = 200u;

    struct mimfs_test_worker_context contexts[MIMFS_TEST_THREAD_COUNT];

    mimfs_test_run_workers(mimfs_test_exclusive_race_worker, volume,
                           contested_name_count, contexts);

    unsigned long total_success_count = 0u;
    unsigned long total_refusal_count = 0u;

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        total_success_count += (*(contexts + index)).success_count;
        total_refusal_count += (*(contexts + index)).failure_count;
    }

    /* One thread creates each name; every other thread must be refused. */
    MIMFS_CHECK_EQUAL(total_success_count, contested_name_count);
    MIMFS_CHECK_EQUAL(total_refusal_count,
                      contested_name_count * (MIMFS_TEST_THREAD_COUNT - 1u));

    printf("ok\n");
}

/* ---- concurrent appends to one file ------------------------------------- */

#define MIMFS_TEST_APPEND_RECORD_SIZE 32u

static void *mimfs_test_append_worker(void *raw_context)
{
    struct mimfs_test_worker_context *context =
        (struct mimfs_test_worker_context *)raw_context;

    struct mimfs_volume *volume = (*context).volume;

    const int descriptor =
        mimfs_open(volume, "/shared-log", MIMFS_OPEN_READ_WRITE | MIMFS_OPEN_APPEND, 0);

    if (descriptor < 0)
    {
        (*context).failure_count++;
        return NULL;
    }

    for (unsigned int iteration = 0u; iteration < (*context).iteration_count;
         iteration++)
    {
        char record[MIMFS_TEST_APPEND_RECORD_SIZE];

        /*
         * Every record is the same size and carries the identity of its writer in every
         * byte position that matters, so a torn or interleaved append is detectable by
         * reading the file back and checking that each record is internally consistent.
         */
        memset(record, 'A' + (int)(*context).worker_index, sizeof(record));

        if (mimfs_write(volume, descriptor, record, sizeof(record)) !=
            (long)sizeof(record))
        {
            (*context).failure_count++;
            continue;
        }

        (*context).success_count++;
    }

    (void)mimfs_close(volume, descriptor);

    return NULL;
}

static void mimfs_test_concurrent_appends_do_not_interleave(void)
{
    mimfs_test_begin_case("concurrent appends neither overwrite nor tear");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int creator = mimfs_open(volume, "/shared-log",
                                   MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(creator >= 0);
    MIMFS_CHECK_EQUAL(mimfs_close(volume, creator), MIMFS_STATUS_SUCCESS);

    const unsigned int records_per_worker = 500u;

    struct mimfs_test_worker_context contexts[MIMFS_TEST_THREAD_COUNT];

    mimfs_test_run_workers(mimfs_test_append_worker, volume, records_per_worker,
                           contexts);

    unsigned long total_records_written = 0u;

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        MIMFS_CHECK_EQUAL((*(contexts + index)).failure_count, 0);
        total_records_written += (*(contexts + index)).success_count;
    }

    MIMFS_CHECK_EQUAL(total_records_written,
                      records_per_worker * MIMFS_TEST_THREAD_COUNT);

    /* The file must be exactly as long as the records that were reported written. */
    struct mimfs_status_information status;
    MIMFS_CHECK_EQUAL(mimfs_get_status(volume, "/shared-log", &status),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(status.file_size_in_bytes,
                      total_records_written * MIMFS_TEST_APPEND_RECORD_SIZE);

    /* And every record must be whole: no append may have landed part way through
     * another. */
    const int reader = mimfs_open(volume, "/shared-log", MIMFS_OPEN_READ_ONLY, 0);
    MIMFS_CHECK(reader >= 0);

    unsigned long records_verified = 0u;
    unsigned long records_per_writer[MIMFS_TEST_THREAD_COUNT];

    memset(records_per_writer, 0, sizeof(records_per_writer));

    for (;;)
    {
        char record[MIMFS_TEST_APPEND_RECORD_SIZE];

        const long bytes_read = mimfs_read(volume, reader, record, sizeof(record));

        if (bytes_read == 0)
        {
            break;
        }

        if (bytes_read != (long)sizeof(record))
        {
            MIMFS_CHECK(false);
            break;
        }

        const char writer_marker = *(record + 0);
        bool record_is_whole = true;

        for (size_t byte_index = 1u; byte_index < sizeof(record); byte_index++)
        {
            if (*(record + byte_index) != writer_marker)
            {
                record_is_whole = false;
                break;
            }
        }

        if (!record_is_whole)
        {
            MIMFS_CHECK(false);
            break;
        }

        const unsigned int writer_index = (unsigned int)(writer_marker - 'A');

        if (writer_index >= MIMFS_TEST_THREAD_COUNT)
        {
            MIMFS_CHECK(false);
            break;
        }

        *(records_per_writer + writer_index) += 1u;
        records_verified++;
    }

    MIMFS_CHECK_EQUAL(records_verified, total_records_written);

    /* Each writer's records must all be present. */
    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        MIMFS_CHECK_EQUAL(*(records_per_writer + index), records_per_worker);
    }

    MIMFS_CHECK_EQUAL(mimfs_close(volume, reader), MIMFS_STATUS_SUCCESS);

    printf("ok (%lu records from %u threads, all whole)\n", records_verified,
           MIMFS_TEST_THREAD_COUNT);
}

/* ---- positioned transfers on disjoint regions of one file --------------- */

struct mimfs_test_positioned_context
{
    struct mimfs_volume *volume;
    int shared_descriptor;
    unsigned int worker_index;
    unsigned int block_count;
    unsigned long failure_count;
};

static void *mimfs_test_positioned_worker(void *raw_context)
{
    struct mimfs_test_positioned_context *context =
        (struct mimfs_test_positioned_context *)raw_context;

    /*
     * Every worker owns a disjoint stripe of the file and reaches it through the one
     * shared descriptor with positioned transfers, which is the pattern positioned
     * transfers exist for: no shared offset is consulted, so no worker interferes with
     * another.
     */
    for (unsigned int block_index = 0u; block_index < (*context).block_count;
         block_index++)
    {
        char payload[512];
        memset(payload, (int)('a' + (*context).worker_index), sizeof(payload));

        const uint64_t offset = ((uint64_t)block_index * MIMFS_TEST_THREAD_COUNT +
                                 (*context).worker_index) *
                                sizeof(payload);

        if (mimfs_write_at_offset((*context).volume, (*context).shared_descriptor,
                                  payload, sizeof(payload),
                                  offset) != (long)sizeof(payload))
        {
            (*context).failure_count++;
        }
    }

    for (unsigned int block_index = 0u; block_index < (*context).block_count;
         block_index++)
    {
        char read_buffer[512];
        memset(read_buffer, 0, sizeof(read_buffer));

        const uint64_t offset = ((uint64_t)block_index * MIMFS_TEST_THREAD_COUNT +
                                 (*context).worker_index) *
                                sizeof(read_buffer);

        if (mimfs_read_at_offset((*context).volume, (*context).shared_descriptor,
                                 read_buffer, sizeof(read_buffer),
                                 offset) != (long)sizeof(read_buffer))
        {
            (*context).failure_count++;
            continue;
        }

        for (size_t byte_index = 0u; byte_index < sizeof(read_buffer); byte_index++)
        {
            if (*(read_buffer + byte_index) != (char)('a' + (*context).worker_index))
            {
                (*context).failure_count++;
                break;
            }
        }
    }

    return NULL;
}

static void mimfs_test_positioned_transfers_are_independent(void)
{
    mimfs_test_begin_case("positioned transfers on one descriptor stay independent");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    const int shared_descriptor =
        mimfs_open(volume, "/striped", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    MIMFS_CHECK(shared_descriptor >= 0);

    pthread_t threads[MIMFS_TEST_THREAD_COUNT];
    struct mimfs_test_positioned_context contexts[MIMFS_TEST_THREAD_COUNT];

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        struct mimfs_test_positioned_context *context = contexts + index;

        (*context).volume = volume;
        (*context).shared_descriptor = shared_descriptor;
        (*context).worker_index = index;
        (*context).block_count = 200u;
        (*context).failure_count = 0u;

        MIMFS_CHECK_EQUAL(pthread_create(threads + index, NULL,
                                         mimfs_test_positioned_worker, context),
                          0);
    }

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        MIMFS_CHECK_EQUAL(pthread_join(*(threads + index), NULL), 0);
        MIMFS_CHECK_EQUAL((*(contexts + index)).failure_count, 0);
    }

    /* The shared offset must be untouched by any positioned transfer. */
    MIMFS_CHECK_EQUAL(mimfs_seek(volume, shared_descriptor, 0, MIMFS_SEEK_FROM_CURRENT),
                      0);

    MIMFS_CHECK_EQUAL(mimfs_close(volume, shared_descriptor), MIMFS_STATUS_SUCCESS);

    printf("ok\n");
}

/* ---- directories and renames under contention --------------------------- */

static void *mimfs_test_directory_churn_worker(void *raw_context)
{
    struct mimfs_test_worker_context *context =
        (struct mimfs_test_worker_context *)raw_context;

    struct mimfs_volume *volume = (*context).volume;

    for (unsigned int iteration = 0u; iteration < (*context).iteration_count;
         iteration++)
    {
        char directory_path[80];
        char nested_path[160];
        char renamed_path[80];

        snprintf(directory_path, sizeof(directory_path), "/churn/dir-%u-%u",
                 (*context).worker_index, iteration);
        snprintf(renamed_path, sizeof(renamed_path), "/churn/moved-%u-%u",
                 (*context).worker_index, iteration);

        if (mimfs_create_directory(volume, directory_path, 0755) !=
            MIMFS_STATUS_SUCCESS)
        {
            (*context).failure_count++;
            continue;
        }

        snprintf(nested_path, sizeof(nested_path), "%s/leaf", directory_path);

        const int descriptor = mimfs_open(
            volume, nested_path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

        if (descriptor < 0)
        {
            (*context).failure_count++;
            continue;
        }

        (void)mimfs_close(volume, descriptor);

        if (mimfs_rename(volume, directory_path, renamed_path) != MIMFS_STATUS_SUCCESS)
        {
            (*context).failure_count++;
            continue;
        }

        /* The subtree must have travelled with the directory. */
        char moved_leaf_path[160];
        snprintf(moved_leaf_path, sizeof(moved_leaf_path), "%s/leaf", renamed_path);

        struct mimfs_status_information status;

        if (mimfs_get_status(volume, moved_leaf_path, &status) != MIMFS_STATUS_SUCCESS)
        {
            (*context).failure_count++;
            continue;
        }

        if (mimfs_remove_link(volume, moved_leaf_path) != MIMFS_STATUS_SUCCESS ||
            mimfs_remove_directory(volume, renamed_path) != MIMFS_STATUS_SUCCESS)
        {
            (*context).failure_count++;
            continue;
        }

        (*context).success_count++;
    }

    return NULL;
}

static void mimfs_test_parallel_directory_churn(void)
{
    mimfs_test_begin_case(
        "parallel directory creation, rename and removal is consistent");

    struct mimfs_volume *volume = mimfs_test_create_volume();
    MIMFS_CHECK_EQUAL(mimfs_create_directory(volume, "/churn", 0755),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_volume_statistics before_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &before_statistics),
                      MIMFS_STATUS_SUCCESS);

    struct mimfs_test_worker_context contexts[MIMFS_TEST_THREAD_COUNT];

    mimfs_test_run_workers(mimfs_test_directory_churn_worker, volume, 150u, contexts);

    unsigned long total_success_count = 0u;

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        MIMFS_CHECK_EQUAL((*(contexts + index)).failure_count, 0);
        total_success_count += (*(contexts + index)).success_count;
    }

    MIMFS_CHECK_EQUAL(total_success_count, 150u * MIMFS_TEST_THREAD_COUNT);

    struct mimfs_volume_statistics after_statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &after_statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(after_statistics.free_block_count,
                      before_statistics.free_block_count);
    MIMFS_CHECK_EQUAL(after_statistics.free_inode_count,
                      before_statistics.free_inode_count);

    /* The churn directory must be empty again, and therefore removable. */
    MIMFS_CHECK_EQUAL(mimfs_remove_directory(volume, "/churn"), MIMFS_STATUS_SUCCESS);

    printf("ok (%lu cycles across %u threads)\n", total_success_count,
           MIMFS_TEST_THREAD_COUNT);
}

/* ---- readers racing a remover ------------------------------------------ */

static void *mimfs_test_reader_versus_remover_worker(void *raw_context)
{
    struct mimfs_test_worker_context *context =
        (struct mimfs_test_worker_context *)raw_context;

    struct mimfs_volume *volume = (*context).volume;

    const bool this_worker_removes = (*context).worker_index == 0u;

    for (unsigned int iteration = 0u; iteration < (*context).iteration_count;
         iteration++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/volatile-%u", iteration % 32u);

        if (this_worker_removes)
        {
            /* Whether the name is there is a race, so both outcomes are acceptable;
             * what is not acceptable is any other status. */
            const int removal_status = mimfs_remove_link(volume, path);

            if (removal_status != MIMFS_STATUS_SUCCESS &&
                removal_status != -MIMFS_STATUS_NO_SUCH_ENTRY)
            {
                (*context).failure_count++;
            }

            continue;
        }

        const int descriptor =
            mimfs_open(volume, path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

        if (descriptor < 0)
        {
            (*context).failure_count++;
            continue;
        }

        /*
         * The file may be removed at any moment from under this thread. Every operation
         * through the descriptor must nevertheless keep working, because the descriptor
         * holds the inode alive.
         */
        char payload[128];
        memset(payload, 'q', sizeof(payload));

        if (mimfs_write_at_offset(volume, descriptor, payload, sizeof(payload), 0u) !=
            (long)sizeof(payload))
        {
            (*context).failure_count++;
        }

        char read_buffer[128];
        memset(read_buffer, 0, sizeof(read_buffer));

        if (mimfs_read_at_offset(volume, descriptor, read_buffer, sizeof(read_buffer),
                                 0u) != (long)sizeof(read_buffer) ||
            memcmp(read_buffer, payload, sizeof(payload)) != 0)
        {
            (*context).failure_count++;
        }

        struct mimfs_status_information status;

        if (mimfs_get_status_by_descriptor(volume, descriptor, &status) !=
            MIMFS_STATUS_SUCCESS)
        {
            (*context).failure_count++;
        }

        (void)mimfs_close(volume, descriptor);

        (*context).success_count++;
    }

    return NULL;
}

static void mimfs_test_readers_survive_a_concurrent_remover(void)
{
    mimfs_test_begin_case("an open file stays usable while its name is being removed");

    struct mimfs_volume *volume = mimfs_test_create_volume();

    struct mimfs_test_worker_context contexts[MIMFS_TEST_THREAD_COUNT];

    mimfs_test_run_workers(mimfs_test_reader_versus_remover_worker, volume, 800u,
                           contexts);

    for (unsigned int index = 0u; index < MIMFS_TEST_THREAD_COUNT; index++)
    {
        MIMFS_CHECK_EQUAL((*(contexts + index)).failure_count, 0);
    }

    /* Remove whatever survived, then check that the volume is fully reclaimed. */
    for (unsigned int index = 0u; index < 32u; index++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/volatile-%u", index);
        (void)mimfs_remove_link(volume, path);
    }

    struct mimfs_volume_statistics statistics;
    MIMFS_CHECK_EQUAL(mimfs_query_volume_statistics(volume, &statistics),
                      MIMFS_STATUS_SUCCESS);
    MIMFS_CHECK_EQUAL(statistics.open_file_count, 0);
    MIMFS_CHECK_EQUAL(statistics.total_inode_count - statistics.free_inode_count, 2);

    printf("ok\n");
}

void mimfs_test_run_concurrency_cases(void)
{
    printf("concurrency\n");

    mimfs_test_parallel_create_and_remove();
    mimfs_test_exclusive_creation_has_exactly_one_winner();
    mimfs_test_concurrent_appends_do_not_interleave();
    mimfs_test_positioned_transfers_are_independent();
    mimfs_test_parallel_directory_churn();
    mimfs_test_readers_survive_a_concurrent_remover();
}
