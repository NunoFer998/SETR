/**
 * @file critical_instant_test.c
 * @brief Critical instant test - all tasks release simultaneously
 * 
 * This tests the WORST-CASE scenario where all periodic tasks become ready
 * at the same time (t=0). This gives us the absolute worst-case response time
 * for each task under maximum interference.
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "project_config.h"
#include "task_trace.h"
#include "tetris_logic.h"
#include "lsm9ds0_accel.h"
#include "board_init.h"
#include "hardware/gpio.h"

extern tetris_state_t g_tetris_state;

/* Synchronization barrier - all tasks wait here before starting */
static SemaphoreHandle_t start_barrier = NULL;
static volatile uint32_t tasks_ready = 0;
#define TASK_COUNT 3  /* Input, Update, Display (Audio is aperiodic) */

/**
 * Synchronized Input Task
 * Waits at barrier, then executes immediately with all other tasks
 */
void task_input_critical(void *params) {
    (void)params;
    
    printf("[CRITICAL] Input task ready, waiting at barrier...\n");
    
    /* Signal ready and wait for all tasks */
    taskENTER_CRITICAL();
    tasks_ready++;
    taskEXIT_CRITICAL();
    
    /* Wait for start signal */
    xSemaphoreTake(start_barrier, portMAX_DELAY);
    
    printf("[CRITICAL] Input task RELEASED at t=0!\n");
    
    /* NOW: Critical instant - all tasks released simultaneously */
    trace_record_release(TRACE_TASK_INPUT);
    uint32_t tidx = trace_record_start(TRACE_TASK_INPUT);
    
    /* Do actual work */
    int16_t raw_x = accel_read_x();
    
    if (raw_x > DEAD_ZONE) {
        g_tetris_state.poll.move_right_activated = true;
    } else if (raw_x < -DEAD_ZONE) {
        g_tetris_state.poll.move_left_activated = true;
    }
    
    /* Read buttons */
    bool btn_left = gpio_get(LEFT_BUTTON);
    bool btn_right = gpio_get(RIGHT_BUTTON);
    
    if (btn_right) {
        g_tetris_state.poll.rotate_cw_activated = true;
    }
    if (btn_left) {
        g_tetris_state.poll.rotate_ccw_activated = true;
    }
    
    trace_record_end(tidx);
    
    uint32_t exec = time_us_32() - tidx;
    printf("[CRITICAL] Input COMPLETED: exec=%lu us\n", (unsigned long)exec);
    
    /* Task done - delete self */
    vTaskDelete(NULL);
}

/**
 * Synchronized Update Task
 */
void task_update_critical(void *params) {
    (void)params;
    
    printf("[CRITICAL] Update task ready, waiting at barrier...\n");
    
    taskENTER_CRITICAL();
    tasks_ready++;
    taskEXIT_CRITICAL();
    
    xSemaphoreTake(start_barrier, portMAX_DELAY);
    
    printf("[CRITICAL] Update task RELEASED at t=0!\n");
    
    trace_record_release(TRACE_TASK_UPDATE);
    uint32_t tidx = trace_record_start(TRACE_TASK_UPDATE);
    
    /* Do actual work */
    if (!tetris_update(&g_tetris_state)) {
        tetris_init(&g_tetris_state);
    }
    
    trace_record_end(tidx);
    
    uint32_t exec = time_us_32() - tidx;
    printf("[CRITICAL] Update COMPLETED: exec=%lu us\n", (unsigned long)exec);
    
    vTaskDelete(NULL);
}

/**
 * Synchronized Display Task
 */
void task_display_critical(void *params) {
    (void)params;
    pcd8544_t *lcd = board_get_lcd();
    
    printf("[CRITICAL] Display task ready, waiting at barrier...\n");
    
    taskENTER_CRITICAL();
    tasks_ready++;
    taskEXIT_CRITICAL();
    
    xSemaphoreTake(start_barrier, portMAX_DELAY);
    
    printf("[CRITICAL] Display task RELEASED at t=0!\n");
    
    trace_record_release(TRACE_TASK_DISPLAY);
    uint32_t tidx = trace_record_start(TRACE_TASK_DISPLAY);
    
    /* Do actual work - full render pipeline */
    uint8_t grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS];
    tetris_get_display_grid(&g_tetris_state, grid);
    
    const int cell = 3;
    const int total_w = TETRIS_TOTAL_ROWS * cell;
    const int total_h = TETRIS_BOARD_COLS * cell;
    const int x_offset = 0;
    const int y_offset = 0;
    
    pcd8544_clear(lcd);
    for (int r = 0; r < TETRIS_TOTAL_ROWS; r++) {
        for (int c = 0; c < TETRIS_BOARD_COLS; c++) {
            if (grid[r][c] != TETRIS_EMPTY) {
                int x = x_offset + (TETRIS_TOTAL_ROWS - 1 - r) * cell;
                int y = y_offset + c * cell;
                if (grid[r][c] == TETRIS_GHOST) {
                    for (int yy = 0; yy < cell; yy++) {
                        for (int xx = 0; xx < cell; xx++) {
                            if (((xx + yy) & 1) == 0) {
                                pcd8544_pixel(lcd, x + xx, y + yy, 1);
                            }
                        }
                    }
                } else {
                    pcd8544_fill_rect(lcd, x, y, cell, cell, 1);
                }
            }
        }
    }
    pcd8544_show(lcd);
    
    trace_record_end(tidx);
    
    uint32_t exec = time_us_32() - tidx;
    printf("[CRITICAL] Display COMPLETED: exec=%lu us\n", (unsigned long)exec);
    
    vTaskDelete(NULL);
}

