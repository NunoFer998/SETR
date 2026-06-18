#include "FreeRTOS.h"

#include "project_config.h"
#include "game_state.h"

void game_state_init(game_state_t *state) {
    state->accel_mutex = xSemaphoreCreateMutex();
    state->display_mutex = xSemaphoreCreateMutex();
    state->accel_x = 0;
    state->square_x = 42;
    state->square_y = INIT_SQUARE_Y;
}


/* Simple lock/unlock wrappers so callers can protect unrelated structures
 * using the existing mutexes (treat accel_mutex as "poll" mutex). */
void game_state_lock_poll(game_state_t *state) {
    if (state->accel_mutex) xSemaphoreTake(state->accel_mutex, portMAX_DELAY);
}

void game_state_unlock_poll(game_state_t *state) {
    if (state->accel_mutex) xSemaphoreGive(state->accel_mutex);
}

void game_state_lock_display(game_state_t *state) {
    if (state->display_mutex) xSemaphoreTake(state->display_mutex, portMAX_DELAY);
}

void game_state_unlock_display(game_state_t *state) {
    if (state->display_mutex) xSemaphoreGive(state->display_mutex);
}