import pygame
from game_state import GameState

def poll_inputs(events,state : GameState ) -> None:
    for event in events:
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_LEFT:
                state.polling_state.rotate_ccw_activated = True
            if event.key == pygame.K_RIGHT: 
                state.polling_state.rotate_cw_activated = True
            if event.key == pygame.K_DOWN: 
                state.polling_state.hold_activated = True
            if event.key == pygame.K_w:
                state.polling_state.hard_drop_activated = True

    # 2. State polling — for held keys (DAS/ARR movement, soft drop)
    keys = pygame.key.get_pressed()
    state.polling_state.move_left_activated  = bool(keys[pygame.K_a])
    state.polling_state.move_right_activated = bool(keys[pygame.K_d])
    state.polling_state.soft_drop_activated  = bool(keys[pygame.K_s])
