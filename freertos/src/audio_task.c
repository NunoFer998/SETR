#include "tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>

#include "project_config.h"
#include "board_init.h"
#include "tetris_logic.h"
#include "hardware/gpio.h"

/* Task handle — referenced by ISR in board_init.c */
TaskHandle_t audio_task_handle = NULL;

/* Audio task: waits for ISR notification and triggers a piece hard drop */
void task_audio(void *params) {
    (void)params;

    /* store our task handle so ISR can notify us */
    audio_task_handle = xTaskGetCurrentTaskHandle();

    for (;;) {
        /* Block until notified from ISR */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Trigger hard drop in the game state in task context */
        extern tetris_state_t g_tetris_state;
        g_tetris_state.poll.hard_drop_activated = true;
    }
}