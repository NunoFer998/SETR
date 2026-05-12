from tetris_logic import *
from game_state import * 
from poll_inputs import * 
import time
import pygame



def main():
    state = GameState()
    game_init(state)
    pygame.init()

    HEIGHT = 450
    WIDTH = 400


    FramePerSec = pygame.time.Clock()
 
    displaysurface = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Tetris")

    while True: 
        poll_inputs(state)
        game_update(state)
        render_game(displaysurface ,get_display_grid(state))
        pygame.display.update()
        FramePerSec.tick(TICK_RATE)


if __name__ == "__main__":
    main()
        
