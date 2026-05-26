#ifndef TETRIS_LOGIC_H
#define TETRIS_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

/* Board geometry */
#define TETRIS_BOARD_ROWS 20
#define TETRIS_BUFFER_ROWS 4
#define TETRIS_TOTAL_ROWS (TETRIS_BOARD_ROWS + TETRIS_BUFFER_ROWS)
#define TETRIS_BOARD_COLS 10
#define TETRIS_PIECE_COUNT 7

/* Timing / gameplay constants (kept comparable to Python) */
#define TETRIS_DAS_DELAY 10
#define TETRIS_ARR_DELAY 2
#define TETRIS_LOCK_DELAY 2
#define TETRIS_MAX_RESETS 10
#define TETRIS_TICK_RATE 30

/* Piece types */
typedef enum {
    TETRIS_I = 0,
    TETRIS_O = 1,
    TETRIS_T = 2,
    TETRIS_S = 3,
    TETRIS_Z = 4,
    TETRIS_J = 5,
    TETRIS_L = 6,
    TETRIS_GHOST = 7,
    TETRIS_EMPTY = 8
} tetris_piece_t;

/* Active piece */
typedef struct {
    int row;
    int col;
    int rotation; /* 0..3 */
    int type; /* tetris_piece_t */
} tetris_active_piece_t;

/* Input polling flags */
typedef struct {
    bool move_left_activated;
    bool move_right_activated;
    bool rotate_ccw_activated;
    bool rotate_cw_activated;
    bool hard_drop_activated;
    bool soft_drop_activated;
    bool hold_activated;
} tetris_poll_t;

/* Main game state */
typedef struct {
    tetris_poll_t poll;
    uint8_t grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS]; /* merged grid for display */
    uint8_t locked[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS];
    int score;
    tetris_active_piece_t active_piece;
    int next_piece;
    int hold_piece;
    bool hold_locked;
    int seven_bag[TETRIS_PIECE_COUNT];
    int bag_index;
    int level;
    int lines_cleared;
    bool back_to_back;
    int das_direction;
    int das_timer;
    int arr_timer;
    bool on_ground;
    int lock_timer;
    int move_resets;
    int gravity_timer;
} tetris_state_t;

/* API */
void tetris_init(tetris_state_t *s);
bool tetris_update(tetris_state_t *s);
bool tetris_gravity_tick(tetris_state_t *s);
void tetris_get_display_grid(tetris_state_t *s, uint8_t out_grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS]);

#endif /* TETRIS_LOGIC_H */
