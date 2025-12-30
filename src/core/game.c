#include "game.h"
#include "collision.h"
#include "direction.h"
#include "game_internal.h"
#include "input.h"
#include "player.h"
#include "utils.h"
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
void game_init(GameState* game, int width, int height, const GameConfig* cfg);
void game_free(GameState* game);
void game_tick(GameState* game);
static void game_state_reset(GameState* game);
static int game_state_get_num_players(const GameState* game);
static bool game_state_player_is_active(const GameState* game, int player_index);
static int game_state_player_current_score(const GameState* game, int player_index);
static bool game_state_player_died_this_tick(const GameState* game, int player_index);
static int game_state_player_score_at_death(const GameState* game, int player_index);
static bool spawn_player(GameState* game, int player_index);
struct Game {
GameState state;
};
Game* game_create(const GameConfig* cfg, uint32_t seed_override) {
if(!cfg) return NULL;
Game* g= malloc(sizeof *g);
if(!g) return NULL;
int bw= 0, bh= 0;
game_config_get_board_size(cfg, &bw, &bh);
(void)memset(g, 0, sizeof(*g));
game_init(&g->state, bw, bh, cfg);
if(seed_override != 0) { snake_rng_seed(&g->state.rng_state, seed_override); }
return g;
}
void game_destroy(Game* g) {
if(!g) return;
game_free(&g->state);
free(g);
}
int game_enqueue_input(Game* g, int player_index, const InputState* in) {
if(!g || !in) return -1;
if(player_index < 0 || player_index >= g->state.max_players) return -1;
PlayerState* player= &g->state.players[player_index];
if(in->move_up) player->queued_dir= SNAKE_DIR_UP;
if(in->move_down) player->queued_dir= SNAKE_DIR_DOWN;
if(in->move_left) player->queued_dir= SNAKE_DIR_LEFT;
if(in->move_right) player->queued_dir= SNAKE_DIR_RIGHT;
if(in->turn_left) player->queued_dir= snake_dir_turn_left(player->current_dir);
if(in->turn_right) player->queued_dir= snake_dir_turn_right(player->current_dir);
if(in->pause_toggle) g->state.status= (g->state.status == GAME_STATUS_PAUSED) ? GAME_STATUS_RUNNING : GAME_STATUS_PAUSED;
if(in->restart) game_state_reset(&g->state);
return 0;
}
void game_step(Game* g, GameEvents* out_events) {
if(!g) return;
if(out_events) (void)memset(out_events, 0, sizeof(*out_events));
g->state.last_food_respawned= false;
game_tick(&g->state);
if(out_events && g->state.last_food_respawned) out_events->food_respawned= true;
g->state.last_food_respawned= false;
int num_players= game_state_get_num_players(&g->state);
if(!out_events) return;
out_events->died_count= 0;
for(int i= 0; i < num_players && out_events->died_count < GAME_EVENTS_MAX_PLAYERS; i++) {
if(g->state.players[i].died_this_tick) {
int idx= out_events->died_count++;
out_events->died_players[idx]= i;
out_events->died_scores[idx]= g->state.players[i].score_at_death;
}
}
if(g->state.status == GAME_STATUS_GAME_OVER) out_events->game_over= true;
}
const GameState* game_get_state(const Game* g) {
if(!g) return NULL;
return &g->state;
}
GameStatus game_get_status(const Game* g) {
if(!g) return GAME_STATUS_GAME_OVER;
return g->state.status;
}
void game_get_size(const Game* g, int* width, int* height) {
if(!g) {
if(width) *width= 0;
if(height) *height= 0;
return;
}
if(width) *width= g->state.width;
if(height) *height= g->state.height;
}
int game_get_num_players(const Game* g) {
if(!g) return 0;
return game_state_get_num_players(&g->state);
}
int game_add_player(Game* g, const PlayerCfg* cfg) {
if(!g || !cfg) return -1;
GameState* s= &g->state;
if(s->num_players >= s->max_players) return -1;
int idx= s->num_players++;
const char* name= player_cfg_get_name(cfg);
if(name && name[0]) {
strncpy(s->players[idx].name, name, PERSIST_PLAYER_NAME_MAX - 1);
s->players[idx].name[PERSIST_PLAYER_NAME_MAX - 1]= '\0';
} else {
s->players[idx].name[0]= '\0';
}
s->players[idx].color= player_cfg_get_color(cfg);
s->players[idx].score= 0;
s->players[idx].died_this_tick= false;
s->players[idx].score_at_death= 0;
s->players[idx].active= false;
s->players[idx].needs_reset= false;
s->players[idx].length= 0;
s->players[idx].max_length= s->max_length;
(void)spawn_player(s, idx);
return idx;
}
bool game_player_is_active(const Game* g, int player_index) {
if(!g) return false;
return game_state_player_is_active(&g->state, player_index);
}
int game_player_current_score(const Game* g, int player_index) {
if(!g) return 0;
return game_state_player_current_score(&g->state, player_index);
}
bool game_player_died_this_tick(const Game* g, int player_index) {
if(!g) return false;
return game_state_player_died_this_tick(&g->state, player_index);
}
int game_player_score_at_death(const Game* g, int player_index) {
if(!g) return 0;
return game_state_player_score_at_death(&g->state, player_index);
}
#define SPAWN_MAX_ATTEMPTS 1000
#define FOOD_RESPAWN_MIN 1
#define FOOD_RESPAWN_MAX 3
#define FOOD_MIN_ATTEMPTS 32
static SnakePoint random_point(GameState* game) {
SnakePoint p;
p.x= snake_rng_range(&game->rng_state, 0, game->width - 1);
p.y= snake_rng_range(&game->rng_state, 0, game->height - 1);
return p;
}
static bool point_in_player(const PlayerState* player, SnakePoint p) {
if(player == NULL || !player->active || player->length <= 0) return false;
for(int i= 0; i < player->length; i++) {
SnakePoint s= player->body[i];
if(s.x == p.x && s.y == p.y) return true;
}
return false;
}
static bool point_in_any_snake(const GameState* game, SnakePoint p) {
if(game == NULL) return false;
for(int i= 0; i < game->max_players; i++)
if(point_in_player(&game->players[i], p)) return true;
return false;
}
static bool point_is_food(const GameState* game, SnakePoint p) {
if(game == NULL) return false;
for(int i= 0; i < game->food_count; i++)
if(game->food[i].x == p.x && game->food[i].y == p.y) return true;
return false;
}
static void food_respawn(GameState* game) {
if(game == NULL) return;
int num_to_spawn= snake_rng_range(&game->rng_state, FOOD_RESPAWN_MIN, FOOD_RESPAWN_MAX);
game->food_count= 0;
int max_attempts= game->width * game->height * 2;
if(max_attempts < FOOD_MIN_ATTEMPTS) max_attempts= FOOD_MIN_ATTEMPTS;
for(int i= 0; i < num_to_spawn && game->food_count < game->max_food; i++) {
for(int attempt= 0; attempt < max_attempts; attempt++) {
SnakePoint p= random_point(game);
if(!point_in_any_snake(game, p) && !point_is_food(game, p)) {
game->food[game->food_count]= p;
game->food_count++;
break;
}
}
}
game->last_food_respawned= true;
}
static void player_move(PlayerState* player, SnakePoint next_head, bool grow) {
if(player == NULL || !player->active || player->length <= 0) return;
if(player->prev_segment_x && player->prev_segment_y) {
for(int i= 0; i < player->length; ++i) {
player->prev_segment_x[i]= (float)player->body[i].x + 0.5f;
player->prev_segment_y[i]= (float)player->body[i].y + 0.5f;
}
}
bool actual_grow= grow && player->length < player->max_length;
int last= actual_grow ? player->length : (player->length - 1);
for(int i= last; i > 0; i--) player->body[i]= player->body[i - 1];
player->body[0]= next_head;
if(actual_grow) {
if(player->prev_segment_x && player->prev_segment_y) {
player->prev_segment_x[last]= (float)player->body[last].x + 0.5f;
player->prev_segment_y[last]= (float)player->body[last].y + 0.5f;
}
player->length++;
}
}
static SnakeDir opposite_dir(SnakeDir dir) {
switch(dir) {
case SNAKE_DIR_UP: return SNAKE_DIR_DOWN;
case SNAKE_DIR_DOWN: return SNAKE_DIR_UP;
case SNAKE_DIR_LEFT: return SNAKE_DIR_RIGHT;
case SNAKE_DIR_RIGHT: return SNAKE_DIR_LEFT;
default: return SNAKE_DIR_UP;
}
}
/* Default palette used when player color is not set via PlayerCfg. */
static const uint32_t DEFAULT_PLAYER_COLS[]= {0xFF008000u, 0xFF00C8FFu, 0xFFFFF700u, 0xFFFF00FFu};
static bool spawn_player(GameState* game, int player_index) {
if(game == NULL || player_index < 0 || player_index >= game->max_players) return false;
if(game->width < 2 || game->height < 1) return false;
PlayerState* player= &game->players[player_index];
/* If color has not been configured, assign a palette color based on index. */
if(player->color == 0) { player->color= DEFAULT_PLAYER_COLS[player_index % (int)(sizeof(DEFAULT_PLAYER_COLS) / sizeof(DEFAULT_PLAYER_COLS[0]))]; }
player->active= true;
player->needs_reset= false;
player->length= 2;
for(int attempt= 0; attempt < SPAWN_MAX_ATTEMPTS; attempt++) {
SnakePoint head= (SnakePoint){
    .x= snake_rng_range(&game->rng_state, 2, game->width - 3),
    .y= snake_rng_range(&game->rng_state, 2, game->height - 3),
};
int dist_left= head.x;
int dist_right= game->width - 1 - head.x;
int dist_up= head.y;
int dist_down= game->height - 1 - head.y;
int max_dist= dist_left;
SnakeDir best_dir= SNAKE_DIR_LEFT;
if(dist_right > max_dist) {
max_dist= dist_right;
best_dir= SNAKE_DIR_RIGHT;
}
if(dist_up > max_dist) {
max_dist= dist_up;
best_dir= SNAKE_DIR_UP;
}
if(dist_down > max_dist) {
max_dist= dist_down;
best_dir= SNAKE_DIR_DOWN;
}
player->current_dir= best_dir;
player->queued_dir= best_dir;
SnakePoint tail= collision_next_head(head, opposite_dir(best_dir));
if(collision_is_wall(tail, game->width, game->height)) continue;
bool overlaps= false;
for(int i= 0; i < game->max_players; i++) {
if(i == player_index) continue;
if(point_in_player(&game->players[i], head) || point_in_player(&game->players[i], tail)) {
overlaps= true;
break;
}
}
if(!overlaps) {
player->body[0]= head;
player->body[1]= tail;
player->prev_head_x= (float)head.x + 0.5f;
player->prev_head_y= (float)head.y + 0.5f;
if(player->prev_segment_x && player->prev_segment_y) {
player->prev_segment_x[0]= (float)player->body[0].x + 0.5f;
player->prev_segment_y[0]= (float)player->body[0].y + 0.5f;
player->prev_segment_x[1]= (float)player->body[1].x + 0.5f;
player->prev_segment_y[1]= (float)player->body[1].y + 0.5f;
for(int k= 2; k < player->max_length; ++k) {
player->prev_segment_x[k]= 0.0f;
player->prev_segment_y[k]= 0.0f;
}
}
return true;
}
}
player->active= false;
player->length= 0;
return false;
}
void game_init(GameState* game, int width, int height, const GameConfig* cfg) {
if(!game || !cfg) return;
game->width= (width < 2) ? 2 : width;
game->height= (height < 2) ? 2 : height;
snake_rng_seed(&game->rng_state, game_config_get_seed(cfg));
game->status= GAME_STATUS_RUNNING;
game->food_count= 0;
game->num_players= game_config_get_num_players(cfg);
if(game->num_players < 1) game->num_players= 1;
game->max_players= game_config_get_max_players(cfg);
game->max_length= game_config_get_max_length(cfg);
game->max_food= game_config_get_max_food(cfg);
if((size_t)game->max_players > SIZE_MAX / sizeof(PlayerState)) return;
if((size_t)game->max_food > SIZE_MAX / sizeof(SnakePoint)) return;
game->players= calloc((size_t)game->max_players, sizeof(PlayerState));
if(!game->players) return;
if(game->max_food > 0) {
game->food= malloc((size_t)game->max_food * sizeof *game->food);
if(!game->food) {
free(game->players);
game->players= NULL;
return;
}
}
size_t total_elements= (size_t)game->max_players * (size_t)game->max_length;
SnakePoint* all_bodies= malloc(total_elements * sizeof *all_bodies);
float* all_prev_x= malloc(total_elements * sizeof *all_prev_x);
float* all_prev_y= malloc(total_elements * sizeof *all_prev_y);
if(!all_bodies || !all_prev_x || !all_prev_y) {
free(all_bodies);
free(all_prev_x);
free(all_prev_y);
goto alloc_fail;
}
for(int i= 0; i < game->max_players; i++) {
game->players[i].score= 0;
game->players[i].died_this_tick= false;
game->players[i].score_at_death= 0;
game->players[i].active= false;
game->players[i].needs_reset= false;
game->players[i].length= 0;
game->players[i].max_length= game->max_length;
/* Initialize multiplayer lives: 3 when starting a multiplayer match; 0 in single-player mode. */
game->players[i].lives= (game->num_players > 1) ? 3 : 0;
game->players[i].eliminated= false;
game->players[i].body= (SnakePoint*)all_bodies + (size_t)i * (size_t)game->max_length;
game->players[i].prev_segment_x= (float*)all_prev_x + (size_t)i * (size_t)game->max_length;
game->players[i].prev_segment_y= (float*)all_prev_y + (size_t)i * (size_t)game->max_length;
}
goto alloc_ok;
alloc_fail:
free(game->food);
free(game->players);
game->players= NULL;
game->food= NULL;
return;
alloc_ok:;
for(int i= 0; i < game->num_players; i++) (void)spawn_player(game, i);
food_respawn(game);
}
void game_free(GameState* game) {
if(!game) return;
if(game->players) {
if(game->max_players > 0) {
free(game->players[0].body);
free(game->players[0].prev_segment_x);
free(game->players[0].prev_segment_y);
}
free(game->players);
game->players= NULL;
}
if(game->food) {
free(game->food);
game->food= NULL;
}
}
static void game_state_reset(GameState* game) {
if(!game) return;
for(int i= 0; i < game->max_players; i++) {
game->players[i].score= 0;
game->players[i].died_this_tick= false;
game->players[i].score_at_death= 0;
game->players[i].active= false;
game->players[i].needs_reset= false;
game->players[i].length= 0;
game->players[i].max_length= game->max_length;
/* Restore multiplayer lives and clear elimination state on reset. */
game->players[i].lives= (game->num_players > 1) ? 3 : 0;
game->players[i].eliminated= false;
game->players[i].prev_head_x= 0.0f;
game->players[i].prev_head_y= 0.0f;
}
for(int i= 0; i < game->num_players; i++) (void)spawn_player(game, i);
game->status= GAME_STATUS_RUNNING;
food_respawn(game);
}
void game_reset(Game* g) {
if(!g) return;
game_state_reset(&g->state);
}
void game_tick(GameState* game) {
if(game == NULL) return;
if(game->status != GAME_STATUS_RUNNING) return;
int num_players= game->num_players;
if(num_players < 0) num_players= 0;
if(num_players > SNAKE_MAX_PLAYERS) num_players= SNAKE_MAX_PLAYERS;
for(int i= 0; i < num_players; i++) {
game->players[i].needs_reset= false;
game->players[i].died_this_tick= false;
game->players[i].score_at_death= 0;
}
for(int i= 0; i < num_players; i++) {
PlayerState* player= &game->players[i];
if(!player->active || player->length <= 0) continue;
if(player->queued_dir != opposite_dir(player->current_dir)) player->current_dir= player->queued_dir;
}
collision_detect_and_resolve(game);
/* Handle deaths: decrement lives in multiplayer mode and respawn/eliminate accordingly. */
for(int i= 0; i < num_players; i++) {
PlayerState* player= &game->players[i];
if(!player->active) continue;
if(!player->needs_reset) continue;
player->died_this_tick= true;
player->score_at_death= player->score;
player->score= 0;
if(game->num_players > 1) {
/* Multiplayer elimination mode: decrement lives and eliminate when 0. */
player->lives--;
if(player->lives <= 0) {
player->eliminated= true;
player->active= false;
player->length= 0;
player->needs_reset= false;
} else {
/* Respawn immediately if lives remain. */
player->needs_reset= false;
(void)spawn_player(game, i);
}
}
}
/* Single-player fallback: respawn any players that need reset. In multiplayer, respawn was handled above. */
for(int i= 0; i < num_players; i++) {
PlayerState* player= &game->players[i];
if(player->needs_reset) {
if(game->num_players > 1) {
if(player->lives > 0)
(void)spawn_player(game, i);
else {
player->active= false;
player->length= 0;
player->needs_reset= false;
}
} else {
(void)spawn_player(game, i);
}
}
}
{
int active_count= 0;
for(int i= 0; i < num_players; i++)
if(game->players[i].active) active_count++;
/* Multiplayer: end when <= 1 active players remain (last man standing).
                           Single-player: end when no active players remain. */
if(game->num_players > 1) {
if(active_count <= 1) game->status= GAME_STATUS_GAME_OVER;
} else {
if(active_count == 0) game->status= GAME_STATUS_GAME_OVER;
}
}
bool food_consumed= false;
for(int i= 0; i < num_players; i++) {
PlayerState* player= &game->players[i];
if(!player->active || player->length <= 0) continue;
if(player->needs_reset) continue;
SnakePoint current_head= player->body[0];
player->prev_head_x= (float)current_head.x + 0.5f;
player->prev_head_y= (float)current_head.y + 0.5f;
SnakePoint next_head= collision_next_head(current_head, player->current_dir);
bool eat= false;
for(int f= 0; f < game->food_count; f++) {
if(next_head.x == game->food[f].x && next_head.y == game->food[f].y) {
eat= true;
for(int j= f; j < game->food_count - 1; j++) game->food[j]= game->food[j + 1];
game->food_count--;
food_consumed= true;
break;
}
}
player_move(player, next_head, eat);
if(eat) player->score++;
}
if(food_consumed && game->food_count == 0) food_respawn(game);
}
static int game_state_get_num_players(const GameState* game) {
if(game == NULL) return 0;
int count= game->num_players;
if(count < 0) return 0;
if(count > game->max_players) return game->max_players;
return count;
}
static bool game_state_player_is_active(const GameState* game, int player_index) {
if(game == NULL || player_index < 0 || player_index >= game->max_players) return false;
return game->players[player_index].active;
}
static int game_state_player_current_score(const GameState* game, int player_index) {
if(game == NULL || player_index < 0 || player_index >= game->max_players) return 0;
return game->players[player_index].score;
}
static bool game_state_player_died_this_tick(const GameState* game, int player_index) {
if(game == NULL || player_index < 0 || player_index >= game->max_players) return false;
return game->players[player_index].died_this_tick;
}
static int game_state_player_score_at_death(const GameState* game, int player_index) {
if(game == NULL || player_index < 0 || player_index >= game->max_players) return 0;
return game->players[player_index].score_at_death;
}
