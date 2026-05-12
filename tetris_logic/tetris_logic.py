# tetris_logic.py

from __future__ import annotations

import random
from copy import deepcopy


from kick_tables import KICKS_I, KICKS_JLSTZ
from piece_data import PIECE_CELLS
from game_state import * 

# ─────────────────────────── public API ─────────────────────────────────────

def game_init(state: GameState) -> None:
    """Call once at startup.  Randomises bags and spawns the first piece."""
    # Reset to defaults (mutate in-place so callers holding the same reference
    # see the fresh state, mirroring the C memset behaviour).
    state.__init__()                         

    state.level = 1
    _shuffle_bag(state)
    state.hold_piece = PieceType.Empty

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
                # No held piece yet
                state.hold_piece         = state.active_piece.type
                state.active_piece.type  = state.next_piece
                state.next_piece         = _pull_from_bag(state)
            else:
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
        # Direction changed or key released — reset DAS
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

        kicks = (
            KICKS_I[old_rot]
            if state.active_piece.type == PieceType.I
            else KICKS_JLSTZ[old_rot]
        )

        for kr, kc in kicks:
            candidate          = deepcopy(state.active_piece)
            candidate.rotation = new_rot
            candidate.row     += kr * rotate_dir
            candidate.col     += kc * rotate_dir
            if _piece_fits(state, candidate):
                state.active_piece = candidate
                break

    # ── Soft drop ─────────────────────────────────────────────────────────────
    if ps.soft_drop_activated:
        candidate = deepcopy(state.active_piece)
        candidate.row += 1
        if _piece_fits(state, candidate):
            state.active_piece  = candidate
            state.score        += 1          # guideline: 1 pt per soft-drop row
            state.gravity_timer = 0
            _maybe_reset_lock(state)
        ps.soft_drop_activated = False

    # ── Gravity ───────────────────────────────────────────────────────────────
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
        state.score += dropped * 2   # guideline: 2 pts per hard-drop row
        ps.hard_drop_activated = False

        _lock_piece(state)
        lines = _clear_lines(state)
        _add_score(state, lines)
        state.score += state.level   # implicit bonus (mirrors original)

        state.active_piece.type = state.next_piece
        state.next_piece        = _pull_from_bag(state)
        _spawn_piece(state.active_piece.type, state)

        if not _piece_fits(state, state.active_piece):
            return False   # game over

        state.hold_locked = False

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

    # Piece can't fall — it's on the ground
    state.on_ground  = True
    state.lock_timer += 1

    if state.lock_timer < LOCK_DELAY and state.move_resets < MAX_RESETS:
        return True   # still in lock-delay window

    _lock_piece(state)
    lines = _clear_lines(state)
    _add_score(state, lines)

    state.active_piece.type = state.next_piece
    state.next_piece        = _pull_from_bag(state)
    state.hold_locked       = False

    _spawn_piece(state.active_piece.type, state)
    if not _piece_fits(state, state.active_piece):
        return False   # game over

    return True


def get_display_grid(state: GameState) -> None:
    """
    Merges locked cells, ghost piece, and active piece into state.grid,
    ready for the renderer to read.
    """
    # Copy locked cells
    for r in range(BOARD_ROWS):
        for c in range(BOARD_COLS):
            state.grid[r][c] = state.locked_grid[r][c]

    # Compute ghost piece (lowest valid drop position)
    ghost = deepcopy(state.active_piece)
    while True:
        candidate = deepcopy(ghost)
        candidate.row += 1
        if not _piece_fits(state, candidate):
            break
        ghost = candidate

    # Paint ghost (only empty cells)
    for dr, dc in PIECE_CELLS[ghost.type][ghost.rotation]:
        r, c = ghost.row + dr, ghost.col + dc
        if 0 <= r < BOARD_ROWS and 0 <= c < BOARD_COLS:
            if not state.grid[r][c]:
                state.grid[r][c] = PieceType.Ghost   # ghost marker

    # Paint active piece
    for dr, dc in PIECE_CELLS[state.active_piece.type][state.active_piece.rotation]:
        r = state.active_piece.row + dr
        c = state.active_piece.col + dc
        if 0 <= r < BOARD_ROWS and 0 <= c < BOARD_COLS:
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


def _piece_fits(state: GameState, p: ActivePiece) -> bool:
    for dr, dc in PIECE_CELLS[p.type][p.rotation]:
        r, c = p.row + dr, p.col + dc
        if r < 0 or r >= BOARD_ROWS:
            return False
        if c < 0 or c >= BOARD_COLS:
            return False
        if state.locked_grid[r][c]:
            return False
    return True


def _lock_piece(state: GameState) -> None:
    for dr, dc in PIECE_CELLS[state.active_piece.type][state.active_piece.rotation]:
        r = state.active_piece.row + dr
        c = state.active_piece.col + dc
        if 0 <= r < BOARD_ROWS and 0 <= c < BOARD_COLS:
            state.locked_grid[r][c] = state.active_piece.type


def _clear_lines(state: GameState) -> int:
    cleared = 0
    r = BOARD_ROWS - 1
    while r >= 0:
        if all(state.locked_grid[r]):
            # Shift everything above down by one row
            for rr in range(r, 0, -1):
                state.locked_grid[rr] = list(state.locked_grid[rr - 1])
            state.locked_grid[0] = [PieceType.Empty] * BOARD_COLS
            cleared += 1
            # Re-check the same row index (now contains the row that was above)
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
    pieces = list(range(PIECE_COUNT))   # 0 .. 6 (matches PieceType values)
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
    state.active_piece.row      = 0
    state.active_piece.col      = BOARD_COLS // 2 - 1

    state.on_ground    = False
    state.lock_timer   = 0
    state.move_resets  = 0
    state.gravity_timer = 0


def _maybe_reset_lock(state: GameState) -> None:
    if state.on_ground and state.move_resets < MAX_RESETS:
        state.lock_timer  = 0
        state.move_resets += 1