/**
 * @file analysis_task.c
 * @brief Custom trace analysis task - shows clean statistics instead of raw CSV
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "project_config.h"
#include "task_trace.h"

/**
 * Task that waits for trace collection, then shows summary statistics
 * Use this instead of task_trace_dump for cleaner output
 */
void task_analysis_summary(void *params) {
    (void)params;

    /* Wait for data collection period */
    vTaskDelay(pdMS_TO_TICKS(TRACE_COLLECTION_MS));

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║          TIMING ANALYSIS RESULTS (%d sec)              ║\n", TRACE_COLLECTION_MS / 1000);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    const char *task_names[] = {"Input", "Update", "Display", "Audio"};
    const uint32_t deadlines_us[] = {20000, 33000, 33000, 1000};  /* Task deadlines */

    /* Analyze each task */
    for (int i = 0; i < TRACE_TASK_COUNT; i++) {
        task_stats_t stats;
        trace_calculate_stats(i, &stats);

        if (stats.count == 0) {
            printf("Task: %-8s — No data collected\n\n", task_names[i]);
            continue;
        }

        printf("Task: %-8s (%lu jobs recorded)\n", task_names[i], (unsigned long)stats.count);
        printf("├─ Execution Time:\n");
        printf("│  ├─ Min:  %6lu µs\n", (unsigned long)stats.min_exec_us);
        printf("│  ├─ Avg:  %6lu µs\n", (unsigned long)stats.avg_exec_us);
        printf("│  └─ Max:  %6lu µs  ← WCET (Worst-Case Execution Time)\n", (unsigned long)stats.max_exec_us);
        printf("│\n");
        printf("├─ Response Time:\n");
        printf("│  ├─ Min:  %6lu µs\n", (unsigned long)stats.min_response_us);
        printf("│  ├─ Avg:  %6lu µs\n", (unsigned long)stats.avg_response_us);
        printf("│  └─ Max:  %6lu µs  ← WCRT (Worst-Case Response Time)\n", (unsigned long)stats.max_response_us);
        printf("│\n");
        printf("└─ Deadline: %lu µs (%lu ms) — ", 
               (unsigned long)deadlines_us[i], 
               (unsigned long)(deadlines_us[i] / 1000));
        
        if (stats.max_response_us <= deadlines_us[i]) {
            printf("✓ MET\n");
        } else {
            printf("✗ MISSED (by %lu µs)\n", 
                   (unsigned long)(stats.max_response_us - deadlines_us[i]));
        }
        printf("\n");
    }

    /* Show FreeRTOS runtime stats */
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                  FREERTOS CPU USAGE                        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    trace_dump_runtime_stats();

    printf("\n[ANALYSIS] Complete. Task will now terminate.\n");

    /* Self-delete after analysis */
    vTaskDelete(NULL);
}

/**
 * Alternative: Show both statistics AND raw CSV data
 */
void task_analysis_full(void *params) {
    (void)params;

    /* Wait for data collection */
    vTaskDelay(pdMS_TO_TICKS(TRACE_COLLECTION_MS));

    printf("\n[ANALYSIS] Collection complete. Generating report...\n\n");

    /* First show summary statistics */
    const char *task_names[] = {"Input", "Update", "Display", "Audio"};
    
    printf("=== SUMMARY STATISTICS ===\n\n");
    for (int i = 0; i < TRACE_TASK_COUNT; i++) {
        task_stats_t stats;
        trace_calculate_stats(i, &stats);
        
        printf("%s: Jobs=%lu WCET=%lu µs WCRT=%lu µs\n",
               task_names[i],
               (unsigned long)stats.count,
               (unsigned long)stats.max_exec_us,
               (unsigned long)stats.max_response_us);
    }

    /* Then dump full CSV for detailed analysis */
    printf("\n");
    trace_dump_usb();
    
    /* And FreeRTOS stats */
    trace_dump_runtime_stats();

    vTaskDelete(NULL);
}
