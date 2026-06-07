#include "tasks.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"

#include "project_config.h"
#include "tetris_logic.h"

extern tetris_state_t g_tetris_state;

void task_update_square(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_UPDATE_MS));

        /* Run one game tick; restart if game over (tetris_update returns false) */
        if (!tetris_update(&g_tetris_state)) {
            tetris_init(&g_tetris_state);
        }
    }
}