#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "snake/game.h"
#include "snake/input.h"
#include "snake/persist.h"
#include "snake/platform.h"
#include "snake/render.h"

int main(void) {
    /* Load configuration */
    GameConfig config;
    persist_load_config(".snake_config", &config);

    /* Initialize rendering with minimum 30x12 terminal */
    if (!render_init(60, 24)) {
        fprintf(stderr, "Failed to initialize rendering\n");
        return 1;
    }

    /* Initialize game with configured dimensions */
    GameState game = {0};
    game_init(&game, config.board_width, config.board_height, 42);

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

        if (input_state.pause_toggle) { game.status = (game.status == GAME_STATUS_PAUSED) ? GAME_STATUS_RUNNING : GAME_STATUS_PAUSED; }

        if (input_state.restart) { game_reset(&game); }

        /* Update game state with latest input applied */
        game_tick(&game);

        /* Save scores when a player dies (before score reset) */
        for (int i = 0; i < game.num_players; i++) {
            if (game.players[i].died_this_tick && game.players[i].score_at_death > 0) {
                char player_name[32];
                snprintf(player_name, sizeof(player_name), "Player%d", i + 1);
                persist_append_score(".snake_scores", player_name, game.players[i].score_at_death);
                render_note_session_score(player_name, game.players[i].score_at_death);
            }
        }

        /* If the player died, show a small animation and wait for acknowledgement. */
        if (game.num_players > 0 && game.players[0].died_this_tick) {
            render_draw(&game);
            render_draw_death_overlay(&game, 0, true);

            /* Pause rendering and wait for any keypress (still allow quit). */
            while (1) {
                InputState in = (InputState){0};
                input_poll(&in);
                if (in.quit) { goto done; }
                if (in.any_key) { break; }
                platform_sleep_ms(20);
            }
        }

        /* Render frame */
        render_draw(&game);

        /* Sleep to control frame rate */
        platform_sleep_ms((uint64_t)config.tick_rate_ms);

        tick++;
    }

done:
    /* Save high scores if the player scored */
    if (game.num_players > 0 && game.players[0].score > 0) {
        persist_append_score(".snake_scores", "Player1", game.players[0].score);
        render_note_session_score("Player1", game.players[0].score);
    }

    /* Save updated config */
    persist_write_config(".snake_config", &config);

    input_shutdown();
    render_shutdown();

    printf("Game ran for %d ticks\n", tick);
    return 0;
}
