#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "snake/game.h"
#include "snake/input.h"
#include "snake/persist.h"
#include "snake/platform.h"
#include "snake/render.h"

static void compute_required_terminal_size(const GameConfig* config, int* out_width, int* out_height) {
    if (!out_width || !out_height) { return; }

    int board_w = (config) ? config->board_width : PERSIST_CONFIG_DEFAULT_WIDTH;
    int board_h = (config) ? config->board_height : PERSIST_CONFIG_DEFAULT_HEIGHT;

    /* Keep in sync with layout in src/render/render.c */
    const int field_x = 1;
    const int field_y = 2;
    const int field_width = board_w + 2;
    const int field_height = board_h + 2;

    int min_w = field_x + field_width;  /* rightmost index + 1 */
    int min_h = field_y + field_height; /* bottommost index + 1 */

    /* Side panel starts at x = field_x + field_width + 2 */
    const int side_panel_x = field_x + field_width + 2;
    const int side_panel_text_min = 14; /* enough for "High Scores:" and short values */
    if (side_panel_x + side_panel_text_min > min_w) { min_w = side_panel_x + side_panel_text_min; }

    /* High-score header starts at y = field_y + 5 */
    const int hiscore_header_y = field_y + 5;
    if (hiscore_header_y + 1 > min_h) { min_h = hiscore_header_y + 1; }

    if (min_w < 20) { min_w = 20; }
    if (min_h < 10) { min_h = 10; }

    *out_width = min_w;
    *out_height = min_h;
}

static void validate_and_fix_config(GameConfig* config) {
    if (!config) { return; }

    /* Ensure values are sane even if config came from somewhere else. */
    if (config->board_width < 20) { config->board_width = 20; }
    if (config->board_width > 100) { config->board_width = 100; }
    if (config->board_height < 10) { config->board_height = 10; }
    if (config->board_height > 50) { config->board_height = 50; }

    if (config->tick_rate_ms < 10) { config->tick_rate_ms = 10; }
    if (config->tick_rate_ms > 1000) { config->tick_rate_ms = 1000; }

    if (config->min_screen_width < 20) { config->min_screen_width = 20; }
    if (config->min_screen_height < 10) { config->min_screen_height = 10; }

    /* Don't allow a board/layout that cannot fit inside the configured minimum screen size. */
    int required_w = 0, required_h = 0;
    compute_required_terminal_size(config, &required_w, &required_h);

    if (config->min_screen_width < required_w) { config->min_screen_width = required_w; }
    if (config->min_screen_height < required_h) { config->min_screen_height = required_h; }
}

int main(void) {
    /* Load configuration */
    GameConfig config;
    persist_load_config(".snake_config", &config);

    validate_and_fix_config(&config);

    /* Initialize rendering using config-driven minimum terminal size */
    if (!render_init(config.min_screen_width, config.min_screen_height)) {
        fprintf(stderr, "Failed to initialize rendering (need at least %dx%d)\n", config.min_screen_width, config.min_screen_height);
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
