#include "tasks/tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "config/project_config.h"
#include "drivers/lsm/lsm9ds0_accel.h"
#include "tetris_logic.h"
#include "hardware/gpio.h"

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

        /* Map accelerometer X to left/right movement */
        if (raw_x > DEAD_ZONE) {
            g_tetris_state.poll.move_right_activated = true;
        } else if (raw_x < -DEAD_ZONE) {
            g_tetris_state.poll.move_left_activated = true;
        }

        /* Read buttons and map to tetris actions */
        bool btn_left = gpio_get(LEFT_BUTTON);
        bool btn_right = gpio_get(RIGHT_BUTTON);
        bool mic = gpio_get(LSM_OUT_PIN);

        if (mic && !prev_diag_button_state) {
            g_tetris_state.poll.hard_drop_activated = true;
        }
        if (btn_right && !prev_right_button_state) {
            g_tetris_state.poll.rotate_cw_activated = true;
        }
        if (btn_left && !prev_left_button_state) {
            g_tetris_state.poll.rotate_ccw_activated = true;
        }

        prev_left_button_state = btn_left;
        prev_right_button_state = btn_right;
        prev_diag_button_state = mic;
    }
}