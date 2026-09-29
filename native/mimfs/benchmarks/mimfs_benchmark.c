/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    benchmarks/mimfs_benchmark.c
 * Purpose: Throughput and scaling measurements.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * What is measured, and why each measurement is here:
 *
 *   sequential transfer   the cost of the block map walk and the copy, which is what
 *                         a bulk reader or writer pays
 *   small transfer        the same path with the copy made negligible, which isolates
 *                         the per-call overhead
 *   metadata operations   create and remove, which is the directory insertion and the
 *                         allocator round trip
 *   path resolution       lookup cost against directory size and path depth, the two
 *                         things a linear directory scan is sensitive to
 *   parallel scaling      the same work spread over more threads, which is the only
 *                         measurement that shows whether the locking is granular
 *
 * Every figure is a rate rather than a duration, so that a change in the iteration
 * counts does not change how a result reads.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mimfs/mimfs.h"

#define MIMFS_BENCHMARK_REGION_SIZE_BYTES    (512u * 1024u * 1024u)
#define MIMFS_BENCHMARK_MAXIMUM_THREAD_COUNT 16u

static unsigned char mimfs_benchmark_memory_region[MIMFS_BENCHMARK_REGION_SIZE_BYTES];

/* A monotonic clock, because a benchmark measures elapsed time and must not be
 * affected by the wall clock being stepped mid-run. */
static double mimfs_benchmark_current_time_seconds(void)
{
    struct timespec current_time;

    if (clock_gettime(CLOCK_MONOTONIC, &current_time) != 0)
    {
        return 0.0;
    }

    return (double)current_time.tv_sec + (double)current_time.tv_nsec / 1e9;
}

static struct mimfs_volume *mimfs_benchmark_fresh_volume(void)
{
    struct mimfs_format_parameters parameters;

    parameters.bytes_per_inode_ratio = 16384u;
    parameters.maximum_open_file_count = 4096u;
    parameters.reserved_block_percentage = 0u;
    parameters.blocks_per_block_group = 0u;

    int format_status = MIMFS_STATUS_SUCCESS;

    struct mimfs_volume *volume = mimfs_format(mimfs_benchmark_memory_region,
                                               sizeof(mimfs_benchmark_memory_region),
                                               &parameters, &format_status);

    if (volume == NULL)
    {
        printf("unable to format the volume: %s\n",
               mimfs_describe_status(format_status));
        exit(1);
    }

    return volume;
}

static void mimfs_benchmark_print_heading(const char *heading)
{
    printf("\n%s\n", heading);
}

static void mimfs_benchmark_print_throughput(const char *label, double byte_count,
                                             double elapsed_seconds)
{
    const double megabytes_per_second =
        elapsed_seconds > 0.0 ? byte_count / elapsed_seconds / (1024.0 * 1024.0) : 0.0;

    printf("  %-44s %10.1f MiB/s\n", label, megabytes_per_second);
}

static void mimfs_benchmark_print_rate(const char *label, double operation_count,
                                       double elapsed_seconds)
{
    const double operations_per_second =
        elapsed_seconds > 0.0 ? operation_count / elapsed_seconds : 0.0;

    printf("  %-44s %10.0f per second\n", label, operations_per_second);
}

/* =========================================================================
 * Sequential transfer
 * ========================================================================= */

