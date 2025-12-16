#pragma once
#include "snake/persist.h"
#include "snake/types.h"
#include <stdbool.h>
#include <stdint.h>
#define SNAKE_MAX_PLAYERS 2
#define SNAKE_MAX_LENGTH  1024
#define SNAKE_MAX_FOOD    3
typedef struct Game Game;
#define GAME_EVENTS_MAX_PLAYERS 8
typedef struct
{
    int  died_players[GAME_EVENTS_MAX_PLAYERS];
    int  died_scores[GAME_EVENTS_MAX_PLAYERS];
    int  died_count;
    bool game_over;
    bool food_respawned;
} GameEvents;
#include "snake/input.h"
typedef struct GameState   GameState;
typedef struct PlayerState PlayerState;
Game* game_create(const GameConfig* cfg, uint32_t seed_override);
void  game_destroy(Game* g);
int   game_enqueue_input(Game* g, int player_index, const InputState* in);
void  game_step(Game* g, GameEvents* out_events);
const GameState* game_get_state(const Game* g);
int              game_get_num_players(const Game* g);
bool             game_player_is_active(const Game* g, int player_index);
int              game_player_current_score(const Game* g, int player_index);
bool             game_player_died_this_tick(const Game* g, int player_index);
int              game_player_score_at_death(const Game* g, int player_index);
void             game_reset(Game* g);
void             game_test_set_dimensions(Game* g, int width, int height);
void             game_test_set_num_players(Game* g, int n);
void game_test_set_player_needs_reset(Game* g, int player_index, bool needs);
void game_test_set_food(Game* g, const SnakePoint* food, int count);
GameState* game_test_get_state(Game* g);
