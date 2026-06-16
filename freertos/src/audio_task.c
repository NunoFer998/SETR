#include "tasks.h"

#include <stdbool.h>
#include <stdio.h>

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
static volatile bool audio_task_busy = false;  // Indicates task is processing
static volatile uint32_t last_drop_time_ms = 0;  // For debouncing
volatile uint32_t execution_budget_us = AUDIO_SERVER_BUDGET_US;  // Execution time budget (NOT static - exported)

#define AUDIO_DEBOUNCE_MS  20  // Minimal debounce (just filter bounce, not block continuous input)

/* ── Replenishment queue (circular buffer) ────────────────────────────── */
typedef struct {
    TickType_t   replenish_at;   /* Absolute tick when this amount is due */
    uint32_t     amount_us;      /* How many microseconds to restore      */
} replenish_entry_t;

static replenish_entry_t repl_queue[AUDIO_SERVER_MAX_REPLENISHMENTS];
static UBaseType_t repl_head  = 0;  /* Next entry to fire  */
static UBaseType_t repl_tail  = 0;  /* Next free slot      */
static UBaseType_t repl_count = 0;  /* Entries in the queue */

static void audio_replenish_callback(TimerHandle_t timer_handle) {
    (void)timer_handle;

    /* Restore the execution time budget from the head entry */
    execution_budget_us += repl_queue[repl_head].amount_us;
    
    /* Cap at maximum budget */
    if (execution_budget_us > AUDIO_SERVER_BUDGET_US) {
        execution_budget_us = AUDIO_SERVER_BUDGET_US;
    }
    
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

        /* Check if we have execution budget available */
        if (execution_budget_us == 0) {
            /* No budget - defer until replenishment */
            budget_exhausted_count++;
            if (budget_exhausted_count % 10 == 1) {
                printf("[AUDIO] Budget exhausted! Deferred %lu events\n", 
                       (unsigned long)budget_exhausted_count);
            }
            continue;
        }

        /* Debouncing: Check minimum time between drops */
        uint32_t current_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((current_time_ms - last_drop_time_ms) < AUDIO_DEBOUNCE_MS) {
            /* Too soon after last drop - ignore this event */
            audio_request_pending = false;
            continue;
        }

        /* Mark task as busy - consolidate any interrupts that arrive during processing */
        audio_task_busy = true;
        
        /* Clear request flag BEFORE processing to catch any new genuine events */
        audio_request_pending = false;

        /* SPORADIC SERVER: Measure execution time */
        uint32_t start_time_us = time_us_32();
        
        /* Perform work - trigger soft drop (accelerated falling) */
        /* This allows continuous audio to continuously accelerate the piece */
        g_tetris_state.poll.soft_drop_activated = true;
        
        /* Update last drop time for debouncing */
        last_drop_time_ms = current_time_ms;
        
        /* Calculate execution time consumed */
        uint32_t execution_time_us = time_us_32() - start_time_us;
        
        /* Task no longer busy - can accept new events */
        audio_task_busy = false;
        
        /* Log if execution time is unusually high (debugging) */
        if (execution_time_us > 1000) {  // > 1ms is suspicious for this simple task
            printf("[AUDIO] High execution time: %lu us (budget remaining: %lu us)\n",
                   (unsigned long)execution_time_us,
                   (unsigned long)execution_budget_us);
        }
        
        /* Consume budget (ensure we don't underflow) */
        if (execution_time_us >= execution_budget_us) {
            printf("[AUDIO] Budget exhausted by single execution! (%lu us consumed, %lu us available)\n",
                   (unsigned long)execution_time_us,
                   (unsigned long)execution_budget_us);
            execution_budget_us = 0;
        } else {
            execution_budget_us -= execution_time_us;
        }

        /* SS rule: schedule replenishment of consumed time, T_period from now */
        TickType_t now = xTaskGetTickCount();
        repl_queue[repl_tail].replenish_at = now + pdMS_TO_TICKS(AUDIO_SERVER_PERIOD_MS);
        repl_queue[repl_tail].amount_us = execution_time_us;
        repl_tail = (repl_tail + 1) % AUDIO_SERVER_MAX_REPLENISHMENTS;
        repl_count++;

        /* Arm timer for the earliest pending replenishment */
        if (repl_count == 1) {
            xTimerChangePeriod(audio_replenish_timer,
                               pdMS_TO_TICKS(AUDIO_SERVER_PERIOD_MS), 0);
        }
    }
}

void audio_task_request_drop_from_isr(void) {
    // Only set pending if task is NOT already busy processing
    // This consolidates multiple interrupts into one request
    if (!audio_task_busy) {
        audio_request_pending = true;
    }
    // If task is busy, this interrupt is effectively ignored (consolidated)
}