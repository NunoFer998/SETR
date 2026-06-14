#ifndef TETRIS_DISPLAY_H
#define TETRIS_DISPLAY_H

#include <stdint.h>
#include "drivers/pcd8544/pcd8544.h"
#include "tetris_logic.h"

typedef struct {
    int x;
    int y;
} Position;

extern const int GAME_WIDTH;
extern const int GAME_HEIGHT;
extern const int BOARD_WIDTH;
extern const Position BOARD_START;
extern const Position BOARD_END;
extern const Position SCORE_START;
extern const Position SCORE_END;
extern const Position LEVEL_START;
extern const Position LEVEL_END;
extern const Position HOLD_START;
extern const Position NEXT_START;
extern const Position NEXT_INCREMENT;

extern const unsigned char GAME_IMAGE[];
extern const unsigned char NUMBERS_IMAGE[];
extern const unsigned char square_image[];

#define DISPLAY_NUMBER_WIDTH 3
#define DISPLAY_NUMBER_HEIGHT 3
#define DISPLAY_NUMBER_SPACING 1

void display_draw_background(pcd8544_t *lcd);
void display_draw_board(pcd8544_t *lcd, uint8_t grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS]);
void display_draw_number(pcd8544_t *lcd, int x, int y, int value);
void display_draw_hold(pcd8544_t *lcd, int hold_piece);
void display_draw_next_queue(pcd8544_t *lcd, int next_pieces[], int count);

#endif /* TETRIS_DISPLAY_H */
