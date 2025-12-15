#include "snake/snakegame.h"
#include "snake/console.h"
#include "snake/game.h"
#include "snake/game_internal.h"
#include "snake/input.h"
#include "snake/persist.h"
#include "snake/platform.h"
#include "snake/render.h"
#include "snake/render_3d.h"
#include "snake/tty.h"
#include "snake/types.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// new includes for allocation and booleans
#include <stdlib.h>
#include <stdbool.h>
/* New pointer-style SnakeGame object used to separate init/run/cleanup. */
struct SnakeGame {
GameConfig cfg;
Game* game;
bool has_3d;
};
/* Allocate and initialize a SnakeGame instance. On error, returns NULL
 * and sets *err_out to a non-zero code (2=render init fail, 3=game create fail,
 * 4=input init fail, 1=invalid args/other). */
SnakeGame* snake_game_new(const GameConfig* config_in, int* err_out) {
int err= 0;
if(err_out) *err_out= 1; /* default to generic error */
if(!config_in) return NULL;
SnakeGame* s= (SnakeGame*)malloc(sizeof(*s));
if(!s) return NULL;
GameConfig cfg= *config_in;
if(cfg.tick_rate_ms < 10) cfg.tick_rate_ms= 10;
if(cfg.tick_rate_ms > 1000) cfg.tick_rate_ms= 1000;
render_set_glyphs((cfg.render_glyphs == 1) ? RENDER_GLYPHS_ASCII : RENDER_GLYPHS_UTF8);
input_set_key_bindings(cfg.key_up, cfg.key_down, cfg.key_left, cfg.key_right, cfg.key_quit, cfg.key_restart, cfg.key_pause);
const int board_width= cfg.board_width;
const int board_height= cfg.board_height;
platform_winch_init();
int term_w= 0;
int term_h= 0;
while(1) {
if(!platform_get_terminal_size(&term_w, &term_h)) {
console_error("Failed to get terminal size, assuming 120x30\n");
term_w= 120;
term_h= 30;
}
if(tty_size_sufficient_for_board(term_w, term_h, board_width, board_height)) break;
console_box_too_small_for_game(term_w, term_h, board_width + 4, board_height + 4);
platform_sleep_ms(500);
(void)platform_was_resized();
}
console_info("Terminal size OK (%dx%d). Starting game...\n", term_w, term_h);
int min_render_w= board_width + 10;
int min_render_h= board_height + 5;
if(!cfg.enable_external_3d_view) {
if(cfg.screen_width > min_render_w) min_render_w= cfg.screen_width;
if(cfg.screen_height > min_render_h) min_render_h= cfg.screen_height;
}
if(!render_init(min_render_w, min_render_h)) {
console_error("Failed to initialize rendering\n");
err= 2;
goto out_err;
}
Game* game= game_create(&cfg, cfg.seed);
if(!game) {
console_error("Failed to create game\n");
err= 3;
goto cleanup_render;
}
Render3DConfig config_3d= {.active_player= cfg.active_player, .fov_degrees= cfg.fov_degrees, .show_sprite_debug= (cfg.show_sprite_debug != 0), .screen_width= cfg.screen_width, .screen_height= cfg.screen_height, .wall_height_scale= cfg.wall_height_scale, .tail_height_scale= cfg.tail_height_scale};
if(cfg.wall_texture[0]) snprintf(config_3d.wall_texture_path, (int)sizeof(config_3d.wall_texture_path), "%s", cfg.wall_texture);
if(cfg.floor_texture[0]) snprintf(config_3d.floor_texture_path, (int)sizeof(config_3d.floor_texture_path), "%s", cfg.floor_texture);
bool has_3d= false;
if(cfg.enable_external_3d_view) {
has_3d= render_3d_init(game_get_state(game), &config_3d);
if(!has_3d)
console_warn("Warning: external 3D view initialization failed, continuing with 2D only\n");
else {
render_3d_set_tick_rate_ms(cfg.tick_rate_ms);
render_3d_draw(game_get_state(game), cfg.player_name, NULL, 0, 0.0f);
}
}
if(!input_init()) {
console_error("Failed to initialize input\n");
err= 4;
goto cleanup_game;
}
render_draw_startup_screen(cfg.player_name, (int)sizeof(cfg.player_name));
render_draw(game_get_state(game), cfg.player_name, NULL, 0);
/* populate struct and return */
s->cfg= cfg;
s->game= game;
s->has_3d= has_3d;
if(err_out) *err_out= 0;
return s;
cleanup_game:
if(game) game_destroy(game);
cleanup_render:
render_shutdown();
out_err:
if(err_out) *err_out= err ? err : 1;
free(s);
return NULL;
}
/* Run loop that operates on an initialized SnakeGame. The function
 * performs the main loop and does cleanup of game/input/render state
 * when finished; it does not free the SnakeGame pointer itself. */
