#include "tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "project_config.h"
#include "board_init.h"
#include "tetris_logic.h"
#include "display.h"
#include "game_state.h"

extern tetris_state_t g_tetris_state;

void task_display(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();
    pcd8544_t *lcd = board_get_lcd();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_DISPLAY_MS));


        uint8_t grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS];
        game_state_lock_display(&g_game_state);
        tetris_get_display_grid(&g_tetris_state, grid);
        int score = g_tetris_state.score;
        int level = g_tetris_state.level;
        int hold_piece = g_tetris_state.hold_piece;
        int next_pieces[4] = {TETRIS_EMPTY, TETRIS_EMPTY, TETRIS_EMPTY, TETRIS_EMPTY};
        for (int i = 0; i < 4; i++) {
            int idx = (g_tetris_state.bag_index + i) % TETRIS_PIECE_COUNT;
            next_pieces[i] = g_tetris_state.seven_bag[idx];
        }
        game_state_unlock_display(&g_game_state);

        pcd8544_clear(lcd);
        display_draw_background(lcd);
        display_draw_board(lcd, grid);
        display_draw_score(lcd, score);
        display_draw_level(lcd, level);
        display_draw_hold(lcd, hold_piece);
        display_draw_next_queue(lcd, next_pieces, 4);
        pcd8544_show(lcd);
    }
}