static void mimfs_benchmark_sequential_transfer(void)
{
    mimfs_benchmark_print_heading("sequential transfer, one thread");

    struct mimfs_volume *volume = mimfs_benchmark_fresh_volume();

    static char transfer_buffer[1024u * 1024u];
    memset(transfer_buffer, 'B', sizeof(transfer_buffer));

    /* Two hundred megabytes, comfortably past every cache on the machine, so the
     * figure reflects memory bandwidth rather than cache bandwidth. */
    const unsigned int transfer_count = 200u;

    const int descriptor =
        mimfs_open(volume, "/bulk", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

    if (descriptor < 0)
    {
        printf("  unable to open the file\n");
        return;
    }

    double start_time = mimfs_benchmark_current_time_seconds();

    for (unsigned int index = 0u; index < transfer_count; index++)
    {
        if (mimfs_write(volume, descriptor, transfer_buffer, sizeof(transfer_buffer)) !=
            (long)sizeof(transfer_buffer))
        {
            printf("  the volume ran out of space after %u transfers\n", index);
            break;
        }
    }

    double elapsed_seconds = mimfs_benchmark_current_time_seconds() - start_time;

    mimfs_benchmark_print_throughput(
        "write, one mebibyte at a time",
        (double)transfer_count * (double)sizeof(transfer_buffer), elapsed_seconds);

    (void)mimfs_seek(volume, descriptor, 0, MIMFS_SEEK_FROM_START);

    start_time = mimfs_benchmark_current_time_seconds();

    for (unsigned int index = 0u; index < transfer_count; index++)
    {
        if (mimfs_read(volume, descriptor, transfer_buffer, sizeof(transfer_buffer)) <=
            0)
        {
            break;
        }
    }

    elapsed_seconds = mimfs_benchmark_current_time_seconds() - start_time;

    mimfs_benchmark_print_throughput(
        "read, one mebibyte at a time",
        (double)transfer_count * (double)sizeof(transfer_buffer), elapsed_seconds);

    /* The same volume of data moved in small pieces, which shifts the cost from the
     * copy to the per-call path: descriptor lookup, offset claim, lock, block map walk.
     */
    (void)mimfs_seek(volume, descriptor, 0, MIMFS_SEEK_FROM_START);

    const unsigned int small_transfer_count = 2u * 1024u * 1024u;
    const size_t small_transfer_size = 64u;

    start_time = mimfs_benchmark_current_time_seconds();

    for (unsigned int index = 0u; index < small_transfer_count; index++)
    {
        if (mimfs_read(volume, descriptor, transfer_buffer, small_transfer_size) <= 0)
        {
            (void)mimfs_seek(volume, descriptor, 0, MIMFS_SEEK_FROM_START);
        }
    }

    elapsed_seconds = mimfs_benchmark_current_time_seconds() - start_time;

    mimfs_benchmark_print_rate("read, sixty four bytes at a time",
                               (double)small_transfer_count, elapsed_seconds);
    mimfs_benchmark_print_throughput(
        "read, sixty four bytes at a time",
        (double)small_transfer_count * (double)small_transfer_size, elapsed_seconds);

    (void)mimfs_close(volume, descriptor);
    (void)mimfs_unmount(volume);
}

/* =========================================================================
 * Metadata operations
 * ========================================================================= */

static void mimfs_benchmark_metadata_operations(void)
{
    mimfs_benchmark_print_heading("metadata operations, one thread");

    struct mimfs_volume *volume = mimfs_benchmark_fresh_volume();

    (void)mimfs_create_directory(volume, "/bench", 0755);

    const unsigned int file_count = 20000u;

    double start_time = mimfs_benchmark_current_time_seconds();

    for (unsigned int index = 0u; index < file_count; index++)
    {
        char path[80];
        (void)snprintf(path, sizeof(path), "/bench/file-%06u", index);

        const int descriptor =
            mimfs_open(volume, path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

        if (descriptor < 0)
        {
            printf("  create failed at %u: %s\n", index,
                   mimfs_describe_status(descriptor));
            break;
        }

        (void)mimfs_close(volume, descriptor);
    }

    double elapsed_seconds = mimfs_benchmark_current_time_seconds() - start_time;

    mimfs_benchmark_print_rate("create and close an empty file", (double)file_count,
                               elapsed_seconds);

    /* Look up the last file created, repeatedly. In a directory of this size the scan
     * has to walk every block, which is the worst case for a linear directory. */
    char deepest_path[80];
    (void)snprintf(deepest_path, sizeof(deepest_path), "/bench/file-%06u",
                   file_count - 1u);

    const unsigned int lookup_count = 20000u;

    start_time = mimfs_benchmark_current_time_seconds();

    for (unsigned int index = 0u; index < lookup_count; index++)
    {
        struct mimfs_status_information status;

        if (mimfs_get_status(volume, deepest_path, &status) < 0)
        {
            break;
        }
    }

    elapsed_seconds = mimfs_benchmark_current_time_seconds() - start_time;

    printf("  %-44s %10.0f per second\n", "resolve a path in a directory of 20000",
           (double)lookup_count / (elapsed_seconds > 0.0 ? elapsed_seconds : 1.0));

    /* And in a directory holding almost nothing, which is the common case. */
    (void)mimfs_create_directory(volume, "/small", 0755);

    const int single_descriptor = mimfs_open(
        volume, "/small/only", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);
    (void)mimfs_close(volume, single_descriptor);

    start_time = mimfs_benchmark_current_time_seconds();

    for (unsigned int index = 0u; index < lookup_count; index++)
    {
        struct mimfs_status_information status;

        if (mimfs_get_status(volume, "/small/only", &status) < 0)
        {
            break;
        }
    }

    elapsed_seconds = mimfs_benchmark_current_time_seconds() - start_time;

    mimfs_benchmark_print_rate("resolve a path in a directory of one",
                               (double)lookup_count, elapsed_seconds);

    start_time = mimfs_benchmark_current_time_seconds();

    for (unsigned int index = 0u; index < file_count; index++)
    {
        char path[80];
        (void)snprintf(path, sizeof(path), "/bench/file-%06u", index);

        if (mimfs_remove_link(volume, path) < 0)
        {
            break;
        }
    }

    elapsed_seconds = mimfs_benchmark_current_time_seconds() - start_time;

    mimfs_benchmark_print_rate("remove a file", (double)file_count, elapsed_seconds);

    (void)mimfs_unmount(volume);
}

/* =========================================================================
 * Parallel scaling
 * ========================================================================= */

struct mimfs_benchmark_scaling_worker
{
    struct mimfs_volume *volume;
    unsigned int worker_index;
    unsigned int operation_count;
    bool operate_on_a_shared_directory;
};

/* Create, write, read and remove a private file, repeatedly. */
static void *mimfs_benchmark_scaling_worker_body(void *raw_worker)
{
    struct mimfs_benchmark_scaling_worker *worker =
        (struct mimfs_benchmark_scaling_worker *)raw_worker;

    struct mimfs_volume *volume = (*worker).volume;

    char payload[4096];
    memset(payload, 'S', sizeof(payload));

    for (unsigned int index = 0u; index < (*worker).operation_count; index++)
    {
        char path[96];

        if ((*worker).operate_on_a_shared_directory)
        {
            (void)snprintf(path, sizeof(path), "/shared/w%u-i%u",
                           (*worker).worker_index, index);
        }
        else
        {
            (void)snprintf(path, sizeof(path), "/private-%u/i%u",
                           (*worker).worker_index, index);
        }

        const int descriptor =
            mimfs_open(volume, path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

        if (descriptor < 0)
        {
            continue;
        }

        (void)mimfs_write(volume, descriptor, payload, sizeof(payload));
        (void)mimfs_read_at_offset(volume, descriptor, payload, sizeof(payload), 0u);
        (void)mimfs_close(volume, descriptor);
        (void)mimfs_remove_link(volume, path);
    }

    return NULL;
}

static double mimfs_benchmark_measure_scaling(struct mimfs_volume *volume,
                                              unsigned int thread_count,
                                              unsigned int operations_per_thread,
                                              bool operate_on_a_shared_directory)
{
    pthread_t threads[MIMFS_BENCHMARK_MAXIMUM_THREAD_COUNT];
    struct mimfs_benchmark_scaling_worker workers[MIMFS_BENCHMARK_MAXIMUM_THREAD_COUNT];

    const double start_time = mimfs_benchmark_current_time_seconds();

    for (unsigned int index = 0u; index < thread_count; index++)
    {
        struct mimfs_benchmark_scaling_worker *worker = workers + index;

        (*worker).volume = volume;
        (*worker).worker_index = index;
        (*worker).operation_count = operations_per_thread;
        (*worker).operate_on_a_shared_directory = operate_on_a_shared_directory;

        if (pthread_create(threads + index, NULL, mimfs_benchmark_scaling_worker_body,
                           worker) != 0)
        {
            return 0.0;
        }
    }

    for (unsigned int index = 0u; index < thread_count; index++)
    {
        (void)pthread_join(*(threads + index), NULL);
    }

    const double elapsed_seconds = mimfs_benchmark_current_time_seconds() - start_time;

    return (double)thread_count * (double)operations_per_thread /
           (elapsed_seconds > 0.0 ? elapsed_seconds : 1.0);
}

static void mimfs_benchmark_parallel_scaling(void)
{
    mimfs_benchmark_print_heading(
        "parallel scaling, one create, write, read and remove per operation");

    struct mimfs_volume *volume = mimfs_benchmark_fresh_volume();

    (void)mimfs_create_directory(volume, "/shared", 0755);

    for (unsigned int index = 0u; index < MIMFS_BENCHMARK_MAXIMUM_THREAD_COUNT; index++)
    {
        char path[64];
        (void)snprintf(path, sizeof(path), "/private-%u", index);
        (void)mimfs_create_directory(volume, path, 0755);
    }

    const unsigned int thread_counts_to_measure[] = { 1u, 2u, 4u, 8u, 16u };
    const size_t measurement_count =
        sizeof(thread_counts_to_measure) / sizeof(thread_counts_to_measure[0]);

    const unsigned int operations_per_thread = 4000u;

    double single_thread_rate_private = 0.0;
    double single_thread_rate_shared = 0.0;

    printf("  %-10s %16s %10s %16s %10s\n", "threads", "private dir", "scaling",
           "shared dir", "scaling");

    for (size_t measurement_index = 0u; measurement_index < measurement_count;
         measurement_index++)
    {
        const unsigned int thread_count =
            *(thread_counts_to_measure + measurement_index);

        /*
         * Two arrangements, because they separate two different costs. With a private
         * directory per thread, nothing is shared but the allocators, so the figure
         * shows whether allocation scales. With one shared directory, every thread also
         * contends for that directory's lock, so the figure shows the cost of the
         * serialisation that a real workload on one hot directory would pay.
         */
        const double private_directory_rate = mimfs_benchmark_measure_scaling(
            volume, thread_count, operations_per_thread, false);

        const double shared_directory_rate = mimfs_benchmark_measure_scaling(
            volume, thread_count, operations_per_thread, true);

        if (measurement_index == 0u)
        {
            single_thread_rate_private = private_directory_rate;
            single_thread_rate_shared = shared_directory_rate;
        }

        printf("  %-10u %13.0f/s %9.2fx %13.0f/s %9.2fx\n", thread_count,
               private_directory_rate,
               single_thread_rate_private > 0.0
                   ? private_directory_rate / single_thread_rate_private
                   : 0.0,
               shared_directory_rate,
               single_thread_rate_shared > 0.0
                   ? shared_directory_rate / single_thread_rate_shared
                   : 0.0);
    }

    (void)mimfs_unmount(volume);
}

/* =========================================================================
 * Parallel bulk transfer
 * ========================================================================= */

struct mimfs_benchmark_transfer_worker
{
    struct mimfs_volume *volume;
    unsigned int worker_index;
    unsigned int megabytes_to_write;
};

static void *mimfs_benchmark_transfer_worker_body(void *raw_worker)
{
    struct mimfs_benchmark_transfer_worker *worker =
        (struct mimfs_benchmark_transfer_worker *)raw_worker;

    char path[64];
    (void)snprintf(path, sizeof(path), "/stream-%u", (*worker).worker_index);

    const int descriptor = mimfs_open((*worker).volume, path,
                                      MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

    if (descriptor < 0)
    {
        return NULL;
    }

    static char shared_payload[256u * 1024u];

    for (unsigned int index = 0u; index < (*worker).megabytes_to_write * 4u; index++)
    {
        if (mimfs_write((*worker).volume, descriptor, shared_payload,
                        sizeof(shared_payload)) <= 0)
        {
            break;
        }
    }

    (void)mimfs_close((*worker).volume, descriptor);

    return NULL;
}

static void mimfs_benchmark_parallel_transfer(void)
{
    mimfs_benchmark_print_heading("parallel bulk write, one file per thread");

    const unsigned int thread_counts_to_measure[] = { 1u, 2u, 4u, 8u };
    const size_t measurement_count =
        sizeof(thread_counts_to_measure) / sizeof(thread_counts_to_measure[0]);

    const unsigned int megabytes_per_thread = 24u;

    printf("  %-10s %16s %10s\n", "threads", "aggregate", "scaling");

    double single_thread_rate = 0.0;

    for (size_t measurement_index = 0u; measurement_index < measurement_count;
         measurement_index++)
    {
        const unsigned int thread_count =
            *(thread_counts_to_measure + measurement_index);

        /* A fresh volume per measurement, so that one run does not leave the next one
         * allocating out of a fragmented free pool. */
        struct mimfs_volume *volume = mimfs_benchmark_fresh_volume();

        pthread_t threads[MIMFS_BENCHMARK_MAXIMUM_THREAD_COUNT];
        struct mimfs_benchmark_transfer_worker
            workers[MIMFS_BENCHMARK_MAXIMUM_THREAD_COUNT];

        const double start_time = mimfs_benchmark_current_time_seconds();

        for (unsigned int index = 0u; index < thread_count; index++)
        {
            struct mimfs_benchmark_transfer_worker *worker = workers + index;

            (*worker).volume = volume;
            (*worker).worker_index = index;
            (*worker).megabytes_to_write = megabytes_per_thread;

            (void)pthread_create(threads + index, NULL,
                                 mimfs_benchmark_transfer_worker_body, worker);
        }

        for (unsigned int index = 0u; index < thread_count; index++)
        {
            (void)pthread_join(*(threads + index), NULL);
        }

        const double elapsed_seconds =
            mimfs_benchmark_current_time_seconds() - start_time;

        const double megabytes_per_second =
            (double)thread_count * (double)megabytes_per_thread /
            (elapsed_seconds > 0.0 ? elapsed_seconds : 1.0);

        if (measurement_index == 0u)
        {
            single_thread_rate = megabytes_per_second;
        }

        printf("  %-10u %11.1f MiB/s %9.2fx\n", thread_count, megabytes_per_second,
               single_thread_rate > 0.0 ? megabytes_per_second / single_thread_rate
                                        : 0.0);

        (void)mimfs_unmount(volume);
    }
}

int main(void)
{
    printf("MIMFS %s benchmark\n", mimfs_library_version_string());
    printf("  volume %u MiB, block %u bytes, inode %u bytes\n",
           (unsigned int)(sizeof(mimfs_benchmark_memory_region) / (1024u * 1024u)),
           MIMFS_BLOCK_SIZE_BYTES, MIMFS_INODE_SIZE_BYTES);

    mimfs_benchmark_sequential_transfer();
    mimfs_benchmark_metadata_operations();
    mimfs_benchmark_parallel_scaling();
    mimfs_benchmark_parallel_transfer();

    printf("\n");

    return 0;
}
