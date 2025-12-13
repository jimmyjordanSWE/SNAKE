#include <stdio.h>
#include <stdlib.h>

#include "snake/game.h"
#include "snake/input.h"
#include "snake/platform.h"
#include "snake/render.h"

int main(void) {
    /* Initialize rendering with minimum 30x12 terminal */
    if (!render_init(60, 24)) {
        fprintf(stderr, "Failed to initialize rendering\n");
        return 1;
    }

    /* Initialize game with 20x10 playfield */
    GameState game = {0};
    game_init(&game, 40, 20, 42);

    /* Initialize input */
    if (!input_init()) {
        fprintf(stderr, "Failed to initialize input\n");
        render_shutdown();
        return 1;
    }

    /* Render loop */
    int tick = 0;
    const int TICKS_MAX = 500;

    while (tick < TICKS_MAX && game.status != GAME_STATUS_GAME_OVER) {
        /* Poll input continuously throughout the frame */
        InputState input_state = {0};
        input_poll(&input_state);

        if (input_state.quit) { goto done; }

        if (input_state.p1_dir_set && game.players[0].active) { game.players[0].queued_dir = input_state.p1_dir; }

        if (input_state.p2_dir_set && game.players[1].active) { game.players[1].queued_dir = input_state.p2_dir; }

        if (input_state.pause_toggle) { game.status = (game.status == GAME_STATUS_PAUSED) ? GAME_STATUS_RUNNING : GAME_STATUS_PAUSED; }

        if (input_state.restart) { game_reset(&game); }

        /* Update game state with latest input applied */
        game_tick(&game);

        /* Render frame */
        render_draw(&game);

        /* Sleep to control frame rate */
        platform_sleep_ms(100);

        tick++;
    }

done:
    input_shutdown();
    render_shutdown();

    printf("Game ran for %d ticks\n", tick);
    return 0;
}
