#pragma once
#include "persist.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#define SNAKE_MAX_PLAYERS 2
#define SNAKE_MAX_LENGTH 1024
#define SNAKE_MAX_FOOD 3
typedef struct Game Game;
#define GAME_EVENTS_MAX_PLAYERS 8
typedef struct {
int died_players[GAME_EVENTS_MAX_PLAYERS];
int died_scores[GAME_EVENTS_MAX_PLAYERS];
int died_count;
bool game_over;
bool food_respawned;
} GameEvents;
#include "input.h"
typedef struct GameState GameState;
typedef struct PlayerState PlayerState;
// Returns a newly allocated Game; caller must call game_destroy() to free it.
Game* game_create(const GameConfig* cfg, uint32_t seed_override);
void game_destroy(Game* g);
int game_enqueue_input(Game* g, int player_index, const InputState* in);
void game_step(Game* g, GameEvents* out_events);
const GameState* game_get_state(const Game* g);
GameStatus game_get_status(const Game* g);
void game_get_size(const Game* g, int* width, int* height);
int game_get_num_players(const Game* g);
bool game_player_is_active(const Game* g, int player_index);
int game_player_current_score(const Game* g, int player_index);
bool game_player_died_this_tick(const Game* g, int player_index);
int game_player_score_at_death(const Game* g, int player_index);
void game_reset(Game* g);
