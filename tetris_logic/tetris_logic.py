# tetris_logic.py

from __future__ import annotations

import random
from copy import deepcopy


from kick_tables import KICKS_I, KICKS_JLSTZ
from piece_data import PIECE_CELLS
from game_state import *

def game_init(state: GameState) -> None:
    """Call once at startup (or to restart). Resets all fields and spawns the first piece."""
    # Reset every field explicitly — avoids relying on __init__() which can
    # leave attributes unset if the class definition changes.
    state.polling_state   = PollingState()
    state.grid            = [[PieceType.Empty] * BOARD_COLS for _ in range(TOTAL_ROWS)]
    state.locked_grid     = [[PieceType.Empty] * BOARD_COLS for _ in range(TOTAL_ROWS)]
    state.score           = 0
    state.active_piece    = ActivePiece()
    state.next_piece      = PieceType.Empty
    state.hold_piece      = PieceType.Empty
    state.hold_locked     = False
    state.seven_bag       = list(BAG_TYPES)
    state.bag_index       = 0
    state.level           = 1
    state.lines_cleared   = 0
    state.back_to_back    = False
    state.das_direction   = 0
    state.das_timer       = 0
    state.arr_timer       = 0
    state.on_ground       = False
    state.lock_timer      = 0
    state.move_resets     = 0
    state.gravity_timer   = 0
 
    _shuffle_bag(state)
    _spawn_piece(_pull_from_bag(state), state)
    state.next_piece = _pull_from_bag(state)
 
 
def game_update(state: GameState) -> bool:
    """
    Call every frame / input tick.
    Consumes all activated flags in state.polling_state.
    Returns False when the game is over (top-out).
    """
    ps = state.polling_state
 
    # ── Hold ─────────────────────────────────────────────────────────────────
    if ps.hold_activated:
        ps.hold_activated = False
 
        if not state.hold_locked:
            if state.hold_piece == PieceType.Empty:
                # Nothing held yet — stash current, pull next from bag
                state.hold_piece        = state.active_piece.type
                state.active_piece.type = state.next_piece
                state.next_piece        = _pull_from_bag(state)
            else:
                # Swap current with held
                state.hold_piece, state.active_piece.type = (
                    state.active_piece.type, state.hold_piece
                )
 
            state.hold_locked = True
            _spawn_piece(state.active_piece.type, state)
 
    # ── DAS ───────────────────────────────────────────────────────────────────
    new_dir = 0
    if ps.move_left_activated:  new_dir = -1
    if ps.move_right_activated: new_dir =  1
 
    if new_dir != state.das_direction:
        state.das_direction = new_dir
        state.das_timer     = 0
        state.arr_timer     = 0
 
        if new_dir != 0:
            candidate = deepcopy(state.active_piece)
            candidate.col += new_dir
            if _piece_fits(state, candidate):
                state.active_piece = candidate
                _maybe_reset_lock(state)
    elif new_dir != 0:
        state.das_timer += 1
        if state.das_timer >= DAS_DELAY:
            state.arr_timer += 1
            if state.arr_timer >= ARR_DELAY:
                state.arr_timer = 0
                candidate = deepcopy(state.active_piece)
                candidate.col += new_dir
                if _piece_fits(state, candidate):
                    state.active_piece = candidate
                    _maybe_reset_lock(state)
 
    ps.move_left_activated  = False
    ps.move_right_activated = False
 
    # ── Rotation ──────────────────────────────────────────────────────────────
    rotate_dir = 0
    if ps.rotate_cw_activated:
        rotate_dir = 1
        ps.rotate_cw_activated = False
    if ps.rotate_ccw_activated:
        rotate_dir = -1
        ps.rotate_ccw_activated = False
 
    if rotate_dir != 0:
        old_rot = state.active_piece.rotation
        new_rot = (old_rot + rotate_dir) % 4
 
        # O piece has no kicks
        if state.active_piece.type == PieceType.O:
            candidate          = deepcopy(state.active_piece)
            candidate.rotation = new_rot
            if _piece_fits(state, candidate):
                state.active_piece = candidate
 
        else:
            # Look up the exact transition — kicks are direction-specific,
            # NOT a negation of the opposite direction
            table = KICKS_I if state.active_piece.type == PieceType.I else KICKS_JLSTZ
            kicks = table[(old_rot, new_rot)]
 
            for kr, kc in kicks:
                candidate          = deepcopy(state.active_piece)
                candidate.rotation = new_rot
                candidate.row     += kr
                candidate.col     += kc
                if _piece_fits(state, candidate):
                    state.active_piece = candidate
                    _maybe_reset_lock(state)
                    break
 

    # ── Hard drop ─────────────────────────────────────────────────────────────
    if ps.hard_drop_activated:
        dropped = 0
        while True:
            candidate = deepcopy(state.active_piece)
            candidate.row += 1
            if not _piece_fits(state, candidate):
                break
            state.active_piece = candidate
            dropped += 1
        state.score += dropped * 2
        ps.hard_drop_activated = False
 
        _lock_piece(state)
        lines = _clear_lines(state)
        _add_score(state, lines)
 
        # Capture locked position before overwriting with the new piece
        locked_row      = state.active_piece.row
        locked_type     = state.active_piece.type
        locked_rotation = state.active_piece.rotation
 
        state.active_piece.type = state.next_piece
        state.next_piece        = _pull_from_bag(state)
        _spawn_piece(state.active_piece.type, state)
 
        if _is_topped_out(locked_row, locked_type, locked_rotation):
            return False
 
        state.hold_locked = False

    rows_per_tick = _get_gravity_rows_per_tick(state)

    if rows_per_tick > 0:
        for _ in range(rows_per_tick):
            if not game_gravity_tick(state):
                return False
        state.gravity_timer = 0
    else:
        state.gravity_timer += 1
        if state.gravity_timer >= _get_gravity_ticks(state):
            state.gravity_timer = 0
            if not game_gravity_tick(state):
                return False
            
    if ps.soft_drop_activated:
        candidate = deepcopy(state.active_piece)
        candidate.row += 1
        if _piece_fits(state, candidate):
            state.active_piece  = candidate
            state.score        += 1          # guideline: 1 pt per soft-drop row
            state.gravity_timer = 0
            _maybe_reset_lock(state)
        ps.soft_drop_activated = False
 
    return True
 
 
