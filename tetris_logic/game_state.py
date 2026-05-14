

# ─────────────────────────── constants ──────────────────────────────────────

BOARD_ROWS:  int = 20
BUFFER_ROWS: int = 4
TOTAL_ROWS: int = BOARD_ROWS + BUFFER_ROWS
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


BAG_TYPES: tuple[int, ...] = (
    PieceType.I, PieceType.O, PieceType.T,
    PieceType.S, PieceType.Z, PieceType.J, PieceType.L,
)
 



# ─────────────────────────── data classes ───────────────────────────────────

class ActivePiece:
    def __init__(self): 
        self.row:      int = 0
        self.col:      int = 0
        self.rotation: int = 0
        self.type:     int = PieceType.Empty

class PollingState:
    """Set flags here before each call to game_update()."""
    def __init__(self):
        self.move_left_activated:  bool = False 
        self.move_right_activated: bool = False
        self.rotate_ccw_activated: bool = False
        self.rotate_cw_activated:  bool = False
        self.hard_drop_activated:  bool = False
        self.soft_drop_activated:  bool = False #not used (maybe remove later)
        self.hold_activated:       bool = False #not used (maybe remove later)


class GameState:
    def __init__(self):
        self.polling_state: PollingState = PollingState()

        # Display grid (locked + ghost + active piece merged)
        self.grid:        list[list[PieceType]] = [[PieceType.Empty] * BOARD_COLS for _ in range(BOARD_ROWS)]
        
        # Settled cells only
        self.locked_grid: list[list[PieceType]] = [[PieceType.Empty] * BOARD_COLS for _ in range(BOARD_ROWS)]
        

        self.score:         int = 0
        self.active_piece:  ActivePiece = ActivePiece()
        self.next_piece:    int = 0        # PieceType value
        self.hold_piece:    int = PieceType.Empty

        self.hold_locked:   bool = False

        self.seven_bag:     list[int] 
        self.bag_index:     int = 0

        self.level:         int = 1
        self.lines_cleared: int = 0
        self.back_to_back:  bool = False

        self.das_direction: int = 0    # -1 left | 0 none | 1 right
        self.das_timer:     int = 0
        self.arr_timer:     int = 0

        self.on_ground:    bool = False
        self.lock_timer:    int = 0
        self.move_resets:   int = 0
        self.gravity_timer: int = 0