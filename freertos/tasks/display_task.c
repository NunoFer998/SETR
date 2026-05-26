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

        /* Render Tetris grid to the display. Scale each cell to 8x2 pixels. */
        uint8_t grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS];
        tetris_get_display_grid(&g_tetris_state, grid);

        const int cell_w = 8;
        const int cell_h = 2;
        const int total_w = TETRIS_BOARD_COLS * cell_w; /* 80 */
        const int total_h = TETRIS_TOTAL_ROWS * cell_h; /* 48 */
        const int x_offset = (PCD8544_WIDTH - total_w) / 2; /* center horizontally */
        const int y_offset = 0; /* fill vertically */

        pcd8544_clear(lcd);
        for (int r = 0; r < TETRIS_TOTAL_ROWS; r++) {
            for (int c = 0; c < TETRIS_BOARD_COLS; c++) {
                if (grid[r][c] != TETRIS_EMPTY) {
                    int x = x_offset + c * cell_w;
                    int y = y_offset + r * cell_h;
                    pcd8544_fill_rect(lcd, x, y, cell_w, cell_h, 1);
                }
            }
        }
        pcd8544_show(lcd);
    }
}