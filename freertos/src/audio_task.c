#include "tasks.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "project_config.h"
#include "board_init.h"
#include "tetris_logic.h"

extern tetris_state_t g_tetris_state;

TaskHandle_t audio_task_handle = NULL;

static TimerHandle_t audio_replenish_timer = NULL;
static volatile bool audio_request_pending = false;
static volatile UBaseType_t audio_budget = AUDIO_SERVER_BUDGET_EVENTS;

static void audio_replenish_callback(TimerHandle_t timer_handle) {
    (void)timer_handle;

    audio_budget = AUDIO_SERVER_BUDGET_EVENTS;

    if (audio_request_pending && audio_task_handle != NULL) {
        xTaskNotifyGive(audio_task_handle);
    }
}

void task_audio(void *params) {
    (void)params;

    audio_task_handle = xTaskGetCurrentTaskHandle();
    audio_replenish_timer = xTimerCreate(
        "AudioSrv",
        pdMS_TO_TICKS(AUDIO_SERVER_REPLENISH_MS),
        pdFALSE,
        NULL,
        audio_replenish_callback);
    configASSERT(audio_replenish_timer != NULL);

    board_enable_audio_irq();

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!audio_request_pending) {
            continue;
        }

        if (audio_budget == 0) {
            continue;
        }

        audio_request_pending = false;
        audio_budget--;
        g_tetris_state.poll.hard_drop_activated = true;

        if (audio_budget == 0) {
            xTimerStart(audio_replenish_timer, 0);
        }
    }
}

void audio_task_request_drop_from_isr(void) {
    audio_request_pending = true;
}