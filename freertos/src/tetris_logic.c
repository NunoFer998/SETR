#include "tetris_logic.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const int8_t PIECE_CELLS[TETRIS_PIECE_COUNT][4][4][2] = {
    /* I */
    {{{0,-1},{0,0},{0,1},{0,2}},{{-1,1},{0,1},{1,1},{2,1}},{{1,-1},{1,0},{1,1},{1,2}},{{-1,0},{0,0},{1,0},{2,0}}},
    /* O */
    {{{0,0},{0,1},{1,0},{1,1}},{{0,0},{0,1},{1,0},{1,1}},{{0,0},{0,1},{1,0},{1,1}},{{0,0},{0,1},{1,0},{1,1}}},
    /* T */
    {{{0,-1},{0,0},{0,1},{-1,0}},{{-1,0},{0,0},{1,0},{0,1}},{{1,0},{0,-1},{0,0},{0,1}},{{-1,0},{0,-1},{0,0},{1,0}}},
    /* S */
    {{{0,-1},{0,0},{-1,0},{-1,1}},{{-1,0},{0,0},{0,1},{1,1}},{{1,-1},{1,0},{0,0},{0,1}},{{-1,-1},{0,-1},{0,0},{1,0}}},
    /* Z */
    {{{-1,-1},{-1,0},{0,0},{0,1}},{{0,1},{1,0},{1,1},{2,0}},{{0,-1},{0,0},{1,0},{1,1}},{{0,0},{1,-1},{1,0},{2,-1}}},
    /* J */
    {{{-1,-1},{0,-1},{0,0},{0,1}},{{-1,0},{-1,1},{0,0},{1,0}},{{0,-1},{0,0},{0,1},{1,1}},{{-1,0},{0,0},{1,-1},{1,0}}},
    /* L */
    {{{0,-1},{0,0},{0,1},{-1,1}},{{-1,0},{0,0},{1,0},{1,1}},{{0,-1},{0,0},{0,1},{1,-1}},{{-1,-1},{-1,0},{0,0},{1,0}}}
};

/* Simple bag order (1..7) and helper to shuffle */
static const int BAG_TYPES[TETRIS_PIECE_COUNT] = {0,1,2,3,4,5,6};

static void shuffle_bag(int bag[TETRIS_PIECE_COUNT]){
    for(int i=0;i<TETRIS_PIECE_COUNT;i++) bag[i]=BAG_TYPES[i];
    for(int i=TETRIS_PIECE_COUNT-1;i>0;i--){
        int j = rand() % (i+1);
        int tmp = bag[i]; bag[i]=bag[j]; bag[j]=tmp;
    }
}

static inline bool piece_fits(tetris_state_t *s, const tetris_active_piece_t *p){
    for(int i=0;i<4;i++){
        int dr = PIECE_CELLS[p->type][p->rotation][i][0];
        int dc = PIECE_CELLS[p->type][p->rotation][i][1];
        int r = p->row + dr;
        int c = p->col + dc;
        if(r < 0 || r >= TETRIS_TOTAL_ROWS) return false;
        if(c < 0 || c >= TETRIS_BOARD_COLS) return false;
        if(s->locked[r][c] != TETRIS_EMPTY) return false;
    }
    return true;
}

static void lock_piece(tetris_state_t *s){
    for(int i=0;i<4;i++){
        int dr = PIECE_CELLS[s->active_piece.type][s->active_piece.rotation][i][0];
        int dc = PIECE_CELLS[s->active_piece.type][s->active_piece.rotation][i][1];
        int r = s->active_piece.row + dr;
        int c = s->active_piece.col + dc;
        if(r>=0 && r<TETRIS_TOTAL_ROWS && c>=0 && c<TETRIS_BOARD_COLS){
            s->locked[r][c] = (uint8_t)s->active_piece.type;
        }
    }
}

static int clear_lines(tetris_state_t *s){
    int cleared = 0;
    for(int r=TETRIS_TOTAL_ROWS-1;r>=0;){
        bool full = true;
        for(int c=0;c<TETRIS_BOARD_COLS;c++){
            if(s->locked[r][c] == TETRIS_EMPTY){ full = false; break; }
        }
        if(full){
            for(int rr=r; rr>0; rr--){
                memcpy(s->locked[rr], s->locked[rr-1], TETRIS_BOARD_COLS);
            }
            for(int c=0;c<TETRIS_BOARD_COLS;c++) s->locked[0][c] = TETRIS_EMPTY;
            cleared++;
        } else {
            r--;
        }
    }
    return cleared;
}

static void add_score(tetris_state_t *s, int lines){
    if(lines==0) return;
    const int base_pts[5] = {0,100,300,500,800};
    int pts = base_pts[lines] * s->level;
    if(lines==4){ if(s->back_to_back) pts = (pts*3)/2; s->back_to_back = true; } else s->back_to_back=false;
    s->lines_cleared += lines;
    s->level = (s->lines_cleared/10)+1; if(s->level>15) s->level=15;
    s->score += pts;
}

