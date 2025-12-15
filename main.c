#include "snake/game.h"
#include "snake/input.h"
#include "snake/persist.h"
#include "snake/platform.h"
#include "snake/render.h"
#include "snake/render_3d.h"
#include "snake/types.h"
#include <ctype.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
static char player_name[32]= "Player";
static bool get_stdout_terminal_size(int* out_width, int* out_height) {
if(!out_width || !out_height) return false;
struct winsize ws;
if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) return false;
if(ws.ws_col == 0 || ws.ws_row == 0) return false;
*out_width= (int)ws.ws_col;
*out_height= (int)ws.ws_row;
return true;
}
static int board_width = PERSIST_CONFIG_DEFAULT_WIDTH;
static int board_height = PERSIST_CONFIG_DEFAULT_HEIGHT;

static bool terminal_size_sufficient(int term_width, int term_height) {
    const int field_width = board_width + 2;
    const int field_height = board_height + 2;
    int min_h = field_height + 2;
    int min_w = field_width + 2;
    return term_width >= min_w && term_height >= min_h;
}
static volatile sig_atomic_t terminal_resized= 0;
static void handle_sigwinch(int sig) {
(void)sig;
terminal_resized= 1;
}

static SnakeDir strafe_left_dir(SnakeDir d) {
    switch(d) {
    case SNAKE_DIR_UP: return SNAKE_DIR_LEFT;
    case SNAKE_DIR_DOWN: return SNAKE_DIR_RIGHT;
    case SNAKE_DIR_LEFT: return SNAKE_DIR_DOWN;
    case SNAKE_DIR_RIGHT: return SNAKE_DIR_UP;
    default: return d;
    }
}