/**
 * Coordinator Task
 * Waits for all tasks to be ready, then releases them simultaneously
 */
void task_critical_coordinator(void *params) {
    (void)params;
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║         CRITICAL INSTANT TEST                            ║\n");
    printf("║   Testing worst-case response time when all tasks       ║\n");
    printf("║   release simultaneously at t=0                          ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Wait for all tasks to reach barrier */
    printf("[COORDINATOR] Waiting for all %d tasks to be ready...\n", TASK_COUNT);
    
    while (tasks_ready < TASK_COUNT) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    printf("[COORDINATOR] All tasks ready! Preparing to release...\n");
    vTaskDelay(pdMS_TO_TICKS(100));  /* Small delay for stability */
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  RELEASING ALL TASKS NOW - CRITICAL INSTANT at t=0!     ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    uint32_t release_time = time_us_32();
    printf("[COORDINATOR] Release timestamp: %lu us\n\n", (unsigned long)release_time);
    
    /* Release all tasks simultaneously by giving semaphore TASK_COUNT times */
    for (int i = 0; i < TASK_COUNT; i++) {
        xSemaphoreGive(start_barrier);
    }
    
    /* Wait for tasks to complete */
    vTaskDelay(pdMS_TO_TICKS(500));
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║              CRITICAL INSTANT TEST RESULTS               ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Dump trace data */
    trace_dump_usb();
    
    /* Analyze results */
    printf("\n");
    printf("════════════════════════════════════════════════════════════\n");
    printf("           WORST-CASE RESPONSE TIME ANALYSIS\n");
    printf("════════════════════════════════════════════════════════════\n\n");
    
    const char *task_names[] = {"Input", "Update", "Display"};
    const uint32_t deadlines_us[] = {20000, 33000, 33000};
    
    for (int i = 0; i < TASK_COUNT; i++) {
        task_stats_t stats;
        trace_calculate_stats(i, &stats);
        
        if (stats.count > 0) {
            printf("Task: %s\n", task_names[i]);
            printf("  Execution Time:  %lu µs\n", (unsigned long)stats.max_exec_us);
            printf("  Response Time:   %lu µs  ← WCRT under critical instant\n", 
                   (unsigned long)stats.max_response_us);
            printf("  Deadline:        %lu µs\n", (unsigned long)deadlines_us[i]);
            
            if (stats.max_response_us <= deadlines_us[i]) {
                float margin = 100.0f * (1.0f - (float)stats.max_response_us / deadlines_us[i]);
                printf("  Status:          ✓ MET (%.1f%% margin)\n", margin);
            } else {
                printf("  Status:          ✗ MISSED by %lu µs\n", 
                       (unsigned long)(stats.max_response_us - deadlines_us[i]));
            }
            printf("\n");
        }
    }
    
    printf("════════════════════════════════════════════════════════════\n");
    printf("\nCritical instant test complete. System will continue normally.\n\n");
    
    /* Delete self */
    vTaskDelete(NULL);
}

/**
 * Setup and run critical instant test
 * Call this instead of regular task creation for worst-case testing
 */
void run_critical_instant_test(void) {
    /* Create barrier semaphore (counting semaphore for multiple takes) */
    start_barrier = xSemaphoreCreateCounting(TASK_COUNT, 0);
    configASSERT(start_barrier != NULL);
    
    /* Create synchronized tasks (they will wait at barrier) */
    xTaskCreate(task_input_critical, "Input", STACK_INPUT, NULL, PRIORITY_INPUT, NULL);
    xTaskCreate(task_update_critical, "Update", STACK_UPDATE, NULL, PRIORITY_UPDATE, NULL);
    xTaskCreate(task_display_critical, "Display", STACK_DISPLAY, NULL, PRIORITY_DISPLAY, NULL);
    
    /* Create coordinator (will release all tasks simultaneously) */
    xTaskCreate(task_critical_coordinator, "Coordinator", 1024, NULL, 
                configMAX_PRIORITIES - 1, NULL);  /* Highest priority */
    
    printf("[SETUP] Critical instant test tasks created\n");
}