static void spawn_piece(int piece_type, tetris_state_t *s){
    s->active_piece.type = piece_type;
    s->active_piece.rotation = 0;
    s->active_piece.row = TETRIS_BUFFER_ROWS/2;
    s->active_piece.col = TETRIS_BOARD_COLS/2 - 1;
    s->on_ground=false; s->lock_timer=0; s->move_resets=0; s->gravity_timer=0;
}

static bool is_topped_out(int locked_row, int locked_type, int locked_rotation){
    for(int i=0;i<4;i++){
        int dr = PIECE_CELLS[locked_type][locked_rotation][i][0];
        if(locked_row + dr >= TETRIS_BUFFER_ROWS) return false;
    }
    return true;
}

static int pull_from_bag(tetris_state_t *s){
    if(s->bag_index >= TETRIS_PIECE_COUNT){ shuffle_bag(s->seven_bag); s->bag_index = 0; }
    return s->seven_bag[s->bag_index++];
}

static void maybe_reset_lock(tetris_state_t *s){
    if(s->on_ground && s->move_resets < TETRIS_MAX_RESETS){ s->lock_timer = 0; s->move_resets++; }
}

static int get_gravity_rows_per_tick(const tetris_state_t *s){
    double scale = 1.0;
    for(int i = 0; i < s->level - 1; i++) {
        scale *= 0.8 - i * 0.007;
    }
    double ticks_per_row = scale * TETRIS_TICK_RATE;
    if(ticks_per_row < 1.0) {
        return (int)(1.0 / ticks_per_row) + 1;
    }
    return 0;
}

static int get_gravity_ticks(const tetris_state_t *s){
    double scale = 1.0;
    for(int i = 0; i < s->level - 1; i++) {
        scale *= 0.8 - i * 0.007;
    }
    int ticks = (int)(scale * TETRIS_TICK_RATE);
    return ticks > 0 ? ticks : 1;
}

void tetris_init(tetris_state_t *s){
    memset(s, 0, sizeof(*s));
    for(int r=0;r<TETRIS_TOTAL_ROWS;r++) for(int c=0;c<TETRIS_BOARD_COLS;c++){ s->grid[r][c]=TETRIS_EMPTY; s->locked[r][c]=TETRIS_EMPTY; }
    s->score = 0; s->next_piece = TETRIS_EMPTY; s->hold_piece = TETRIS_EMPTY; s->hold_locked=false;
    s->level=1; s->lines_cleared=0; s->back_to_back=false; s->das_direction=0; s->das_timer=0; s->arr_timer=0;
    s->on_ground=false; s->lock_timer=0; s->move_resets=0; s->gravity_timer=0;
    /* init rand and bag */
    srand((unsigned)time(NULL));
    shuffle_bag(s->seven_bag);
    s->bag_index = 0;
    int first = pull_from_bag(s);
    spawn_piece(first, s);
    s->next_piece = pull_from_bag(s);
}

bool tetris_gravity_tick(tetris_state_t *s){
    tetris_active_piece_t cand = s->active_piece;
    cand.row++;
    if(piece_fits(s, &cand)){ s->active_piece = cand; s->on_ground=false; s->lock_timer=0; return true; }
    s->on_ground=true; s->lock_timer++;
    if(s->lock_timer < TETRIS_LOCK_DELAY && s->move_resets < TETRIS_MAX_RESETS) return true;
    lock_piece(s);
    int lines = clear_lines(s);
    add_score(s, lines);
    int locked_row = s->active_piece.row; int locked_type=s->active_piece.type; int locked_rotation=s->active_piece.rotation;
    s->active_piece.type = pull_from_bag(s);
    s->next_piece = pull_from_bag(s);
    s->hold_locked = false;
    spawn_piece(s->active_piece.type, s);
    if(is_topped_out(locked_row, locked_type, locked_rotation)) return false;
    return true;
}

