#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "board_init.h"
#include "project_config.h"
#include "tetris_logic.h"
#include "tasks.h"
#include "game_state.h"

tetris_state_t g_tetris_state;
game_state_t g_game_state;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    for (;;) {
        tight_loop_contents();
    }
}

int main(void) {
    hw_init();

    tetris_init(&g_tetris_state);
    game_state_init(&g_game_state);

    BaseType_t ret;

    ret = xTaskCreate(task_read_input, "Input", STACK_INPUT, NULL, PRIORITY_INPUT, NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(task_update_square, "Update", STACK_UPDATE, NULL, PRIORITY_UPDATE, NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(task_display, "Display", STACK_DISPLAY, NULL, PRIORITY_DISPLAY, NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(task_audio, "Audio", STACK_AUDIO, NULL, PRIORITY_AUDIO, NULL);
    configASSERT(ret == pdPASS);
    vTaskStartScheduler();

    for (;;) {
        tight_loop_contents();
    }

    return 0;
}
