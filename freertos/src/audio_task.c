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

/* ── Replenishment queue (circular buffer) ────────────────────────────── */
typedef struct {
    TickType_t   replenish_at;   /* Absolute tick when this amount is due */
    UBaseType_t  amount;         /* How many budget units to restore      */
} replenish_entry_t;

static replenish_entry_t repl_queue[AUDIO_SERVER_MAX_REPLENISHMENTS];
static UBaseType_t repl_head  = 0;  /* Next entry to fire  */
static UBaseType_t repl_tail  = 0;  /* Next free slot      */
static UBaseType_t repl_count = 0;  /* Entries in the queue */

static void audio_replenish_callback(TimerHandle_t timer_handle) {
    (void)timer_handle;

    /* Restore the amount from the head entry */
    audio_budget += repl_queue[repl_head].amount;
    repl_head = (repl_head + 1) % AUDIO_SERVER_MAX_REPLENISHMENTS;
    repl_count--;

    /* If more entries pending, re-arm timer for the next one */
    if (repl_count > 0) {
        TickType_t now  = xTaskGetTickCount();
        TickType_t next = repl_queue[repl_head].replenish_at;
        TickType_t delay = (next > now) ? (next - now) : 1;
        xTimerChangePeriod(audio_replenish_timer, delay, 0);
    }

    /* Wake the audio task if a request was pending and budget is available */
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

        /* SS rule: schedule replenishment of exactly 1 unit, T_S from now */
        TickType_t now = xTaskGetTickCount();
        repl_queue[repl_tail].replenish_at = now + pdMS_TO_TICKS(AUDIO_SERVER_REPLENISH_MS);
        repl_queue[repl_tail].amount = 1;
        repl_tail = (repl_tail + 1) % AUDIO_SERVER_MAX_REPLENISHMENTS;
        repl_count++;

        /* Arm timer for the earliest pending replenishment */
        if (repl_count == 1) {
            xTimerChangePeriod(audio_replenish_timer,
                               pdMS_TO_TICKS(AUDIO_SERVER_REPLENISH_MS), 0);
        }
    }
}

void audio_task_request_drop_from_isr(void) {
    audio_request_pending = true;
}