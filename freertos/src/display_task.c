#include "tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "project_config.h"
#include "board_init.h"
#include "tetris_logic.h"
#include "display.h"

extern tetris_state_t g_tetris_state;

void task_display(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();
    pcd8544_t *lcd = board_get_lcd();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_DISPLAY_MS));

        /* Render using display helpers. Get a snapshot of the display grid
         * (locked + ghost + active) first — function takes internal mutex. */
        uint8_t grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS];
        tetris_get_display_grid(&g_tetris_state, grid);

        /* Copy other state fields under the state's mutex to avoid races */
        int score = 0;
        int level = 0;
        int hold_piece = TETRIS_EMPTY;
        int next_pieces[4] = {TETRIS_EMPTY, TETRIS_EMPTY, TETRIS_EMPTY, TETRIS_EMPTY};

        /* copy values directly (mutex to be added later) */
        score = g_tetris_state.score;
        level = g_tetris_state.level;
        hold_piece = g_tetris_state.hold_piece;
        for (int i = 0; i < 4; i++) {
            int idx = (g_tetris_state.bag_index + i) % TETRIS_PIECE_COUNT;
            next_pieces[i] = g_tetris_state.seven_bag[idx];
        }

        /* draw */
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