

# ─────────────────────────── constants ──────────────────────────────────────

BOARD_ROWS:  int = 20
BOARD_COLS:  int = 10
PIECE_COUNT: int = 7

DAS_DELAY:  int = 10   # ticks before auto-repeat kicks in
ARR_DELAY:  int = 2    # ticks between auto-repeat shifts
LOCK_DELAY: int = 30   # ticks before locking
MAX_RESETS: int = 10   # move resets allowed before forced lock
TICK_RATE:  int = 30   # target ticks per second

# ─────────────────────────── enums / types ──────────────────────────────────

class PieceType:
    I = 0
    O = 1
    T = 2
    S = 3
    Z = 4
    J = 5
    L = 6
    Ghost = 7 
    Empty = 8



# ─────────────────────────── data classes ───────────────────────────────────

class ActivePiece:
    row:      int = 0
    col:      int = 0
    rotation: int = 0
    type:     int = 0   # PieceType value (int) or _HOLD_EMPTY


class PollingState:
    """Set flags here before each call to game_update()."""
    move_left_activated:  bool = False 
    move_right_activated: bool = False
    rotate_ccw_activated: bool = False
    rotate_cw_activated:  bool = False
    hard_drop_activated:  bool = False
    soft_drop_activated:  bool = False #not used (maybe remove later)
    hold_activated:       bool = False #not used (maybe remove later)


class GameState:
    polling_state: PollingState

    # Display grid (locked + ghost + active piece merged)
    grid:        list[list[PieceType]] = [[PieceType.Empty] * BOARD_COLS for _ in range(BOARD_ROWS)]
    
    # Settled cells only
    locked_grid: list[list[PieceType]] = [[PieceType.Empty] * BOARD_COLS for _ in range(BOARD_ROWS)]
    

    score:         int = 0
    active_piece:  ActivePiece
    next_piece:    int = 0        # PieceType value
    hold_piece:    int = PieceType.Empty

    hold_locked:   bool = False

    seven_bag:     list[int] 
    bag_index:     int = 0

    level:         int = 1
    lines_cleared: int = 0
    back_to_back:  bool = False

    das_direction: int = 0    # -1 left | 0 none | 1 right
    das_timer:     int = 0
    arr_timer:     int = 0

    on_ground:    bool = False
    lock_timer:    int = 0
    move_resets:   int = 0
    gravity_timer: int = 0