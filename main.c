#include <ctype.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include "snake/game.h"
#include "snake/input.h"
#include "snake/persist.h"
#include "snake/platform.h"
#include "snake/render.h"
#include "snake/types.h"

/* Fixed board dimensions are now defined in types.h as FIXED_BOARD_WIDTH and FIXED_BOARD_HEIGHT */

/* Global player name */
static char player_name[32] = "Player";

static bool get_stdout_terminal_size(int* out_width, int* out_height) {
    if (!out_width || !out_height) { return false; }

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) { return false; }
    if (ws.ws_col == 0 || ws.ws_row == 0) { return false; }

    *out_width = (int)ws.ws_col;
    *out_height = (int)ws.ws_row;
    return true;
}

static bool terminal_size_sufficient(int term_width, int term_height) {
    /* Minimum terminal size needed to fit the fixed board and UI
       Board is fixed at FIXED_BOARD_WIDTH x FIXED_BOARD_HEIGHT
       Centered with at least 1 char on sides and 1 char on top
    */
    const int field_width = FIXED_BOARD_WIDTH + 2;   /* +2 for borders */
    const int field_height = FIXED_BOARD_HEIGHT + 2; /* +2 for borders */

    /* Need space for: top bar (1) + centered field + some margin */
    int min_h = field_height + 2; /* 2 for top bar and some padding */
    int min_w = field_width + 2;  /* at least borders + 1 char margin on each side */

    return term_width >= min_w && term_height >= min_h;
}

static volatile sig_atomic_t terminal_resized = 0;

static void handle_sigwinch(int sig) {
    (void)sig;
    terminal_resized = 1;
}

