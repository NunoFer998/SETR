#include "tasks.h"

#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "project_config.h"
#include "board_init.h"
#include "tetris_logic.h"
#include "game_state.h"

extern tetris_state_t g_tetris_state;

TaskHandle_t audio_task_handle = NULL;

static TimerHandle_t audio_replenish_timer = NULL;
static volatile bool audio_request_pending = false;
static volatile bool audio_task_busy = false;
static volatile uint32_t last_drop_time_ms = 0;
volatile uint32_t execution_budget_us = AUDIO_SERVER_BUDGET_US;

#define AUDIO_DEBOUNCE_MS  20

typedef struct {
    TickType_t   replenish_at;
    uint32_t     amount_us;
} replenish_entry_t;

static replenish_entry_t repl_queue[AUDIO_SERVER_MAX_REPLENISHMENTS];
static UBaseType_t repl_head  = 0;
static UBaseType_t repl_tail  = 0;
static UBaseType_t repl_count = 0;

static void audio_replenish_callback(TimerHandle_t timer_handle) {
    (void)timer_handle;

    execution_budget_us += repl_queue[repl_head].amount_us;
    if (execution_budget_us > AUDIO_SERVER_BUDGET_US) {
        execution_budget_us = AUDIO_SERVER_BUDGET_US;
    }
    
    repl_head = (repl_head + 1) % AUDIO_SERVER_MAX_REPLENISHMENTS;
    repl_count--;

    if (repl_count > 0) {
        TickType_t now  = xTaskGetTickCount();
        TickType_t next = repl_queue[repl_head].replenish_at;
        TickType_t delay = (next > now) ? (next - now) : 1;
        xTimerChangePeriod(audio_replenish_timer, delay, 0);
    }

    if (audio_request_pending && execution_budget_us > 0 && audio_task_handle != NULL) {
        xTaskNotifyGive(audio_task_handle);
    }
}

void task_audio(void *params) {
    (void)params;

    audio_task_handle = xTaskGetCurrentTaskHandle();
    audio_replenish_timer = xTimerCreate(
        "AudioSrv",
        pdMS_TO_TICKS(AUDIO_SERVER_PERIOD_MS),
        pdFALSE,
        NULL,
        audio_replenish_callback);
    configASSERT(audio_replenish_timer != NULL);

    board_enable_audio_irq();

    uint32_t budget_exhausted_count = 0;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!audio_request_pending) {
            continue;
        }

        if (execution_budget_us == 0) {
            budget_exhausted_count++;
            continue;
        }

        uint32_t current_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((current_time_ms - last_drop_time_ms) < AUDIO_DEBOUNCE_MS) {
            audio_request_pending = false;
            continue;
        }

        audio_task_busy = true;
        audio_request_pending = false;

        uint32_t start_time_us = time_us_32();
        
        game_state_lock_poll(&g_game_state);
        g_tetris_state.poll.soft_drop_activated = true;
        game_state_unlock_poll(&g_game_state);
        
        last_drop_time_ms = current_time_ms;
        
        uint32_t execution_time_us = time_us_32() - start_time_us;
        audio_task_busy = false;
        
        if (execution_time_us >= execution_budget_us) {
            execution_budget_us = 0;
        } else {
            execution_budget_us -= execution_time_us;
        }

        TickType_t now = xTaskGetTickCount();
        repl_queue[repl_tail].replenish_at = now + pdMS_TO_TICKS(AUDIO_SERVER_PERIOD_MS);
        repl_queue[repl_tail].amount_us = execution_time_us;
        repl_tail = (repl_tail + 1) % AUDIO_SERVER_MAX_REPLENISHMENTS;
        repl_count++;

        if (repl_count == 1) {
            xTimerChangePeriod(audio_replenish_timer,
                               pdMS_TO_TICKS(AUDIO_SERVER_PERIOD_MS), 0);
        }
    }
}

void audio_task_request_drop_from_isr(void) {
    if (!audio_task_busy) {
        audio_request_pending = true;
    }
}