def game_gravity_tick(state: GameState) -> bool:
    """
    Drop the active piece by one row.
    Locks + spawns when it lands.
    Returns False on top-out (game over).
    """
    candidate = deepcopy(state.active_piece)
    candidate.row += 1
 
    if _piece_fits(state, candidate):
        state.active_piece  = candidate
        state.on_ground     = False
        state.lock_timer    = 0
        return True
 
    state.on_ground   = True
    state.lock_timer += 1
 
    if state.lock_timer < LOCK_DELAY and state.move_resets < MAX_RESETS:
        return True
 
    _lock_piece(state)
    lines = _clear_lines(state)
    _add_score(state, lines)
 
    locked_row      = state.active_piece.row
    locked_type     = state.active_piece.type
    locked_rotation = state.active_piece.rotation
 
    state.active_piece.type = state.next_piece
    state.next_piece        = _pull_from_bag(state)
    state.hold_locked       = False
 
    _spawn_piece(state.active_piece.type, state)
    if _is_topped_out(locked_row, locked_type, locked_rotation):
        return False
 
    return True
 
 
def get_display_grid(state: GameState) -> None:
    """
    Merges locked cells, ghost piece, and active piece into state.grid.
    Each cell holds a PieceType int so the renderer can map it to a colour
    directly — no secondary lookup needed.
 
    Write order (lowest priority first, each layer can overwrite):
      1. locked_grid  — preserves exact piece-type colour
      2. ghost        — PieceType.Ghost, written only over Empty cells
      3. active piece — exact piece-type colour, always on top
    """
    # 1. Snapshot locked cells (full grid including buffer)
    for r in range(TOTAL_ROWS):
        state.grid[r] = list(state.locked_grid[r])
 
    # 2. Ghost — compute lowest valid position, paint over empty cells only
    ghost = deepcopy(state.active_piece)
    while True:
        candidate = deepcopy(ghost)
        candidate.row += 1
        if not _piece_fits(state, candidate):
            break
        ghost = candidate
 
    for dr, dc in PIECE_CELLS[ghost.type][ghost.rotation]:
        r, c = ghost.row + dr, ghost.col + dc
        if 0 <= r < TOTAL_ROWS and 0 <= c < BOARD_COLS:
            if state.grid[r][c] == PieceType.Empty:
                state.grid[r][c] = PieceType.Ghost
 
    # 3. Active piece — always on top
    for dr, dc in PIECE_CELLS[state.active_piece.type][state.active_piece.rotation]:
        r = state.active_piece.row + dr
        c = state.active_piece.col + dc
        if 0 <= r < TOTAL_ROWS and 0 <= c < BOARD_COLS:
            state.grid[r][c] = state.active_piece.type
 
 
