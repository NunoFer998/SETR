#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "board_init.h"
#include "project_config.h"
#include "tetris_logic.h"
#include "tasks.h"

tetris_state_t g_tetris_state;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
    for (;;) {
        tight_loop_contents();
    }
}

int main(void) {
    hw_init();

    printf("\n");
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   FreeRTOS Tetris — Preemptive Scheduler     ║\n");
    printf("║   Priorities: Input=3  Update=2  Display=1   ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    printf("\n");

    tetris_init(&g_tetris_state);
    printf("[RTOS] Tetris state initialized\n");

    BaseType_t ret;

    ret = xTaskCreate(task_read_input, "Input", STACK_INPUT, NULL, PRIORITY_INPUT, NULL);
    configASSERT(ret == pdPASS);
    printf("[RTOS] Task 'Input'   created — priority %d, period %d ms\n", PRIORITY_INPUT, PERIOD_INPUT_MS);

    ret = xTaskCreate(task_update_square, "Update", STACK_UPDATE, NULL, PRIORITY_UPDATE, NULL);
    configASSERT(ret == pdPASS);
    printf("[RTOS] Task 'Update'  created — priority %d, period %d ms\n", PRIORITY_UPDATE, PERIOD_UPDATE_MS);

    ret = xTaskCreate(task_display, "Display", STACK_DISPLAY, NULL, PRIORITY_DISPLAY, NULL);
    configASSERT(ret == pdPASS);
    printf("[RTOS] Task 'Display' created — priority %d, period %d ms\n", PRIORITY_DISPLAY, PERIOD_DISPLAY_MS);

    ret = xTaskCreate(task_audio, "Audio", STACK_AUDIO, NULL, PRIORITY_AUDIO, NULL);
    configASSERT(ret == pdPASS);
    printf("[RTOS] Task 'Audio'   created — priority %d\n", PRIORITY_AUDIO);

    printf("[RTOS] Starting preemptive scheduler...\n\n");
    vTaskStartScheduler();

    printf("[FATAL] Scheduler exited!\n");
    for (;;) {
        tight_loop_contents();
    }

    return 0;
}
