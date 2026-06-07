#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

typedef struct {
    SemaphoreHandle_t accel_mutex;
    SemaphoreHandle_t display_mutex;
    volatile int16_t accel_x;
    volatile int square_x;
    volatile int square_y;
} game_state_t;

void game_state_init(game_state_t *state);
void game_state_set_accel_x(game_state_t *state, int16_t accel_x);
int16_t game_state_get_accel_x(game_state_t *state);
int game_state_get_square_y(game_state_t *state);
void game_state_get_square_position(game_state_t *state, int *square_x, int *square_y);
void game_state_set_square_position(game_state_t *state, int square_x, int square_y, int rotate);

#endif /* GAME_STATE_H */