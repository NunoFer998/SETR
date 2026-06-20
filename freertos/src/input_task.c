#include "tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "project_config.h"
#include "lsm9ds0_accel.h"
#include "tetris_logic.h"
#include "hardware/gpio.h"
#include "game_state.h"

extern tetris_state_t g_tetris_state;


int prev_left_button_state = 1;
int prev_right_button_state = 1;
int prev_diag_button_state = 1;

void task_read_input(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_INPUT_MS));

        int16_t raw_x = accel_read_x();

        if (raw_x > DEAD_ZONE) {
            game_state_lock_poll(&g_game_state);
            g_tetris_state.poll.move_right_activated = true;
            game_state_unlock_poll(&g_game_state);
        } else if (raw_x < -DEAD_ZONE) {
            game_state_lock_poll(&g_game_state);
            g_tetris_state.poll.move_left_activated = true;
            game_state_unlock_poll(&g_game_state);
        }

        bool btn_left = gpio_get(LEFT_BUTTON);
        bool btn_right = gpio_get(RIGHT_BUTTON);
        if (btn_right && !prev_right_button_state) {
            game_state_lock_poll(&g_game_state);
            g_tetris_state.poll.rotate_cw_activated = true;
            game_state_unlock_poll(&g_game_state);
        }
        if (btn_left && !prev_left_button_state) {
            game_state_lock_poll(&g_game_state);
            g_tetris_state.poll.rotate_ccw_activated = true;
            game_state_unlock_poll(&g_game_state);
        }

        prev_left_button_state = btn_left;
        prev_right_button_state = btn_right;
    }
}