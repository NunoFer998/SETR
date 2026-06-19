#include "tasks.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"

#include "project_config.h"
#include "tetris_logic.h"
#include "game_state.h"

#include <stdio.h>

extern tetris_state_t g_tetris_state;
extern volatile uint32_t debug_audio_irq_count;
extern volatile uint32_t debug_isr_entry_count;
extern volatile uint32_t execution_budget_us;

void task_update_square(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t tick_counter = 0;
    uint32_t last_gpio_state = gpio_get(15);
    uint32_t gpio_change_count = 0;

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_UPDATE_MS));

        if (!tetris_update(&g_tetris_state)) {
            tetris_init(&g_tetris_state);
        }

        uint32_t current_gpio_state = gpio_get(15);
        if (current_gpio_state != last_gpio_state) {
            gpio_change_count++;
            last_gpio_state = current_gpio_state;
        }

        if (++tick_counter % 30 == 0) {

            uint32_t gpio_state = gpio_get(15);

            uint32_t irq_status = gpio_get_irq_event_mask(15);
            game_state_lock_poll(&g_game_state);
            int soft_drop = g_tetris_state.poll.soft_drop_activated;
            game_state_unlock_poll(&g_game_state);
        }
    }
}