# kick_tables.py
#
# SRS wall-kick offsets, exactly as specified in the Tetris guideline.
# Indexed by (from_rotation, to_rotation) -> list of (row, col) offsets.
#
# Rotation states:  0=spawn  1=R(CW)  2=180  3=L(CCW)
#
# Convention: offsets are (row, col) where row increases downward.
# The guideline publishes offsets as (col, row) with row increasing upward,
# so every row component is negated when transcribing.
#
# Guideline (col,row) -> our (row,col):   (x,y) -> (-y, x)

# J / L / S / T / Z
KICKS_JLSTZ: dict = {
    # 0->R
    (0, 1): [( 0,  0), ( 0, -1), (-1, -1), ( 2,  0), ( 2, -1)],
    # R->0
    (1, 0): [( 0,  0), ( 0,  1), ( 1,  1), (-2,  0), (-2,  1)],
    # R->2
    (1, 2): [( 0,  0), ( 0,  1), ( 1,  1), (-2,  0), (-2,  1)],
    # 2->R
    (2, 1): [( 0,  0), ( 0, -1), (-1, -1), ( 2,  0), ( 2, -1)],
    # 2->L
    (2, 3): [( 0,  0), ( 0,  1), (-1,  1), ( 2,  0), ( 2,  1)],    # was wrong
    # L->2
    (3, 2): [( 0,  0), ( 0, -1), ( 1, -1), (-2,  0), (-2, -1)],    # was wrong
    # L->0
    (3, 0): [( 0,  0), ( 0, -1), ( 1, -1), (-2,  0), (-2, -1)],
    # 0->L
    (0, 3): [( 0,  0), ( 0,  1), (-1,  1), ( 2,  0), ( 2,  1)],
}

# I tetromino
KICKS_I: dict = {
    # 0->R
    (0, 1): [( 0,  0), ( 0, -2), ( 0,  1), (-1, -2), ( 2,  1)],
    # R->0
    (1, 0): [( 0,  0), ( 0,  2), ( 0, -1), ( 1,  2), (-2, -1)],
    # R->2
    (1, 2): [( 0,  0), ( 0, -1), ( 0,  2), (-2, -1), ( 1,  2)],
    # 2->R
    (2, 1): [( 0,  0), ( 0,  1), ( 0, -2), ( 2,  1), (-1, -2)],
    # 2->L
    (2, 3): [( 0,  0), ( 0,  2), ( 0, -1), ( 1,  2), (-2, -1)],
    # L->2
    (3, 2): [( 0,  0), ( 0, -2), ( 0,  1), (-1, -2), ( 2,  1)],
    # L->0
    (3, 0): [( 0,  0), ( 0,  1), ( 0, -2), ( 2,  1), (-1, -2)],
    # 0->L
    (0, 3): [( 0,  0), ( 0, -1), ( 0,  2), (-2, -1), ( 1,  2)],
}
