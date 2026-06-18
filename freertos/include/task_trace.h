#ifndef TASK_TRACE_H
#define TASK_TRACE_H

#include <stdint.h>
#include <stdbool.h>  // For bool type

/* ── Task identifiers for the trace system ────────────────────────────── */
typedef enum {
    TRACE_TASK_INPUT   = 0,
    TRACE_TASK_UPDATE  = 1,
    TRACE_TASK_DISPLAY = 2,
    TRACE_TASK_AUDIO   = 3,
    TRACE_TASK_COUNT   = 4
} trace_task_id_t;

/* ── Per-job trace record ─────────────────────────────────────────────── */
typedef struct {
    uint8_t  task_id;       /* trace_task_id_t                              */
    uint32_t release_us;    /* time_us_32() when task was released/event    */
    uint32_t start_us;      /* time_us_32() when execution began            */
    uint32_t end_us;        /* time_us_32() when execution finished         */
} task_trace_t;

#define TRACE_BUFFER_SIZE  512

/* ── Recording API ────────────────────────────────────────────────────── */

/* Mark the release instant for a periodic task (call right after vTaskDelayUntil) */
void trace_record_release(trace_task_id_t id);

/* Mark the release instant from ISR context (for aperiodic tasks like audio) */
void trace_set_release_from_isr(trace_task_id_t id);

/* Allocate a trace slot, store release_us and start_us.  Returns slot index. */
uint32_t trace_record_start(trace_task_id_t id);

/* Store end_us for the given slot index */
void trace_record_end(uint32_t idx);

/* ── Output ───────────────────────────────────────────────────────────── */

/* Print all trace entries as CSV over USB serial */
void trace_dump_usb(void);

/* Print vTaskGetRunTimeStats output over USB serial */
void trace_dump_runtime_stats(void);

/* FreeRTOS task: waits TRACE_COLLECTION_MS, then dumps and self-deletes */
void task_trace_dump(void *params);

/* Run-time counter for FreeRTOS stats (portGET_RUN_TIME_COUNTER_VALUE) */
uint32_t get_run_time_counter_value(void);

/* ── Direct Buffer Access (for programmatic analysis) ─────────────────── */

/**
 * Get direct read-only access to the trace buffer
 * @param out_count Pointer to store the number of valid entries
 * @param out_start_idx Pointer to store the starting index (for circular buffer)
 * @return Pointer to the trace buffer array
 */
const task_trace_t* trace_get_buffer(uint32_t *out_count, uint32_t *out_start_idx);

/**
 * Get a specific trace entry by logical index (0 = oldest, count-1 = newest)
 * @param logical_idx Logical index (0 to count-1)
 * @param out_entry Pointer to store the trace entry
 * @return true if successful, false if index out of range
 */
bool trace_get_entry(uint32_t logical_idx, task_trace_t *out_entry);

/**
 * Calculate statistics for a specific task
 */
typedef struct {
    uint32_t count;              // Number of jobs recorded
    uint32_t min_exec_us;        // Minimum execution time
    uint32_t max_exec_us;        // Maximum execution time
    uint32_t avg_exec_us;        // Average execution time
    uint32_t min_response_us;    // Minimum response time
    uint32_t max_response_us;    // Maximum response time (WCRT)
    uint32_t avg_response_us;    // Average response time
} task_stats_t;

/**
 * Calculate statistics for a specific task from the trace buffer
 * @param task_id Task to analyze
 * @param out_stats Pointer to store calculated statistics
 */
void trace_calculate_stats(trace_task_id_t task_id, task_stats_t *out_stats);

#endif /* TASK_TRACE_H */
