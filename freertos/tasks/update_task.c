#include "tasks/tasks.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"

#include "config/project_config.h"
#include "state/game_state.h"

extern game_state_t g_game_state;

void task_update_square(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_UPDATE_MS));

        int16_t local_x = game_state_get_accel_x(&g_game_state);

        bool btn_up = gpio_get(LEFT_BUTTON);
        bool btn_down = gpio_get(RIGHT_BUTTON);
        bool btn_diag = gpio_get(LSM_OUT_PIN);

        int x = (int)local_x;
        if (x > DEAD_ZONE) {
            x = x - DEAD_ZONE;
            if (x > MAX_RAW) x = MAX_RAW;
        } else if (x < -DEAD_ZONE) {
            x = x + DEAD_ZONE;
            if (x < -MAX_RAW) x = -MAX_RAW;
        } else {
            x = 0;
        }

        int max_x = PCD8544_WIDTH - SQUARE_SIZE;
        int new_pos_x = (x + MAX_RAW) * max_x / (2 * MAX_RAW);
        int rotate = 0;
        if (new_pos_x < 0) new_pos_x = 0;
        if (new_pos_x > max_x) new_pos_x = max_x;

        int max_y = PCD8544_HEIGHT - SQUARE_SIZE;
        int new_pos_y = game_state_get_square_y(&g_game_state);

        if (btn_up) {
            new_pos_y += BUTTON_STEP;
        }
        if (btn_down) {
            new_pos_x += BUTTON_STEP;
        }
        if (btn_diag) {
            rotate = (rotate + 90) % 360; 
        }

        if (new_pos_y < 0) new_pos_y = 0;
        if (new_pos_y > max_y) new_pos_y = max_y;
        if (new_pos_x < 0) new_pos_x = 0;
        if (new_pos_x > max_x) new_pos_x = max_x;

        game_state_set_square_position(&g_game_state, new_pos_x, new_pos_y, rotate);
    }
}