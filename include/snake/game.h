#pragma once
#include "snake/types.h"
#include <stdbool.h>
#include <stdint.h>
#define SNAKE_MAX_PLAYERS 2
#define SNAKE_MAX_LENGTH 1024
#define SNAKE_MAX_FOOD 3
typedef struct {
SnakeDir current_dir, queued_dir;
int score;
bool died_this_tick;
int score_at_death;
SnakePoint body[SNAKE_MAX_LENGTH];
int length;
bool active, needs_reset;
} PlayerState;
typedef struct {
int width, height;
uint32_t rng_state;
GameStatus status;
int num_players;
SnakePoint food[SNAKE_MAX_FOOD];
int food_count;
PlayerState players[SNAKE_MAX_PLAYERS];
} GameState;
/* Initialize `game` to a running state. Width and height are clamped to
 * a minimum of 2. `seed` initializes the module RNG for deterministic
 * behavior in tests. */
void game_init(GameState* game, int width, int height, uint32_t seed);

/* Reset `game` while preserving configuration like `num_players`. */
void game_reset(GameState* game);

/* Advance the simulation by one tick. Handles movement, collisions, and
 * food consumption. */
void game_tick(GameState* game);

/* Query helpers: return safe defaults for invalid inputs (see implementation). */
int game_get_num_players(const GameState* game);
bool game_player_is_active(const GameState* game, int player_index);
int game_player_current_score(const GameState* game, int player_index);
bool game_player_died_this_tick(const GameState* game, int player_index);
int game_player_score_at_death(const GameState* game, int player_index);

/* Test helper: set the food positions deterministically. Copies up to
 * `SNAKE_MAX_FOOD` entries from `food` into `game`. Intended for tests. */
void game_set_food(GameState* game, const SnakePoint* food, int count);