int snake_game_run(SnakeGame* s) {
if(!s) return 1;
int err= 0;
Game* game= s->game;
GameConfig cfg= s->cfg;
const int board_width= cfg.board_width;
const int board_height= cfg.board_height;
bool has_3d= s->has_3d;
int tick= 0;
HighScore highscores[PERSIST_MAX_SCORES];
int highscore_count= 0;
highscore_count= persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);
render_draw(game_get_state(game), cfg.player_name, highscores, highscore_count);
while(((const struct GameState*)game_get_state(game))->status != GAME_STATUS_GAME_OVER) {
if(platform_was_resized()) {
int new_w= 0, new_h= 0;
if(!platform_get_terminal_size(&new_w, &new_h)) {
new_w= 120;
new_h= 30;
}
if(!tty_size_sufficient_for_board(new_w, new_h, board_width, board_height)) {
render_draw(game_get_state(game), cfg.player_name, highscores, highscore_count);
console_box_paused_terminal_small(new_w, new_h, board_width + 4, board_height + 4);
while(1) {
InputState in= (InputState){0};
input_poll(&in);
if(in.quit) goto clean_done;
int check_w= 0, check_h= 0;
if(!platform_get_terminal_size(&check_w, &check_h)) {
check_w= 120;
check_h= 30;
}
if(tty_size_sufficient_for_board(check_w, check_h, board_width, board_height)) {
console_terminal_resized(check_w, check_h);
break;
}
platform_sleep_ms(250);
}
}
}
InputState input_state= (InputState){0};
input_poll(&input_state);
if(input_state.quit) goto clean_done;
(void)game_enqueue_input(game, 0, &input_state);
GameEvents events= {0};
game_step(game, &events);
if(has_3d) render_3d_on_tick(game_get_state(game));
highscore_count= persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);
for(int ei= 0; ei < events.died_count; ei++) {
int death_score= events.died_scores[ei];
if(death_score > 0) {
persist_append_score(".snake_scores", cfg.player_name, death_score);
render_note_session_score(cfg.player_name, death_score);
highscore_count= persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);
}
}
bool player0_died= false;
for(int ei= 0; ei < events.died_count; ei++)
if(events.died_players[ei] == 0) player0_died= true;
if(player0_died) {
render_draw(game_get_state(game), cfg.player_name, highscores, highscore_count);
render_draw_death_overlay(game_get_state(game), 0, true);
while(1) {
InputState in= (InputState){0};
input_poll(&in);
if(in.quit) goto clean_done;
if(in.any_key) {
game_reset(game);
break;
}
platform_sleep_ms(20);
}
}
uint64_t frame_start= platform_now_ms();
uint64_t frame_deadline= frame_start + (uint64_t)cfg.tick_rate_ms;
uint64_t prev_frame= frame_start;
while(platform_now_ms() < frame_deadline) {
uint64_t now= platform_now_ms();
float delta_s= (float)(now - prev_frame) / 1000.0f;
prev_frame= now;
InputState in= (InputState){0};
input_poll(&in);
if(in.quit) goto clean_done;
(void)game_enqueue_input(game, 0, &in);
render_draw(game_get_state(game), cfg.player_name, highscores, highscore_count);
if(has_3d) render_3d_draw(game_get_state(game), cfg.player_name, highscores, highscore_count, delta_s);
platform_sleep_ms(16);
}
tick++;
}
clean_done:
if(game_player_is_active(game, 0) && game_player_current_score(game, 0) > 0) {
persist_append_score(".snake_scores", cfg.player_name, game_player_current_score(game, 0));
render_note_session_score(cfg.player_name, game_player_current_score(game, 0));
}
if(game) {
game_destroy(game);
s->game= NULL;
}
input_shutdown();
render_shutdown();
if(has_3d) render_3d_shutdown();
console_game_ran(tick);
return err;
}
/* Free the SnakeGame structure. If run() wasn't called or didn't clean up
 * resources, this function will attempt to clean up remaining resources. */
void snake_game_free(SnakeGame* s) {
if(!s) return;
if(s->game) game_destroy(s->game);
/* best-effort shutdown in case run didn't happen */
input_shutdown();
render_shutdown();
if(s->has_3d) render_3d_shutdown();
free(s);
}
