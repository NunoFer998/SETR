#include "tasks/tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "config/project_config.h"
#include "drivers/lsm9ds0_accel.h"
#include "state/game_state.h"

extern game_state_t g_game_state;

void task_read_input(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_INPUT_MS));

        int16_t raw_x = accel_read_x();
        game_state_set_accel_x(&g_game_state, raw_x);

        printf("[INPUT] Accel X: %d\n", raw_x);
    }
}