int main(void) {
    /* Load configuration for tick rate only; board size is now fixed */
    GameConfig config;
    persist_load_config(".snake_config", &config);

    /* Clamp tick rate */
    if (config.tick_rate_ms < 10) { config.tick_rate_ms = 10; }
    if (config.tick_rate_ms > 1000) { config.tick_rate_ms = 1000; }

    /* Select renderer glyph set (0=utf8 box drawing, 1=legacy ascii). */
    render_set_glyphs((config.render_glyphs == 1) ? RENDER_GLYPHS_ASCII : RENDER_GLYPHS_UTF8);

    /* Set up signal handler for terminal resize */
    signal(SIGWINCH, handle_sigwinch);

    int term_w = 0;
    int term_h = 0;

    /* Startup: wait for terminal to be large enough */
    while (1) {
        if (!get_stdout_terminal_size(&term_w, &term_h)) {
            fprintf(stderr, "Failed to get terminal size, assuming 120x30\n");
            term_w = 120;
            term_h = 30;
        }

        if (terminal_size_sufficient(term_w, term_h)) { break; /* Terminal is big enough */ }

        /* Terminal too small: show message and wait for resize or Ctrl+C */
        fprintf(stderr, "\n");
        fprintf(stderr, "╔════════════════════════════════════════╗\n");
        fprintf(stderr, "║  TERMINAL TOO SMALL FOR SNAKE GAME    ║\n");
        fprintf(stderr, "║                                        ║\n");
        fprintf(stderr, "║  Current size: %d x %d                  ║\n", term_w, term_h);
        fprintf(stderr, "║  Minimum size: %d x %d                 ║\n", FIXED_BOARD_WIDTH + 4, FIXED_BOARD_HEIGHT + 4);
        fprintf(stderr, "║                                        ║\n");
        fprintf(stderr, "║  Please resize your terminal window    ║\n");
        fprintf(stderr, "║  or press Ctrl+C to exit               ║\n");
        fprintf(stderr, "╚════════════════════════════════════════╝\n");
        fprintf(stderr, "\n");

        /* Wait a bit and check again */
        platform_sleep_ms(500);
        terminal_resized = 0;
    }

    fprintf(stderr, "Terminal size OK (%dx%d). Starting game...\n", term_w, term_h);

    /* Initialize rendering with a generous minimum to allow centering */
    int min_render_w = FIXED_BOARD_WIDTH + 10;
    int min_render_h = FIXED_BOARD_HEIGHT + 5;
    if (!render_init(min_render_w, min_render_h)) {
        fprintf(stderr, "Failed to initialize rendering\n");
        goto done;
    }

    /* Initialize game with FIXED board dimensions */
    GameState game = {0};
    game_init(&game, FIXED_BOARD_WIDTH, FIXED_BOARD_HEIGHT, 42);

    /* Initialize input */
    if (!input_init()) {
        fprintf(stderr, "Failed to initialize input\n");
        goto done;
    }

    /* Startup: unified welcome, name input, and help screen */
    render_draw_startup_screen(player_name, (int)sizeof(player_name));

    /* Render loop */
    int tick = 0;
    HighScore highscores[PERSIST_MAX_SCORES];
    int highscore_count = 0;

    /* Load high scores for initial render */
    highscore_count = persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);

    /* Clear startup screen and draw initial game state */
    render_draw(&game, player_name, highscores, highscore_count);

    while (game.status != GAME_STATUS_GAME_OVER) {
        /* Check for terminal resize and handle it */
        if (terminal_resized) {
            terminal_resized = 0;
            int new_w = 0, new_h = 0;
            if (!get_stdout_terminal_size(&new_w, &new_h)) {
                new_w = 120;
                new_h = 30;
            }
            if (!terminal_size_sufficient(new_w, new_h)) {
                /* Terminal became too small: show prompt and pause game */
                render_draw(&game, player_name, highscores, highscore_count);

                fprintf(stderr, "\n");
                fprintf(stderr, "╔════════════════════════════════════════╗\n");
                fprintf(stderr, "║  TERMINAL TOO SMALL - GAME PAUSED      ║\n");
                fprintf(stderr, "║                                        ║\n");
                fprintf(stderr, "║  Current size: %d x %d                 ║\n", new_w, new_h);
                fprintf(stderr, "║  Required: %d x %d                     ║\n", FIXED_BOARD_WIDTH + 4, FIXED_BOARD_HEIGHT + 4);
                fprintf(stderr, "║                                        ║\n");
                fprintf(stderr, "║  Resize your terminal to continue      ║\n");
                fprintf(stderr, "║  or press Ctrl+C to exit               ║\n");
                fprintf(stderr, "╚════════════════════════════════════════╝\n");
                fprintf(stderr, "\n");

                /* Wait for terminal to be resized back to acceptable size */
                while (1) {
                    InputState in = (InputState){0};
                    input_poll(&in);
                    if (in.quit) { goto done; }

                    int check_w = 0, check_h = 0;
                    if (!get_stdout_terminal_size(&check_w, &check_h)) {
                        check_w = 120;
                        check_h = 30;
                    }
                    if (terminal_size_sufficient(check_w, check_h)) {
                        fprintf(stderr, "Terminal resized to %dx%d. Resuming game...\n", check_w, check_h);
                        break;
                    }

                    platform_sleep_ms(250);
                    terminal_resized = 0;
                }
            }
        }

        /* Poll input continuously throughout the frame */
        InputState input_state = (InputState){0};
        input_poll(&input_state);

        if (input_state.quit) { goto done; }

        /* Translate raw input to game direction for player 1 */
        if (game_player_is_active(&game, 0)) {
            if (input_state.move_up) { game.players[0].queued_dir = SNAKE_DIR_UP; }
            if (input_state.move_down) { game.players[0].queued_dir = SNAKE_DIR_DOWN; }
            if (input_state.move_left) { game.players[0].queued_dir = SNAKE_DIR_LEFT; }
            if (input_state.move_right) { game.players[0].queued_dir = SNAKE_DIR_RIGHT; }
        }

        if (input_state.pause_toggle) { game.status = (game.status == GAME_STATUS_PAUSED) ? GAME_STATUS_RUNNING : GAME_STATUS_PAUSED; }

        if (input_state.restart) { game_reset(&game); }

        /* Update game state with latest input applied */
        game_tick(&game);

        /* Load high scores (do this periodically to show updates) */
        highscore_count = persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);

        /* Save scores when a player dies (before score reset) */
        int num_players = game_get_num_players(&game);
        for (int i = 0; i < num_players; i++) {
            if (game_player_died_this_tick(&game, i)) {
                int death_score = game_player_score_at_death(&game, i);
                if (death_score > 0) {
                    persist_append_score(".snake_scores", player_name, death_score);
                    render_note_session_score(player_name, death_score);
                    /* Reload scores after appending */
                    highscore_count = persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);
                }
            }
        }

        /* If the player died, show a small animation and wait for acknowledgement */
        if (game_player_died_this_tick(&game, 0)) {
            render_draw(&game, player_name, highscores, highscore_count);
            render_draw_death_overlay(&game, 0, true);

            /* Pause rendering and wait: any key restarts, Q quits */
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

        render_draw(&game, player_name, highscores, highscore_count);

        /* Sleep to control frame rate */
        platform_sleep_ms((uint64_t)config.tick_rate_ms);

        tick++;
    }

done:
    /* Save high scores if the player scored */
    if (game_player_is_active(&game, 0) && game_player_current_score(&game, 0) > 0) {
        persist_append_score(".snake_scores", player_name, game_player_current_score(&game, 0));
        render_note_session_score(player_name, game_player_current_score(&game, 0));
    }

    /* Save updated config */
    persist_write_config(".snake_config", &config);

    input_shutdown();
    render_shutdown();

    printf("Game ran for %d ticks\n", tick);
    return 0;
}
