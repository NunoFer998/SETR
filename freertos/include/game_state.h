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

void game_state_lock_poll(game_state_t *state);
void game_state_unlock_poll(game_state_t *state);
void game_state_lock_display(game_state_t *state);
void game_state_unlock_display(game_state_t *state);

extern game_state_t g_game_state;

#endif /* GAME_STATE_H */