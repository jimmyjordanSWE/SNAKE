#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <unistd.h>

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

static void clamp_config(GameConfig* config) {
    if (!config) { return; }

    /* Ensure values are sane even if config came from somewhere else. */
    if (config->board_width < 20) { config->board_width = 20; }
    if (config->board_width > 100) { config->board_width = 100; }
    if (config->board_height < 10) { config->board_height = 10; }
    if (config->board_height > 50) { config->board_height = 50; }

    if (config->tick_rate_ms < 10) { config->tick_rate_ms = 10; }
    if (config->tick_rate_ms > 1000) { config->tick_rate_ms = 1000; }

    if (config->screen_width < 20) { config->screen_width = 20; }
    if (config->screen_height < 10) { config->screen_height = 10; }
}

static bool get_stdout_terminal_size(int* out_width, int* out_height) {
    if (!out_width || !out_height) { return false; }

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) { return false; }
    if (ws.ws_col == 0 || ws.ws_row == 0) { return false; }

    *out_width = (int)ws.ws_col;
    *out_height = (int)ws.ws_row;
    return true;
}

static bool compute_max_board_size_for_terminal(int term_width, int term_height, int* out_board_w, int* out_board_h) {
    if (!out_board_w || !out_board_h) { return false; }

    /* Keep in sync with layout in compute_required_terminal_size() / src/render/render.c.
       Width requirement is dominated by the side panel: required_w = board_w + 19.
       Height requirement is dominated by the field box: required_h = board_h + 4.
     */
    const int side_panel_text_min = 14;
    const int field_x = 1;
    const int field_y = 2;
    const int field_border_w = 2;
    const int field_border_h = 2;
    const int gap_after_field = 2;

    const int board_w_overhead = field_x + field_border_w + gap_after_field + side_panel_text_min; /* == 1 + 2 + 2 + 14 = 19 */
    const int board_h_overhead = field_y + field_border_h;                                         /* == 2 + 2 = 4 */

    int max_w = term_width - board_w_overhead;
    int max_h = term_height - board_h_overhead;

    /* Also need room for the high-score header at y = field_y + 5 (line index 7). */
    const int hiscore_header_y = field_y + 5;
    if (term_height < hiscore_header_y + 1) { max_h = 0; }

    if (max_w < 0) { max_w = 0; }
    if (max_h < 0) { max_h = 0; }

    *out_board_w = max_w;
    *out_board_h = max_h;
    return true;
}

int main(void) {
    /* Load configuration */
    GameConfig config;
    persist_load_config(".snake_config", &config);

    clamp_config(&config);

    int term_w = 0;
    int term_h = 0;
    bool have_term_size = get_stdout_terminal_size(&term_w, &term_h);

    /* Use a runtime config so we can auto-fit without overwriting .snake_config. */
    GameConfig run_config = config;

    if (have_term_size) {
        int max_board_w = 0;
        int max_board_h = 0;
        (void)compute_max_board_size_for_terminal(term_w, term_h, &max_board_w, &max_board_h);

        /* Keep within game limits, but allow shrinking below the usual config minimums if needed. */
        if (max_board_w > 100) { max_board_w = 100; }
        if (max_board_h > 50) { max_board_h = 50; }

        if (max_board_w < 5 || max_board_h < 5) {
            fprintf(stderr, "TERMINAL TOO SMALL for Snake: terminal is %dx%d; need at least 39x14 to render the minimum board\n", term_w, term_h);
            fprintf(stderr, "Fix: enlarge your terminal window\n");
            return 1;
        }

        if (run_config.board_width > max_board_w || run_config.board_height > max_board_h) {
            int requested_w = run_config.board_width;
            int requested_h = run_config.board_height;

            if (run_config.board_width > max_board_w) { run_config.board_width = max_board_w; }
            if (run_config.board_height > max_board_h) { run_config.board_height = max_board_h; }

            fprintf(stderr, "TERMINAL TOO SMALL for requested board %dx%d; rendering %dx%d instead\n", requested_w, requested_h, run_config.board_width,
                    run_config.board_height);
        }
    }

    int required_w = 0;
    int required_h = 0;
    compute_required_terminal_size(&run_config, &required_w, &required_h);

    /* Initialize rendering based on what the board/UI layout actually needs.
       Note: screen_width/screen_height in .snake_config do not resize the terminal;
       the current terminal window must be at least the required size. */
    if (!render_init(required_w, required_h)) {
        int actual_w = 0;
        int actual_h = 0;
        if (get_stdout_terminal_size(&actual_w, &actual_h)) {
            fprintf(stderr, "Failed to initialize rendering: terminal is %dx%d but need at least %dx%d for board %dx%d\n", actual_w, actual_h, required_w,
                    required_h, run_config.board_width, run_config.board_height);
        } else {
            fprintf(stderr, "Failed to initialize rendering (need at least %dx%d for board %dx%d)\n", required_w, required_h, run_config.board_width,
                    run_config.board_height);
        }
        fprintf(stderr, "Fix: enlarge your terminal window (or reduce board_height/board_width in .snake_config)\n");
        return 1;
    }

    /* Initialize game with configured dimensions */
    GameState game = {0};
    game_init(&game, run_config.board_width, run_config.board_height, 42);

    /* Initialize input */
    if (!input_init()) {
        fprintf(stderr, "Failed to initialize input\n");
        render_shutdown();
        return 1;
    }

    /* Startup: upbeat welcome screen before the game begins. */
    render_draw_welcome_screen();
    while (1) {
        InputState in = (InputState){0};
        input_poll(&in);
        if (in.quit) { goto done; }
        if (in.any_key) { break; }
        platform_sleep_ms(20);
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

            /* Pause rendering and wait: any key restarts, Q quits. */
            while (1) {
                InputState in = (InputState){0};
                input_poll(&in);
                if (in.quit) { goto done; }
                if (in.any_key) {
                    game_reset(&game);
                    break;
                }
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
