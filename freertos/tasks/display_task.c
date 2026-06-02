#include "tasks/tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "config/project_config.h"
#include "board/board_init.h"
#include "tetris_logic.h"

extern tetris_state_t g_tetris_state;

void task_display(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();
    pcd8544_t *lcd = board_get_lcd();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_DISPLAY_MS));

        /* Render Tetris grid rotated +90°  (game top → screen right).
           After rotation: rows span the x-axis (reversed), cols span the y-axis.
           Square cells for uniform proportions. */
        uint8_t grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS];
        tetris_get_display_grid(&g_tetris_state, grid);

        const int cell = 3;  /* square: 3×3 px per cell */
        const int total_w = TETRIS_TOTAL_ROWS * cell;  /* 24×3 = 72 */
        const int total_h = TETRIS_BOARD_COLS  * cell;  /* 10×3 = 30 */
        const int x_offset = (PCD8544_WIDTH  - total_w) / 2;  /* 6 */
        const int y_offset = (PCD8544_HEIGHT - total_h) / 2;  /* 9 */

        pcd8544_clear(lcd);
        for (int r = 0; r < TETRIS_TOTAL_ROWS; r++) {
            for (int c = 0; c < TETRIS_BOARD_COLS; c++) {
                if (grid[r][c] != TETRIS_EMPTY) {
                    /* +90° rotation: top→right, bottom→left */
                    int x = x_offset + (TETRIS_TOTAL_ROWS - 1 - r) * cell;
                    int y = y_offset + c * cell;
                    pcd8544_fill_rect(lcd, x, y, cell, cell, 1);
                }
            }
        }
        pcd8544_show(lcd);
    }
}