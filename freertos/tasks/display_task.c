#include "tasks/tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "config/project_config.h"
#include "board/board_init.h"
#include "state/game_state.h"

extern game_state_t g_game_state;

void task_display(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();
    pcd8544_t *lcd = board_get_lcd();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_DISPLAY_MS));

        int pos_x = 0;
        int pos_y = 0;
        game_state_get_square_position(&g_game_state, &pos_x, &pos_y);

        pcd8544_clear(lcd);
        pcd8544_fill_rect(lcd, pos_x, pos_y, SQUARE_SIZE, SQUARE_SIZE, 1);
        pcd8544_show(lcd);
    }
}