bool tetris_update(tetris_state_t *s){
    tetris_poll_t ps = s->poll; 
    if(ps.hold_activated){
        s->poll.hold_activated = false;
        if(!s->hold_locked){
            if(s->hold_piece == TETRIS_EMPTY){ s->hold_piece = s->active_piece.type; s->active_piece.type = s->next_piece; s->next_piece = pull_from_bag(s); }
            else { int tmp = s->hold_piece; s->hold_piece = s->active_piece.type; s->active_piece.type = tmp; }
            s->hold_locked = true; spawn_piece(s->active_piece.type, s);
        }
    }

    /* DAS / movement */
    int new_dir = 0;
    if(ps.move_left_activated) new_dir = -1;
    if(ps.move_right_activated) new_dir = 1;
    if(new_dir != s->das_direction){ s->das_direction=new_dir; s->das_timer=0; s->arr_timer=0; if(new_dir!=0){ tetris_active_piece_t cand = s->active_piece; cand.col += new_dir; if(piece_fits(s,&cand)){ s->active_piece=cand; maybe_reset_lock(s); } } }
    else if(new_dir!=0){ s->das_timer++; if(s->das_timer>=TETRIS_DAS_DELAY){ s->arr_timer++; if(s->arr_timer>=TETRIS_ARR_DELAY){ s->arr_timer=0; tetris_active_piece_t cand=s->active_piece; cand.col+=new_dir; if(piece_fits(s,&cand)){ s->active_piece=cand; maybe_reset_lock(s); } } } }
    s->poll.move_left_activated = false; s->poll.move_right_activated = false;

    /* Rotation */
    int rotate_dir=0;
    if(ps.rotate_cw_activated){ rotate_dir = 1; s->poll.rotate_cw_activated=false; }
    if(ps.rotate_ccw_activated){ rotate_dir = -1; s->poll.rotate_ccw_activated=false; }
    if(rotate_dir!=0){
        int old = s->active_piece.rotation; int nw = (old + rotate_dir) & 3;
        if(s->active_piece.type == TETRIS_O){ tetris_active_piece_t cand = s->active_piece; cand.rotation = nw; if(piece_fits(s,&cand)) s->active_piece=cand; }
        else {
            for(int k=0;k<5;k++){
                tetris_active_piece_t cand = s->active_piece; cand.rotation = nw;
                if(piece_fits(s,&cand)){ s->active_piece=cand; maybe_reset_lock(s); break; }
            }
        }
    }

    /* Hard drop */
    if(ps.hard_drop_activated){
        int dropped=0; while(1){ tetris_active_piece_t cand = s->active_piece; cand.row++; if(!piece_fits(s,&cand)) break; s->active_piece=cand; dropped++; }
        s->score += dropped*2; s->poll.hard_drop_activated = false; lock_piece(s); int lines = clear_lines(s); add_score(s, lines);
        int locked_row = s->active_piece.row; int locked_type = s->active_piece.type; int locked_rotation = s->active_piece.rotation;
        s->active_piece.type = s->next_piece; s->next_piece = pull_from_bag(s); spawn_piece(s->active_piece.type, s);
        if(is_topped_out(locked_row, locked_type, locked_rotation)) return false; s->hold_locked=false;
    }

    /* Gravity */
    int rows_per_tick = get_gravity_rows_per_tick(s);
    if(rows_per_tick>0){ for(int i=0;i<rows_per_tick;i++) if(!tetris_gravity_tick(s)) return false; s->gravity_timer=0; }
    else { s->gravity_timer++; if(s->gravity_timer >= get_gravity_ticks(s)){ s->gravity_timer=0; if(!tetris_gravity_tick(s)) return false; } }

    /* Soft drop - accelerate falling */
    if(ps.soft_drop_activated){ 
        s->poll.soft_drop_activated=false;  // Clear flag immediately
        
        tetris_active_piece_t cand = s->active_piece; 
        cand.row++; 
        if(piece_fits(s,&cand)){ 
            s->active_piece=cand; 
            s->score++; 
            s->gravity_timer=0; 
            s->on_ground = false;  
            s->lock_timer = 0;     
        } else {
            lock_piece(s);
            int lines = clear_lines(s);
            add_score(s, lines);
            
            int locked_row = s->active_piece.row;
            int locked_type = s->active_piece.type;
            int locked_rotation = s->active_piece.rotation;
            
            s->active_piece.type = s->next_piece;
            s->next_piece = pull_from_bag(s);
            s->hold_locked = false;
            spawn_piece(s->active_piece.type, s);
            
            if(is_topped_out(locked_row, locked_type, locked_rotation)) {
                return false;  // Game over
            }
        }
    }

    return true;
}

void tetris_get_display_grid(tetris_state_t *s, uint8_t out_grid[TETRIS_TOTAL_ROWS][TETRIS_BOARD_COLS]){
    /* copy locked (caller must hold display mutex to avoid races) */
    for(int r=0;r<TETRIS_TOTAL_ROWS;r++) memcpy(out_grid[r], s->locked[r], TETRIS_BOARD_COLS);
    /* ghost */
    tetris_active_piece_t ghost = s->active_piece;
    while (1) {
        tetris_active_piece_t cand = ghost;
        cand.row++;
        if (!piece_fits(s, &cand)) break;
        ghost = cand;
    }
    for(int i=0;i<4;i++){
        int r = ghost.row + PIECE_CELLS[ghost.type][ghost.rotation][i][0];
        int c = ghost.col + PIECE_CELLS[ghost.type][ghost.rotation][i][1];
        if (r>=0 && r<TETRIS_TOTAL_ROWS && c>=0 && c<TETRIS_BOARD_COLS) {
            if (out_grid[r][c] == TETRIS_EMPTY) out_grid[r][c] = TETRIS_GHOST;
        }
    }
    /* active */
    for(int i=0;i<4;i++){
        int r = s->active_piece.row + PIECE_CELLS[s->active_piece.type][s->active_piece.rotation][i][0];
        int c = s->active_piece.col + PIECE_CELLS[s->active_piece.type][s->active_piece.rotation][i][1];
        if (r>=0 && r<TETRIS_TOTAL_ROWS && c>=0 && c<TETRIS_BOARD_COLS) out_grid[r][c] = s->active_piece.type;
    }
}

