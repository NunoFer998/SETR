#include "FreeRTOS.h"

#include "config/project_config.h"
#include "state/game_state.h"

void game_state_init(game_state_t *state) {
    state->accel_mutex = xSemaphoreCreateMutex();
    state->display_mutex = xSemaphoreCreateMutex();
    state->accel_x = 0;
    state->square_x = 42;
    state->square_y = INIT_SQUARE_Y;
}

void game_state_set_accel_x(game_state_t *state, int16_t accel_x) {
    if (xSemaphoreTake(state->accel_mutex, portMAX_DELAY) == pdTRUE) {
        state->accel_x = accel_x;
        xSemaphoreGive(state->accel_mutex);
    }
}

int16_t game_state_get_accel_x(game_state_t *state) {
    int16_t accel_x = 0;
    if (xSemaphoreTake(state->accel_mutex, portMAX_DELAY) == pdTRUE) {
        accel_x = state->accel_x;
        xSemaphoreGive(state->accel_mutex);
    }
    return accel_x;
}

int game_state_get_square_y(game_state_t *state) {
    int square_y = INIT_SQUARE_Y;
    if (xSemaphoreTake(state->display_mutex, portMAX_DELAY) == pdTRUE) {
        square_y = state->square_y;
        xSemaphoreGive(state->display_mutex);
    }
    return square_y;
}

void game_state_get_square_position(game_state_t *state, int *square_x, int *square_y) {
    if (xSemaphoreTake(state->display_mutex, portMAX_DELAY) == pdTRUE) {
        *square_x = state->square_x;
        *square_y = state->square_y;
        xSemaphoreGive(state->display_mutex);
    }
}

void game_state_set_square_position(game_state_t *state, int square_x, int square_y) {
    if (xSemaphoreTake(state->display_mutex, portMAX_DELAY) == pdTRUE) {
        state->square_x = square_x;
        state->square_y = square_y;
        xSemaphoreGive(state->display_mutex);
    }
}