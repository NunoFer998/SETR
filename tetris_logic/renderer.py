# renderer.py

import pygame
from tetris_logic import (
    GameState, PieceType,
    BOARD_ROWS, BOARD_COLS, BUFFER_ROWS,
    PIECE_CELLS,
)

# ─────────────────────────── layout ─────────────────────────────────────────

CELL          = 22
BOARD_X       = 140
BOARD_Y       = 20 + BUFFER_ROWS * CELL   # visible board starts below the buffer zone
BUFFER_Y      = 20                         # buffer zone starts here
SIDE_PANEL_W  = 120

SCREEN_WIDTH  = BOARD_X + BOARD_COLS * CELL + SIDE_PANEL_W + 10
SCREEN_HEIGHT = BUFFER_Y + (BUFFER_ROWS + BOARD_ROWS) * CELL + 20

# ─────────────────────────── colours ────────────────────────────────────────

BLACK        = (  0,   0,   0)
WHITE        = (255, 255, 255)
DIMGREY      = ( 25,  25,  25)
GREY         = ( 40,  40,  40)   # visible board background
BUFFER_BG    = ( 20,  20,  35)   # buffer zone background — dark blue tint
BUFFER_LINE  = ( 50,  50,  80)   # buffer zone grid lines
BUFFER_SEP   = ( 80,  80, 140)   # separator line between buffer and board

PIECE_COLOURS = {
    PieceType.I:     (  0, 240, 240),
    PieceType.O:     (240, 240,   0),
    PieceType.T:     (160,   0, 240),
    PieceType.S:     (  0, 240,   0),
    PieceType.Z:     (240,   0,   0),
    PieceType.J:     (  0,   0, 240),
    PieceType.L:     (240, 160,   0),
    PieceType.Ghost: ( 70,  70,  70),
    PieceType.Empty: ( 40,  40,  40),
}

# Fonts — initialised once on first call to renderer_init(), which must be
# called after pygame.init() so the font module is fully loaded.
_font_big   = None
_font_small = None


def renderer_init() -> None:
    """
    Call once after pygame.init() before the first render_game().
    Uses pygame.freetype which is unaffected by the pygame.font circular-import
    bug present in pygame 2.6.1 / Python 3.14.
    """
    global _font_big, _font_small
    import pygame.freetype
    pygame.freetype.init()
    # Use the built-in default font — no font file needed
    _font_big   = pygame.freetype.SysFont("monospace", 16, bold=True)
    _font_small = pygame.freetype.SysFont("monospace", 13)

# ─────────────────────────── drawing helpers ─────────────────────────────────

def _cell_rect(row: int, col: int) -> pygame.Rect:
    return pygame.Rect(BOARD_X + col * CELL, BOARD_Y + row * CELL, CELL, CELL)


def _draw_cell(surface: pygame.Surface, rect: pygame.Rect, colour: tuple) -> None:
    pygame.draw.rect(surface, colour, rect)
    pygame.draw.rect(surface, DIMGREY, rect, 1)


def _draw_piece_preview(
    surface: pygame.Surface,
    piece_type: int,
    centre_x: int,
    centre_y: int,
    cell_size: int = 16,
) -> None:
    """Draw a small centred preview of piece_type at rotation 0."""
    if piece_type == PieceType.Empty:
        return

    colour = PIECE_COLOURS[piece_type]
    cells  = PIECE_CELLS[piece_type][0]

    min_r = min(dr for dr, dc in cells)
    max_r = max(dr for dr, dc in cells)
    min_c = min(dc for dr, dc in cells)
    max_c = max(dc for dr, dc in cells)
    origin_x = centre_x - ((max_c - min_c + 1) * cell_size) // 2 - min_c * cell_size
    origin_y = centre_y - ((max_r - min_r + 1) * cell_size) // 2 - min_r * cell_size

    for dr, dc in cells:
        rect = pygame.Rect(
            origin_x + dc * cell_size,
            origin_y + dr * cell_size,
            cell_size, cell_size,
        )
        _draw_cell(surface, rect, colour)

# ─────────────────────────── board ───────────────────────────────────────────