# ─────────────────────────── private helpers ────────────────────────────────
 
def _get_gravity_rows_per_tick(state: GameState) -> int:
    s = 1.0
    for i in range(state.level - 1):
        s *= 0.8 - i * 0.007
    ticks_per_row = s * TICK_RATE
    if ticks_per_row < 1.0:
        return int(1.0 / ticks_per_row) + 1
    return 0
 
 
def _get_gravity_ticks(state: GameState) -> int:
    s = 1.0
    for i in range(state.level - 1):
        s *= 0.8 - i * 0.007
    ticks = int(s * TICK_RATE)
    return max(ticks, 1)
 
 
def _is_topped_out(locked_row: int, locked_type: int, locked_rotation: int) -> bool:
    """
    True only when every cell of the just-locked piece is inside the buffer
    zone (r < BUFFER_ROWS), meaning the stack has grown so high that a piece
    could not enter the visible board at all before locking.
    """
    for dr, dc in PIECE_CELLS[locked_type][locked_rotation]:
        if locked_row + dr >= BUFFER_ROWS:
            return False   # at least one cell in visible area — not topped out
    return True
 
 
def _piece_fits(state: GameState, p: ActivePiece) -> bool:
    for dr, dc in PIECE_CELLS[p.type][p.rotation]:
        r, c = p.row + dr, p.col + dc
        if r < 0 or r >= TOTAL_ROWS:     # above buffer or below floor
            return False
        if c < 0 or c >= BOARD_COLS:     # out of sides
            return False
        if state.locked_grid[r][c] != PieceType.Empty:
            return False
    return True
 
 
def _lock_piece(state: GameState) -> None:
    """Write the active piece's type into locked_grid, preserving its colour."""
    for dr, dc in PIECE_CELLS[state.active_piece.type][state.active_piece.rotation]:
        r = state.active_piece.row + dr
        c = state.active_piece.col + dc
        if 0 <= r < TOTAL_ROWS and 0 <= c < BOARD_COLS:
            state.locked_grid[r][c] = state.active_piece.type
 
 
def _clear_lines(state: GameState) -> int:
    cleared = 0
    r = TOTAL_ROWS - 1
    while r >= 0:
        if all(cell != PieceType.Empty for cell in state.locked_grid[r]):
            for rr in range(r, 0, -1):
                state.locked_grid[rr] = list(state.locked_grid[rr - 1])
            state.locked_grid[0] = [PieceType.Empty] * BOARD_COLS
            cleared += 1
        else:
            r -= 1
    return cleared
 
 
_BASE_PTS = (0, 100, 300, 500, 800)
 
def _add_score(state: GameState, lines: int) -> None:
    if lines == 0:
        return
 
    pts = _BASE_PTS[lines] * state.level
 
    if lines == 4:
        if state.back_to_back:
            pts = int(pts * 1.5)
        state.back_to_back = True
    else:
        state.back_to_back = False
 
    state.lines_cleared += lines
    state.level = min(state.lines_cleared // 10 + 1, 15)
    state.score += pts
 
 
def _shuffle_bag(state: GameState) -> None:
    pieces = list(BAG_TYPES)
    random.shuffle(pieces)
    state.seven_bag = pieces
    state.bag_index = 0
 
 
def _pull_from_bag(state: GameState) -> int:
    if state.bag_index >= PIECE_COUNT:
        _shuffle_bag(state)
    piece = state.seven_bag[state.bag_index]
    state.bag_index += 1
    return piece
 
 
def _spawn_piece(piece_type: int, state: GameState) -> None:
    state.active_piece.type     = piece_type
    state.active_piece.rotation = 0
    state.active_piece.row      = BUFFER_ROWS // 2   # centre of buffer — enters from above
    state.active_piece.col      = BOARD_COLS // 2 - 1
 
    state.on_ground     = False
    state.lock_timer    = 0
    state.move_resets   = 0
    state.gravity_timer = 0
 
 
def _maybe_reset_lock(state: GameState) -> None:
    if state.on_ground and state.move_resets < MAX_RESETS:
        state.lock_timer  = 0
        state.move_resets += 1
 
