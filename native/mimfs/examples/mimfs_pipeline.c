/*
 * MIMFS - Multithreaded In-Memory File System
 *
 * File:    examples/mimfs_pipeline.c
 * Purpose: A worked example of the concurrency contract: many threads producing
 *          files, many consuming them, and one collecting results, all against one
 *          volume with no locking in the application at all.
 *
 * Author:  Rahul Dange
 * Year:    2026
 * License: MIT. See the LICENSE file at the root of the distribution.
 *
 * The point of the example is what is absent from it. There is no mutex in this
 * file guarding the filesystem, no queue of pending operations, and no coordination
 * between the producers and the consumers beyond the filesystem itself. Two
 * properties of the interface make that possible:
 *
 *   * Exclusive creation has exactly one winner, so consumers claim work items
 *     without a shared work queue: whoever creates the claim marker owns the item.
 *   * Appending is atomic per write, so every consumer appends its result to one
 *     shared report file and no result is ever lost or torn.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mimfs/mimfs.h"

#define MIMFS_PIPELINE_REGION_SIZE_BYTES (64u * 1024u * 1024u)
#define MIMFS_PIPELINE_PRODUCER_COUNT    4u
#define MIMFS_PIPELINE_CONSUMER_COUNT    4u
#define MIMFS_PIPELINE_ITEM_COUNT        2000u
#define MIMFS_PIPELINE_PAYLOAD_SIZE      512u

static unsigned char mimfs_pipeline_memory_region[MIMFS_PIPELINE_REGION_SIZE_BYTES];

struct mimfs_pipeline_worker
{
    struct mimfs_volume *volume;
    unsigned int worker_index;

    unsigned long items_handled;
    unsigned long items_lost_to_another_worker;
    unsigned long errors_encountered;
};

/* Path of the work item with the given number. */
static void mimfs_pipeline_build_item_path(char *path_buffer, size_t buffer_size,
                                           unsigned int item_number)
{
    (void)snprintf(path_buffer, buffer_size, "/queue/item-%05u", item_number);
}

/* Path of the marker that records the item as claimed. */
static void mimfs_pipeline_build_claim_path(char *path_buffer, size_t buffer_size,
                                            unsigned int item_number)
{
    (void)snprintf(path_buffer, buffer_size, "/claimed/item-%05u", item_number);
}

/* ---- producers ---------------------------------------------------------- */

