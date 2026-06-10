#include "task_trace.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "project_config.h"

/* ── Trace buffer (circular) ──────────────────────────────────────────── */
static task_trace_t trace_buf[TRACE_BUFFER_SIZE];
static volatile uint32_t trace_write_idx = 0;
static volatile uint32_t trace_count     = 0;

/* Per-task temporary release timestamp (set before start is called) */
static volatile uint32_t release_timestamp[TRACE_TASK_COUNT] = {0};

/* ── Run-time counter for FreeRTOS configGENERATE_RUN_TIME_STATS ──── */
uint32_t get_run_time_counter_value(void) {
    return time_us_32();
}

/* ── Recording functions ──────────────────────────────────────────────── */

void trace_record_release(trace_task_id_t id) {
    release_timestamp[id] = time_us_32();
}

void trace_set_release_from_isr(trace_task_id_t id) {
    release_timestamp[id] = time_us_32();
}

uint32_t trace_record_start(trace_task_id_t id) {
    uint32_t idx = trace_write_idx;

    trace_buf[idx].task_id    = (uint8_t)id;
    trace_buf[idx].release_us = release_timestamp[id];
    trace_buf[idx].start_us   = time_us_32();
    trace_buf[idx].end_us     = 0;

    trace_write_idx = (idx + 1) % TRACE_BUFFER_SIZE;
    if (trace_count < TRACE_BUFFER_SIZE) {
        trace_count++;
    }

    return idx;
}

void trace_record_end(uint32_t idx) {
    if (idx < TRACE_BUFFER_SIZE) {
        trace_buf[idx].end_us = time_us_32();
    }
}

/* ── Output functions ─────────────────────────────────────────────────── */

static const char *task_names[TRACE_TASK_COUNT] = {
    "Input", "Update", "Display", "Audio"
};

void trace_dump_usb(void) {
    uint32_t count = trace_count;
    if (count > TRACE_BUFFER_SIZE) count = TRACE_BUFFER_SIZE;

    /* Determine start index for the circular buffer */
    uint32_t start;
    if (trace_count <= TRACE_BUFFER_SIZE) {
        start = 0;
    } else {
        start = trace_write_idx;  /* oldest surviving entry */
    }

    printf("\n");
    printf("========================================\n");
    printf("  TASK TRACE DUMP  (%lu entries)\n", (unsigned long)count);
    printf("========================================\n");
    printf("task_id,task_name,release_us,start_us,end_us,exec_us,response_us\n");

    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = (start + i) % TRACE_BUFFER_SIZE;
        task_trace_t *t = &trace_buf[idx];

        uint32_t exec_us = t->end_us - t->start_us;
        uint32_t resp_us = t->end_us - t->release_us;

        printf("%u,%s,%lu,%lu,%lu,%lu,%lu\n",
               t->task_id,
               (t->task_id < TRACE_TASK_COUNT) ? task_names[t->task_id] : "?",
               (unsigned long)t->release_us,
               (unsigned long)t->start_us,
               (unsigned long)t->end_us,
               (unsigned long)exec_us,
               (unsigned long)resp_us);
    }

    printf("--- TRACE DUMP END ---\n\n");
}

void trace_dump_runtime_stats(void) {
    static char stats_buf[512];

    printf("========================================\n");
    printf("  FREERTOS RUNTIME STATS\n");
    printf("========================================\n");
    vTaskGetRunTimeStats(stats_buf);
    printf("%s\n", stats_buf);
}

void task_trace_dump(void *params) {
    (void)params;

    /* Let the system run and collect trace data */
    vTaskDelay(pdMS_TO_TICKS(TRACE_COLLECTION_MS));

    printf("\n[TRACE] Collection period (%d ms) complete. Dumping data...\n",
           TRACE_COLLECTION_MS);
    trace_dump_usb();
    trace_dump_runtime_stats();

    /* Self-delete after dump */
    vTaskDelete(NULL);
}