def _draw_board(surface: pygame.Surface, state: GameState) -> None:
    """
    Draws two zones stacked vertically:
      • Buffer zone  (top, BUFFER_ROWS tall, dark blue tint) — pieces spawn here
      • Visible board (bottom, BOARD_ROWS tall, normal grey)  — the play area

    state.grid row 0 maps to the top of the buffer zone.
    state.grid row BUFFER_ROWS maps to the top of the visible board.
    """
    bw = BOARD_COLS * CELL

    # ── Buffer zone background ────────────────────────────────────────────────
    buf_rect = pygame.Rect(BOARD_X, BUFFER_Y, bw, BUFFER_ROWS * CELL)
    pygame.draw.rect(surface, BUFFER_BG, buf_rect)

    # Draw buffer grid lines
    for r in range(BUFFER_ROWS + 1):
        y = BUFFER_Y + r * CELL
        pygame.draw.line(surface, BUFFER_LINE, (BOARD_X, y), (BOARD_X + bw, y))
    for c in range(BOARD_COLS + 1):
        x = BOARD_X + c * CELL
        pygame.draw.line(surface, BUFFER_LINE, (BOARD_X + c * CELL, BUFFER_Y),
                         (BOARD_X + c * CELL, BUFFER_Y + BUFFER_ROWS * CELL))

    # Draw buffer cell contents
    for buf_r in range(BUFFER_ROWS):
        for c in range(BOARD_COLS):
            cell_type = state.grid[buf_r][c]
            if cell_type == PieceType.Empty:
                continue
            colour = PIECE_COLOURS[cell_type]
            rect = pygame.Rect(BOARD_X + c * CELL, BUFFER_Y + buf_r * CELL, CELL, CELL)
            pygame.draw.rect(surface, colour, rect)
            pygame.draw.rect(surface, BUFFER_LINE, rect, 1)

    # ── Separator line between buffer and visible board ───────────────────────
    sep_y = BUFFER_Y + BUFFER_ROWS * CELL
    pygame.draw.line(surface, BUFFER_SEP, (BOARD_X, sep_y), (BOARD_X + bw, sep_y), 2)

    # ── Visible board background ──────────────────────────────────────────────
    board_rect = pygame.Rect(BOARD_X, BOARD_Y, bw, BOARD_ROWS * CELL)
    pygame.draw.rect(surface, GREY, board_rect)

    # Draw visible cell contents
    for vis_r in range(BOARD_ROWS):
        grid_r = vis_r + BUFFER_ROWS
        for c in range(BOARD_COLS):
            cell_type = state.grid[grid_r][c]
            if cell_type == PieceType.Empty:
                continue
            colour = PIECE_COLOURS[cell_type]
            rect = pygame.Rect(BOARD_X + c * CELL, BOARD_Y + vis_r * CELL, CELL, CELL)
            _draw_cell(surface, rect, colour)

    # Borders
    pygame.draw.rect(surface, WHITE, board_rect, 2)
    pygame.draw.rect(surface, BUFFER_SEP, buf_rect, 1)

# ─────────────────────────── side panels ─────────────────────────────────────

def _draw_panels(surface: pygame.Surface, state: GameState) -> None:
    font_big   = _font_big
    font_small = _font_small

    right_x = BOARD_X + BOARD_COLS * CELL + 10
    left_x  = 10

    # ── RIGHT: Next ───────────────────────────────────────────────────────────
    font_big.render_to(surface, (right_x + 20, BOARD_Y), "NEXT", WHITE)
    panel = pygame.Rect(right_x, BOARD_Y + 22, SIDE_PANEL_W - 10, 80)
    pygame.draw.rect(surface, GREY,  panel)
    pygame.draw.rect(surface, WHITE, panel, 1)
    _draw_piece_preview(surface, state.next_piece, panel.centerx, panel.centery)

    # ── RIGHT: Stats ──────────────────────────────────────────────────────────
    y = BOARD_Y + 120
    for heading, value in [
        ("SCORE", str(state.score)),
        ("LEVEL", str(state.level)),
        ("LINES", str(state.lines_cleared)),
    ]:
        font_big.render_to(surface,   (right_x, y),      heading, WHITE)
        font_small.render_to(surface, (right_x, y + 18), value,   (200, 200, 200))
        y += 50

    # ── LEFT: Hold ────────────────────────────────────────────────────────────
    font_big.render_to(surface, (left_x + 15, BOARD_Y), "HOLD", WHITE)
    panel = pygame.Rect(left_x, BOARD_Y + 22, SIDE_PANEL_W - 10, 80)
    pygame.draw.rect(surface, GREY,  panel)
    pygame.draw.rect(surface, WHITE, panel, 1)

    if state.hold_piece != PieceType.Empty:
        _draw_piece_preview(surface, state.hold_piece, panel.centerx, panel.centery)
        if state.hold_locked:
            font_small.render_to(surface, (left_x + 8, BOARD_Y + 106), "locked", (150, 150, 150))

    # ── LEFT: Controls ────────────────────────────────────────────────────────
    y = BOARD_Y + 200
    for line in ["A Move Left","D Move Right", "← Rotate CCW","→ Rotate CW", 
                 "S Soft drop", "W Hard drop", "↑ Hold"]:
        font_small.render_to(surface, (left_x, y), line, (160, 160, 160))
        y += 18

# ─────────────────────────── public entry point ──────────────────────────────

def render_game(display_surface: pygame.Surface, state: GameState) -> None:
    """
    Clear the screen and draw everything.
    Call pygame.display.update() in the game loop after this.
    get_display_grid(state) must be called before this each frame.
    """
    display_surface.fill(BLACK)
    _draw_board(display_surface, state)
    _draw_panels(display_surface, state)