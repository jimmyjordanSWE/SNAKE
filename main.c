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
static bool terminal_size_sufficient(int term_width, int term_height) {
const int field_width= FIXED_BOARD_WIDTH + 2;
const int field_height= FIXED_BOARD_HEIGHT + 2;
int min_h= field_height + 2;
int min_w= field_width + 2;
return term_width >= min_w && term_height >= min_h;
}
static volatile sig_atomic_t terminal_resized= 0;
static void handle_sigwinch(int sig) {
(void)sig;
terminal_resized= 1;
}
int main(void) {
GameConfig config;
persist_load_config(".snake_config", &config);
if(config.tick_rate_ms < 10) config.tick_rate_ms= 10;
if(config.tick_rate_ms > 1000) config.tick_rate_ms= 1000;
render_set_glyphs((config.render_glyphs == 1) ? RENDER_GLYPHS_ASCII : RENDER_GLYPHS_UTF8);
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
fprintf(stderr, "║  Minimum size: %d x %d                 ║\n", FIXED_BOARD_WIDTH + 4, FIXED_BOARD_HEIGHT + 4);
fprintf(stderr, "║                                        ║\n");
fprintf(stderr, "║  Please resize your terminal window    ║\n");
fprintf(stderr, "║  or press Ctrl+C to exit               ║\n");
fprintf(stderr, "╚════════════════════════════════════════╝\n");
fprintf(stderr, "\n");
platform_sleep_ms(500);
terminal_resized= 0;
}
fprintf(stderr, "Terminal size OK (%dx%d). Starting game...\n", term_w, term_h);
int min_render_w= FIXED_BOARD_WIDTH + 10;
int min_render_h= FIXED_BOARD_HEIGHT + 5;
if(!render_init(min_render_w, min_render_h)) {
fprintf(stderr, "Failed to initialize rendering\n");
goto done;
}
GameState game= {0};
game_init(&game, FIXED_BOARD_WIDTH, FIXED_BOARD_HEIGHT, 42);
Render3DConfig config_3d= {.active_player= 0, .fov_degrees= 90.0f, .show_minimap= false, .show_stats= false, .screen_width= 800, .screen_height= 600};
bool has_3d= render_3d_init(&game, &config_3d);
if(!has_3d)
fprintf(stderr,
    "Warning: 3D rendering initialization failed, continuing "
    "with 2D only\n");
else
render_3d_draw(&game, player_name, NULL, 0);
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
fprintf(stderr, "║  Required: %d x %d                     ║\n", FIXED_BOARD_WIDTH + 4, FIXED_BOARD_HEIGHT + 4);
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
}
if(input_state.pause_toggle) game.status= (game.status == GAME_STATUS_PAUSED) ? GAME_STATUS_RUNNING : GAME_STATUS_PAUSED;
if(input_state.restart) game_reset(&game);
game_tick(&game);
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
render_draw(&game, player_name, highscores, highscore_count);
if(has_3d) render_3d_draw(&game, player_name, highscores, highscore_count);
platform_sleep_ms((uint64_t)config.tick_rate_ms);
tick++;
}
done:
if(game_player_is_active(&game, 0) && game_player_current_score(&game, 0) > 0) {
persist_append_score(".snake_scores", player_name, game_player_current_score(&game, 0));
render_note_session_score(player_name, game_player_current_score(&game, 0));
}
persist_write_config(".snake_config", &config);
input_shutdown();
render_shutdown();
if(has_3d) render_3d_shutdown();
printf("Game ran for %d ticks\n", tick);
return 0;
}