static SnakeDir strafe_right_dir(SnakeDir d) {
    switch(d) {
    case SNAKE_DIR_UP: return SNAKE_DIR_RIGHT;
    case SNAKE_DIR_DOWN: return SNAKE_DIR_LEFT;
    case SNAKE_DIR_LEFT: return SNAKE_DIR_UP;
    case SNAKE_DIR_RIGHT: return SNAKE_DIR_DOWN;
    default: return d;
    }
}
int main(void) {
GameConfig config;
/* Prefer explicit project config file, fallback to legacy dotfile. */
if(persist_load_config("snake_cfg.txt", &config)) {
    fprintf(stderr, "Loaded configuration from snake_cfg.txt\n");
} else if(persist_load_config(".snake_config", &config)) {
    fprintf(stderr, "Loaded configuration from .snake_config\n");
} else {
    fprintf(stderr, "No config file found; using defaults\n");
}
if(config.tick_rate_ms < 10) config.tick_rate_ms= 10;
if(config.tick_rate_ms > 1000) config.tick_rate_ms= 1000;
render_set_glyphs((config.render_glyphs == 1) ? RENDER_GLYPHS_ASCII : RENDER_GLYPHS_UTF8);
    /* apply key bindings from config */
    input_set_key_bindings(config.key_up, config.key_down, config.key_left, config.key_right, config.key_quit, config.key_restart, config.key_pause);
    /* apply board dimensions and other runtime config to main globals */
    board_width = config.board_width;
    board_height = config.board_height;
    /* seed and player name */
    snprintf(player_name, (int)sizeof(player_name), "%s", config.player_name);
signal(SIGWINCH, handle_sigwinch);
int term_w= 0;
int term_h= 0;
while(1) {
if(!get_stdout_terminal_size(&term_w, &term_h)) {
fprintf(stderr, "Failed to get terminal size, assuming 120x30\n");
term_w= 120;
term_h= 30;
}
    if(terminal_size_sufficient(term_w, term_h)) break;
fprintf(stderr, "\n");
fprintf(stderr, "╔════════════════════════════════════════╗\n");
fprintf(stderr, "║  TERMINAL TOO SMALL FOR SNAKE GAME    ║\n");
fprintf(stderr, "║                                        ║\n");
fprintf(stderr, "║  Current size: %d x %d                  ║\n", term_w, term_h);
fprintf(stderr, "║  Minimum size: %d x %d                 ║\n", board_width + 4, board_height + 4);
fprintf(stderr, "║                                        ║\n");
fprintf(stderr, "║  Please resize your terminal window    ║\n");
fprintf(stderr, "║  or press Ctrl+C to exit               ║\n");
fprintf(stderr, "╚════════════════════════════════════════╝\n");
fprintf(stderr, "\n");
platform_sleep_ms(500);
terminal_resized= 0;
}
fprintf(stderr, "Terminal size OK (%dx%d). Starting game...\n", term_w, term_h);
/* Determine minimum terminal render size. Prefer explicit `screen_width`/`screen_height`
     * from config when external 3D view is disabled (these values are in character cells).
     * Otherwise default to board-based minimums so UI elements fit. */
int min_render_w = board_width + 10;
int min_render_h = board_height + 5;
if(!config.enable_external_3d_view) {
    if(config.screen_width > min_render_w) min_render_w = config.screen_width;
    if(config.screen_height > min_render_h) min_render_h = config.screen_height;
}
if(!render_init(min_render_w, min_render_h)) {
    fprintf(stderr, "Failed to initialize rendering\n");
    goto done;
}
GameState game= {0};
    game_init(&game, board_width, board_height, &config);
    Render3DConfig config_3d= {.active_player= config.active_player, .fov_degrees= config.fov_degrees, .show_sprite_debug= (config.show_sprite_debug != 0), .screen_width= config.screen_width, .screen_height= config.screen_height, .wall_height_scale = config.wall_height_scale};
    /* copy optional texture paths into Render3D config (zero-terminated) */
    if(config.wall_texture[0]) snprintf(config_3d.wall_texture_path, (int)sizeof(config_3d.wall_texture_path), "%s", config.wall_texture);
    if(config.floor_texture[0]) snprintf(config_3d.floor_texture_path, (int)sizeof(config_3d.floor_texture_path), "%s", config.floor_texture);
    bool has_3d = false;
    if(config.enable_external_3d_view) {
        has_3d = render_3d_init(&game, &config_3d);
        if(!has_3d) fprintf(stderr, "Warning: external 3D view initialization failed, continuing with 2D only\n");
        else {
            render_3d_set_tick_rate_ms(config.tick_rate_ms);
            render_3d_draw(&game, player_name, NULL, 0, 0.0f);
        }
    }
if(!input_init()) {
fprintf(stderr, "Failed to initialize input\n");
goto done;
}
render_draw_startup_screen(player_name, (int)sizeof(player_name));
int tick= 0;
HighScore highscores[PERSIST_MAX_SCORES];
int highscore_count= 0;
highscore_count= persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);
render_draw(&game, player_name, highscores, highscore_count);
while(game.status != GAME_STATUS_GAME_OVER) {
if(terminal_resized) {
terminal_resized= 0;
int new_w= 0, new_h= 0;
if(!get_stdout_terminal_size(&new_w, &new_h)) {
new_w= 120;
new_h= 30;
}
if(!terminal_size_sufficient(new_w, new_h)) {
render_draw(&game, player_name, highscores, highscore_count);
fprintf(stderr, "\n");
fprintf(stderr, "╔════════════════════════════════════════╗\n");
fprintf(stderr, "║  TERMINAL TOO SMALL - GAME PAUSED      ║\n");
fprintf(stderr, "║                                        ║\n");
fprintf(stderr, "║  Current size: %d x %d                 ║\n", new_w, new_h);
fprintf(stderr, "║  Required: %d x %d                     ║\n", board_width + 4, board_height + 4);
fprintf(stderr, "║                                        ║\n");
fprintf(stderr, "║  Resize your terminal to continue      ║\n");
fprintf(stderr, "║  or press Ctrl+C to exit               ║\n");
fprintf(stderr, "╚════════════════════════════════════════╝\n");
fprintf(stderr, "\n");
while(1) {
InputState in= (InputState){0};
input_poll(&in);
if(in.quit) goto done;
int check_w= 0, check_h= 0;
if(!get_stdout_terminal_size(&check_w, &check_h)) {
check_w= 120;
check_h= 30;
}
if(terminal_size_sufficient(check_w, check_h)) {
fprintf(stderr, "Terminal resized to %dx%d. Resuming game...\n", check_w, check_h);
break;
}
platform_sleep_ms(250);
terminal_resized= 0;
}
}
}
InputState input_state= (InputState){0};
input_poll(&input_state);
if(input_state.quit) goto done;
if(game_player_is_active(&game, 0)) {
if(input_state.move_up) game.players[0].queued_dir= SNAKE_DIR_UP;
if(input_state.move_down) game.players[0].queued_dir= SNAKE_DIR_DOWN;
if(input_state.move_left) game.players[0].queued_dir= SNAKE_DIR_LEFT;
if(input_state.move_right) game.players[0].queued_dir= SNAKE_DIR_RIGHT;
/* handle turning (relative to player's current view) - 'a'/'d' */
if(input_state.turn_left) game.players[0].queued_dir= strafe_left_dir(game.players[0].current_dir);
if(input_state.turn_right) game.players[0].queued_dir= strafe_right_dir(game.players[0].current_dir);
}
    if(input_state.pause_toggle) game.status= (game.status == GAME_STATUS_PAUSED) ? GAME_STATUS_RUNNING : GAME_STATUS_PAUSED;
    if(input_state.restart) game_reset(&game);
    /* advance simulation by one tick and notify 3D renderer */
    game_tick(&game);
    if(has_3d) render_3d_on_tick(&game);
    highscore_count= persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);
    int num_players= game_get_num_players(&game);
    for(int i= 0; i < num_players; i++) {
        if(game_player_died_this_tick(&game, i)) {
            int death_score= game_player_score_at_death(&game, i);
            if(death_score > 0) {
                persist_append_score(".snake_scores", player_name, death_score);
                render_note_session_score(player_name, death_score);
                highscore_count= persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);
            }
        }
    }
    if(game_player_died_this_tick(&game, 0)) {
        render_draw(&game, player_name, highscores, highscore_count);
        render_draw_death_overlay(&game, 0, true);
        while(1) {
            InputState in= (InputState){0};
            input_poll(&in);
            if(in.quit) goto done;
            if(in.any_key) {
                game_reset(&game);
                break;
            }
            platform_sleep_ms(20);
        }
    }

    /* Render frames until next tick to provide smooth animation */
    uint64_t frame_start = platform_now_ms();
    uint64_t frame_deadline = frame_start + (uint64_t)config.tick_rate_ms;
    uint64_t prev_frame = frame_start;
    while(platform_now_ms() < frame_deadline) {
        uint64_t now = platform_now_ms();
        float delta_s = (float)(now - prev_frame) / 1000.0f;
        prev_frame = now;
        /* poll input each frame for responsiveness (queued moves will be handled on next tick) */
        InputState in= (InputState){0};
        input_poll(&in);
        if(in.quit) goto done;
        if(game_player_is_active(&game, 0)) {
            if(in.move_up) game.players[0].queued_dir = SNAKE_DIR_UP;
            if(in.move_down) game.players[0].queued_dir = SNAKE_DIR_DOWN;
            if(in.move_left) game.players[0].queued_dir = SNAKE_DIR_LEFT;
            if(in.move_right) game.players[0].queued_dir = SNAKE_DIR_RIGHT;
            if(in.turn_left) game.players[0].queued_dir = strafe_left_dir(game.players[0].current_dir);
            if(in.turn_right) game.players[0].queued_dir = strafe_right_dir(game.players[0].current_dir);
        }
        if(in.pause_toggle) game.status= (game.status == GAME_STATUS_PAUSED) ? GAME_STATUS_RUNNING : GAME_STATUS_PAUSED;
        if(in.restart) game_reset(&game);

        render_draw(&game, player_name, highscores, highscore_count);
        if(has_3d) render_3d_draw(&game, player_name, highscores, highscore_count, delta_s);
        /* target ~60 FPS */
        platform_sleep_ms(16);
    }
    tick++;
}
done:
if(game_player_is_active(&game, 0) && game_player_current_score(&game, 0) > 0) {
persist_append_score(".snake_scores", player_name, game_player_current_score(&game, 0));
render_note_session_score(player_name, game_player_current_score(&game, 0));
}
/* Do not write the configuration file from the game process. The game always reads configuration on startup
     * but must not overwrite the user's config to avoid unwanted changes. */
/* persist_write_config(".snake_config", &config); */
/* free game resources allocated by game_init */
game_free(&game);
input_shutdown();
render_shutdown();
if(has_3d) render_3d_shutdown();
printf("Game ran for %d ticks\n", tick);
return 0;
}
