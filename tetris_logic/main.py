import pygame
from game_state import GameState, TICK_RATE
from tetris_logic import game_init, game_update, get_display_grid
from poll_inputs import poll_inputs
from renderer import render_game, renderer_init, SCREEN_WIDTH, SCREEN_HEIGHT


def main():
    state = GameState()
    game_init(state)

    pygame.init()
    renderer_init()          # must come after pygame.init()
    clock = pygame.time.Clock()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Tetris")

    running = True
    while running:
        # Handle OS-level quit (window close button)
        events = pygame.event.get()

        for event in events:
            if event.type == pygame.QUIT:
                running = False

        print("before polling")

        poll_inputs(events,state)

        print("after polling")

        print(state.polling_state.hard_drop_activated)

        alive = game_update(state)
        if not alive:
            running = False   # game over — extend this with a game-over screen

        print("display")
        get_display_grid(state)
        render_game(screen, state)

        print("after display")

        pygame.display.update()
        print("after call")
        clock.tick(TICK_RATE)

    print("quiting")

    pygame.quit()


if __name__ == "__main__":
    main()