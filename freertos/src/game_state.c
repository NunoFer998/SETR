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

void game_state_set_square_position(game_state_t *state, int square_x, int square_y, int rotate) {
    if (xSemaphoreTake(state->display_mutex, portMAX_DELAY) == pdTRUE) {
        state->square_x = square_x;
        state->square_y = square_y;
        switch (rotate) {
            case 90:
                // Rotate 90 degrees clockwise: (x, y) -> (y, -x)
                state->square_x = square_y;
                state->square_y = -square_x;
                break;
            case 180:
                // Rotate 180 degrees: (x, y) -> (-x, -y)
                state->square_x = -square_x;
                state->square_y = -square_y;
                break;
            case 270:
                // Rotate 270 degrees clockwise: (x, y) -> (-y, x)
                state->square_x = -square_y;
                state->square_y = square_x;
                break;
            default:
                // No rotation
                break;
        }
        xSemaphoreGive(state->display_mutex);
    }
}