#ifndef TASK_TRACE_H
#define TASK_TRACE_H

#include <stdint.h>

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

#endif /* TASK_TRACE_H */