static void *mimfs_pipeline_producer(void *raw_worker)
{
    struct mimfs_pipeline_worker *worker = (struct mimfs_pipeline_worker *)raw_worker;

    struct mimfs_volume *volume = (*worker).volume;

    /*
     * Each producer owns a stripe of the item numbers, so producers never contend for
     * the same name. The directory they all write into is shared, and the filesystem
     * serialises the insertions into it without the application knowing.
     */
    for (unsigned int item_number = (*worker).worker_index;
         item_number < MIMFS_PIPELINE_ITEM_COUNT;
         item_number += MIMFS_PIPELINE_PRODUCER_COUNT)
    {
        char item_path[128];
        mimfs_pipeline_build_item_path(item_path, sizeof(item_path), item_number);

        const int descriptor = mimfs_open(
            volume, item_path, MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

        if (descriptor < 0)
        {
            (*worker).errors_encountered++;
            continue;
        }

        char payload[MIMFS_PIPELINE_PAYLOAD_SIZE];
        memset(payload, (int)('0' + (item_number % 10u)), sizeof(payload));

        if (mimfs_write(volume, descriptor, payload, sizeof(payload)) !=
            (long)sizeof(payload))
        {
            (*worker).errors_encountered++;
        }
        else
        {
            (*worker).items_handled++;
        }

        (void)mimfs_close(volume, descriptor);
    }

    return NULL;
}

/* ---- consumers ---------------------------------------------------------- */

static void *mimfs_pipeline_consumer(void *raw_worker)
{
    struct mimfs_pipeline_worker *worker = (struct mimfs_pipeline_worker *)raw_worker;

    struct mimfs_volume *volume = (*worker).volume;

    /* Every consumer appends to the same report through its own descriptor. The
     * append flag is what makes the writes land end to end rather than on top of one
     * another. */
    const int report_descriptor =
        mimfs_open(volume, "/report", MIMFS_OPEN_READ_WRITE | MIMFS_OPEN_APPEND, 0);

    if (report_descriptor < 0)
    {
        (*worker).errors_encountered++;
        return NULL;
    }

    /*
     * Every consumer sweeps the whole item range. Which consumer gets which item is
     * decided by exclusive creation of the claim marker: exactly one call succeeds per
     * item, and every other consumer is told the name already exists and moves on.
     * That is the whole synchronisation mechanism.
     */
    for (unsigned int item_number = 0u; item_number < MIMFS_PIPELINE_ITEM_COUNT;
         item_number++)
    {
        char claim_path[128];
        mimfs_pipeline_build_claim_path(claim_path, sizeof(claim_path), item_number);

        const int claim_descriptor = mimfs_open(
            volume, claim_path,
            MIMFS_OPEN_CREATE | MIMFS_OPEN_EXCLUSIVE | MIMFS_OPEN_WRITE_ONLY, 0644);

        if (claim_descriptor == -MIMFS_STATUS_ALREADY_EXISTS)
        {
            (*worker).items_lost_to_another_worker++;
            continue;
        }

        if (claim_descriptor < 0)
        {
            (*worker).errors_encountered++;
            continue;
        }

        (void)mimfs_close(volume, claim_descriptor);

        /* This consumer now owns the item. Read it and verify the producer's mark. */
        char item_path[128];
        mimfs_pipeline_build_item_path(item_path, sizeof(item_path), item_number);

        const int item_descriptor =
            mimfs_open(volume, item_path, MIMFS_OPEN_READ_ONLY, 0);

        if (item_descriptor < 0)
        {
            (*worker).errors_encountered++;
            continue;
        }

        char payload[MIMFS_PIPELINE_PAYLOAD_SIZE];
        const long bytes_read =
            mimfs_read(volume, item_descriptor, payload, sizeof(payload));

        (void)mimfs_close(volume, item_descriptor);

        const char expected_mark = (char)('0' + (item_number % 10u));
        bool payload_is_intact = bytes_read == (long)sizeof(payload);

        for (size_t byte_index = 0u; payload_is_intact && byte_index < sizeof(payload);
             byte_index++)
        {
            payload_is_intact = *(payload + byte_index) == expected_mark;
        }

        if (!payload_is_intact)
        {
            (*worker).errors_encountered++;
            continue;
        }

        /* Report the result. One append, one whole record, no interleaving. */
        char report_record[64];
        const int record_length = snprintf(report_record, sizeof(report_record),
                                           "consumer %u handled item %05u\n",
                                           (*worker).worker_index, item_number);

        if (mimfs_write(volume, report_descriptor, report_record,
                        (size_t)record_length) != record_length)
        {
            (*worker).errors_encountered++;
            continue;
        }

        (*worker).items_handled++;
    }

    (void)mimfs_close(volume, report_descriptor);

    return NULL;
}

/* ---- verification ------------------------------------------------------- */

/* Count the lines of the report, which must equal the number of items handled. */
static unsigned long mimfs_pipeline_count_report_lines(struct mimfs_volume *volume)
{
    const int descriptor = mimfs_open(volume, "/report", MIMFS_OPEN_READ_ONLY, 0);

    if (descriptor < 0)
    {
        return 0u;
    }

    unsigned long line_count = 0u;
    char transfer_buffer[8192];
    long bytes_read;

    while ((bytes_read = mimfs_read(volume, descriptor, transfer_buffer,
                                    sizeof(transfer_buffer))) > 0)
    {
        for (long byte_index = 0; byte_index < bytes_read; byte_index++)
        {
            if (*(transfer_buffer + byte_index) == '\n')
            {
                line_count++;
            }
        }
    }

    (void)mimfs_close(volume, descriptor);

    return line_count;
}

static unsigned long mimfs_pipeline_count_directory_entries(struct mimfs_volume *volume,
                                                            const char *path)
{
    const int stream = mimfs_open_directory(volume, path);

    if (stream < 0)
    {
        return 0u;
    }

    unsigned long entry_count = 0u;
    struct mimfs_directory_entry_information entry;

    while (mimfs_read_directory(volume, stream, &entry) == 1)
    {
        if (strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0)
        {
            entry_count++;
        }
    }

    (void)mimfs_close_directory(volume, stream);

    return entry_count;
}

int main(void)
{
    int format_status = MIMFS_STATUS_SUCCESS;

    struct mimfs_volume *volume =
        mimfs_format(mimfs_pipeline_memory_region, sizeof(mimfs_pipeline_memory_region),
                     NULL, &format_status);

    if (volume == NULL)
    {
        printf("unable to format the volume: %s\n",
               mimfs_describe_status(format_status));
        return 1;
    }

    printf("MIMFS %s concurrent pipeline example\n\n", mimfs_library_version_string());
    printf("  %u producers, %u consumers, %u items of %u bytes\n\n",
           MIMFS_PIPELINE_PRODUCER_COUNT, MIMFS_PIPELINE_CONSUMER_COUNT,
           MIMFS_PIPELINE_ITEM_COUNT, MIMFS_PIPELINE_PAYLOAD_SIZE);

    if (mimfs_create_directory(volume, "/queue", 0755) < 0 ||
        mimfs_create_directory(volume, "/claimed", 0755) < 0)
    {
        printf("unable to prepare the directories\n");
        return 1;
    }

    const int report_descriptor =
        mimfs_open(volume, "/report", MIMFS_OPEN_CREATE | MIMFS_OPEN_READ_WRITE, 0644);

    if (report_descriptor < 0)
    {
        printf("unable to create the report\n");
        return 1;
    }

    (void)mimfs_close(volume, report_descriptor);

    /* ---- produce ------------------------------------------------------- */

    pthread_t producer_threads[MIMFS_PIPELINE_PRODUCER_COUNT];
    struct mimfs_pipeline_worker producers[MIMFS_PIPELINE_PRODUCER_COUNT];

    memset(producers, 0, sizeof(producers));

    for (unsigned int index = 0u; index < MIMFS_PIPELINE_PRODUCER_COUNT; index++)
    {
        (*(producers + index)).volume = volume;
        (*(producers + index)).worker_index = index;

        if (pthread_create(producer_threads + index, NULL, mimfs_pipeline_producer,
                           producers + index) != 0)
        {
            printf("unable to start producer %u\n", index);
            return 1;
        }
    }

    for (unsigned int index = 0u; index < MIMFS_PIPELINE_PRODUCER_COUNT; index++)
    {
        (void)pthread_join(*(producer_threads + index), NULL);
    }

    unsigned long items_produced = 0u;
    unsigned long producer_errors = 0u;

    for (unsigned int index = 0u; index < MIMFS_PIPELINE_PRODUCER_COUNT; index++)
    {
        items_produced += (*(producers + index)).items_handled;
        producer_errors += (*(producers + index)).errors_encountered;
    }

    printf("  produced   %lu items, %lu errors\n", items_produced, producer_errors);

    /* ---- consume ------------------------------------------------------- */

    pthread_t consumer_threads[MIMFS_PIPELINE_CONSUMER_COUNT];
    struct mimfs_pipeline_worker consumers[MIMFS_PIPELINE_CONSUMER_COUNT];

    memset(consumers, 0, sizeof(consumers));

    for (unsigned int index = 0u; index < MIMFS_PIPELINE_CONSUMER_COUNT; index++)
    {
        (*(consumers + index)).volume = volume;
        (*(consumers + index)).worker_index = index;

        if (pthread_create(consumer_threads + index, NULL, mimfs_pipeline_consumer,
                           consumers + index) != 0)
        {
            printf("unable to start consumer %u\n", index);
            return 1;
        }
    }

    for (unsigned int index = 0u; index < MIMFS_PIPELINE_CONSUMER_COUNT; index++)
    {
        (void)pthread_join(*(consumer_threads + index), NULL);
    }

    unsigned long items_consumed = 0u;
    unsigned long items_contested = 0u;
    unsigned long consumer_errors = 0u;

    for (unsigned int index = 0u; index < MIMFS_PIPELINE_CONSUMER_COUNT; index++)
    {
        printf("  consumer %u handled %lu items\n", index,
               (*(consumers + index)).items_handled);

        items_consumed += (*(consumers + index)).items_handled;
        items_contested += (*(consumers + index)).items_lost_to_another_worker;
        consumer_errors += (*(consumers + index)).errors_encountered;
    }

    /* ---- verify -------------------------------------------------------- */

    const unsigned long report_line_count = mimfs_pipeline_count_report_lines(volume);
    const unsigned long queue_entry_count =
        mimfs_pipeline_count_directory_entries(volume, "/queue");
    const unsigned long claim_entry_count =
        mimfs_pipeline_count_directory_entries(volume, "/claimed");

    printf(
        "\n  consumed   %lu items, %lu claims lost to another consumer, %lu errors\n",
        items_consumed, items_contested, consumer_errors);
    printf("  report     %lu lines\n", report_line_count);
    printf("  queue      %lu entries\n", queue_entry_count);
    printf("  claims     %lu entries\n", claim_entry_count);

    struct mimfs_volume_statistics statistics;
    (void)mimfs_query_volume_statistics(volume, &statistics);

    printf("  volume     %llu of %llu blocks used, %u descriptors open\n\n",
           (unsigned long long)(statistics.total_block_count -
                                statistics.free_block_count),
           (unsigned long long)statistics.total_block_count,
           statistics.open_file_count);

    /*
     * The four invariants this example exists to demonstrate. Every one of them is a
     * consequence of the filesystem's guarantees rather than of anything this program
     * does.
     */
    const bool every_item_was_produced = items_produced == MIMFS_PIPELINE_ITEM_COUNT;
    const bool every_item_was_consumed_once =
        items_consumed == MIMFS_PIPELINE_ITEM_COUNT;
    const bool every_result_was_recorded = report_line_count == items_consumed;
    const bool nothing_went_wrong = producer_errors == 0u && consumer_errors == 0u;

    printf("  every item produced          %s\n",
           every_item_was_produced ? "yes" : "no");
    printf("  every item consumed exactly once %s\n",
           every_item_was_consumed_once ? "yes" : "no");
    printf("  every result recorded        %s\n",
           every_result_was_recorded ? "yes" : "no");
    printf("  no errors                    %s\n", nothing_went_wrong ? "yes" : "no");

    (void)mimfs_unmount(volume);

    const bool everything_held = every_item_was_produced &&
                                 every_item_was_consumed_once &&
                                 every_result_was_recorded && nothing_went_wrong;

    printf("\n%s\n",
           everything_held ? "all invariants held" : "an invariant was violated");

    return everything_held ? 0 : 1;
}
