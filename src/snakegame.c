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
#include <stdbool.h>
#include <stdlib.h>
struct SnakeGame {
GameConfig* cfg;
Game* game;
bool has_3d;
};
SnakeGame* snake_game_new(const GameConfig* config_in, int* err_out) {
int err= 0;
if(err_out) *err_out= 1;
if(!config_in) return NULL;
SnakeGame* s= (SnakeGame*)malloc(sizeof(*s));
if(!s) return NULL;
s->cfg= game_config_create();
if(!s->cfg) {
free(s);
return NULL;
}
int bw= 0, bh= 0;
game_config_get_board_size(config_in, &bw, &bh);
game_config_set_board_size(s->cfg, bw, bh);
int tick= game_config_get_tick_rate_ms(config_in);
if(tick < 10) tick= 10;
game_config_set_tick_rate_ms(s->cfg, tick);
game_config_set_player_name(s->cfg, game_config_get_player_name(config_in));
game_config_set_seed(s->cfg, game_config_get_seed(config_in));
game_config_set_num_players(s->cfg, game_config_get_num_players(config_in));
int sw= 0, sh= 0;
game_config_get_screen_size(config_in, &sw, &sh);
game_config_set_screen_size(s->cfg, sw, sh);
render_set_glyphs((game_config_get_render_glyphs(config_in) == 1) ? RENDER_GLYPHS_ASCII : RENDER_GLYPHS_UTF8);
input_set_key_bindings(game_config_get_key_up(config_in), game_config_get_key_down(config_in), game_config_get_key_left(config_in), game_config_get_key_right(config_in), game_config_get_key_quit(config_in), game_config_get_key_restart(config_in), game_config_get_key_pause(config_in));
const int board_width= bw;
const int board_height= bh;
platform_winch_init();
int term_w= 0, term_h= 0;
while(1) {
if(!platform_get_terminal_size(&term_w, &term_h)) {
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
if(!game_config_get_enable_external_3d_view(config_in)) {
int sw2= 0, sh2= 0;
game_config_get_screen_size(config_in, &sw2, &sh2);
if(sw2 > min_render_w) min_render_w= sw2;
if(sh2 > min_render_h) min_render_h= sh2;
}
if(!render_init(min_render_w, min_render_h)) {
console_error("Failed to initialize rendering\n");
err= 2;
goto out_err;
}
Game* game= game_create(config_in, game_config_get_seed(config_in));
if(!game) {
console_error("Failed to create game\n");
err= 3;
goto cleanup_render;
}
Render3DConfig config_3d= {0};
config_3d.active_player= game_config_get_active_player(config_in);
config_3d.fov_degrees= game_config_get_fov_degrees(config_in);
config_3d.show_sprite_debug= (game_config_get_show_sprite_debug(config_in) != 0);
config_3d.wall_height_scale= game_config_get_wall_height_scale(config_in);
config_3d.tail_height_scale= game_config_get_tail_height_scale(config_in);
int sw3= 0, sh3= 0;
game_config_get_screen_size(config_in, &sw3, &sh3);
config_3d.screen_width= sw3;
config_3d.screen_height= sh3;
if(game_config_get_wall_texture(config_in) && game_config_get_wall_texture(config_in)[0]) snprintf(config_3d.wall_texture_path, (int)sizeof(config_3d.wall_texture_path), "%s", game_config_get_wall_texture(config_in));
if(game_config_get_floor_texture(config_in) && game_config_get_floor_texture(config_in)[0]) snprintf(config_3d.floor_texture_path, (int)sizeof(config_3d.floor_texture_path), "%s", game_config_get_floor_texture(config_in));
bool has_3d= false;
if(game_config_get_enable_external_3d_view(config_in)) {
has_3d= render_3d_init(game_get_state(game), &config_3d);
if(!has_3d)
console_warn("Warning: external 3D view initialization failed, "
             "continuing with 2D only\n");
else {
render_3d_set_tick_rate_ms(game_config_get_tick_rate_ms(config_in));
render_3d_draw(game_get_state(game), game_config_get_player_name(config_in), NULL, 0, 0.0f);
}
}
if(!input_init()) {
console_error("Failed to initialize input\n");
err= 4;
goto cleanup_game;
}
char player_name_buf[PERSIST_PLAYER_NAME_MAX]= {0};
render_draw_startup_screen(player_name_buf, (int)sizeof(player_name_buf));
if(player_name_buf[0]) game_config_set_player_name(s->cfg, player_name_buf);
render_draw(game_get_state(game), game_config_get_player_name(s->cfg), NULL, 0);
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
if(s) {
if(s->cfg) game_config_destroy(s->cfg);
free(s);
}
return NULL;
}
int snake_game_run(SnakeGame* s) {
if(!s) return 1;
int err= 0;
Game* game= s->game;
GameConfig* cfg= s->cfg;
int bw= 0, bh= 0;
game_config_get_board_size(cfg, &bw, &bh);
const int board_width= bw;
const int board_height= bh;
bool has_3d= s->has_3d;
int tick= 0;
HighScore** highscores= NULL;
int highscore_count= persist_read_scores(".snake_scores", &highscores);
render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);
while(((const struct GameState*)game_get_state(game))->status != GAME_STATUS_GAME_OVER) {
if(platform_was_resized()) {
int new_w= 0, new_h= 0;
if(!platform_get_terminal_size(&new_w, &new_h)) {
new_w= 120;
new_h= 30;
}
if(!tty_size_sufficient_for_board(new_w, new_h, board_width, board_height)) {
render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);
console_box_paused_terminal_small(new_w, new_h, board_width + 4, board_height + 4);
while(1) {
InputState in= {0};
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
InputState input_state= {0};
input_poll(&input_state);
if(input_state.quit) goto clean_done;
(void)game_enqueue_input(game, 0, &input_state);
GameEvents events= {0};
game_step(game, &events);
if(has_3d) render_3d_on_tick(game_get_state(game));
if(highscores) {
persist_free_scores(highscores, highscore_count);
highscores= NULL;
}
highscore_count= persist_read_scores(".snake_scores", &highscores);
for(int ei= 0; ei < events.died_count; ei++) {
int death_score= events.died_scores[ei];
if(death_score > 0) {
        /* Only prompt for a name when the score would enter the high score list. */
        HighScore** cur = NULL;
        int cur_cnt = persist_read_scores(".snake_scores", &cur);
        bool qualifies = false;
        if(cur_cnt < PERSIST_MAX_SCORES) qualifies = true;
        for(int i = 0; cur && !qualifies && i < cur_cnt; i++) {
            if(cur[i] && death_score > highscore_get_score(cur[i])) qualifies = true;
        }
        char name_buf[PERSIST_PLAYER_NAME_MAX] = {0};
        if(qualifies) {
            /* prompt for a short name (max 8 chars + NUL) */
            render_prompt_for_highscore_name(name_buf, 9, death_score);
            if(!name_buf[0]) snprintf(name_buf, sizeof(name_buf), "%s", game_config_get_player_name(cfg));
        } else {
            snprintf(name_buf, sizeof(name_buf), "%s", game_config_get_player_name(cfg));
        }
        persist_append_score(".snake_scores", name_buf, death_score);
        render_note_session_score(name_buf, death_score);
        if(cur) { persist_free_scores(cur, cur_cnt); cur = NULL; }
        if(highscores) {
            persist_free_scores(highscores, highscore_count);
            highscores= NULL;
        }
        highscore_count= persist_read_scores(".snake_scores", &highscores);
}
}
bool player0_died= false;
for(int ei= 0; ei < events.died_count; ei++)
if(events.died_players[ei] == 0) player0_died= true;
if(player0_died) {
render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);
render_draw_death_overlay(game_get_state(game), 0, true);
/* Mirror into 3D view if active */
if(has_3d) render_3d_draw_death_overlay(game_get_state(game), 0, true);
while(1) {
InputState in= {0};
input_poll(&in);
if(in.quit) goto clean_done;
if(in.any_key) {
game_reset(game);
break;
}
/* update 3D every loop so external view shows it */
if(has_3d) render_3d_draw_death_overlay(game_get_state(game), 0, true);
platform_sleep_ms(20);
}
}
uint64_t frame_start= platform_now_ms();
uint64_t frame_deadline= frame_start + (uint64_t)game_config_get_tick_rate_ms(cfg);
uint64_t prev_frame= frame_start;
while(platform_now_ms() < frame_deadline) {
uint64_t now= platform_now_ms();
float delta_s= (float)(now - prev_frame) / 1000.0f;
prev_frame= now;
InputState in= {0};
input_poll(&in);
if(in.quit) goto clean_done;
(void)game_enqueue_input(game, 0, &in);
render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);
if(has_3d) render_3d_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count, delta_s);
platform_sleep_ms(16);
}
tick++;
}
clean_done:
if(game_player_is_active(game, 0) && game_player_current_score(game, 0) > 0) {
    int final_score = game_player_current_score(game, 0);
    /* Only prompt for a name when the score would enter the high score list. */
    HighScore** cur = NULL;
    int cur_cnt = persist_read_scores(".snake_scores", &cur);
    bool qualifies = false;
    if(cur_cnt < PERSIST_MAX_SCORES) qualifies = true;
    for(int i = 0; cur && !qualifies && i < cur_cnt; i++) {
        if(cur[i] && final_score > highscore_get_score(cur[i])) qualifies = true;
    }
    char name_buf[PERSIST_PLAYER_NAME_MAX] = {0};
    if(qualifies) {
        render_prompt_for_highscore_name(name_buf, 9, final_score);
        if(!name_buf[0]) snprintf(name_buf, sizeof(name_buf), "%s", game_config_get_player_name(cfg));
    } else {
        snprintf(name_buf, sizeof(name_buf), "%s", game_config_get_player_name(cfg));
    }
    persist_append_score(".snake_scores", name_buf, final_score);
    render_note_session_score(name_buf, final_score);
    if(cur) persist_free_scores(cur, cur_cnt);
}
if(highscores) {
persist_free_scores(highscores, highscore_count);
highscores= NULL;
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
void snake_game_free(SnakeGame* s) {
if(!s) return;
if(s->game) game_destroy(s->game);
input_shutdown();
render_shutdown();
if(s->has_3d) render_3d_shutdown();
if(s->cfg) game_config_destroy(s->cfg);
free(s);
}
