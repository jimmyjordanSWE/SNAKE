#pragma once
#include "snake/types.h"
#include "snake/persist.h"
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
SnakePoint* body;
int length;
int max_length;
bool active, needs_reset;
/* previous head position for interpolation (world coords, centered)
 * updated on each tick before applying movement */
float prev_head_x;
float prev_head_y;
} PlayerState;
typedef struct {
int width, height;
uint32_t rng_state;
GameStatus status;
int num_players;
int max_players;
SnakePoint* food;
int food_count;
int max_food;
PlayerState* players;
int max_length;
} GameState;
/* Initialize `game` to a running state. Width and height are clamped to
 * a minimum of 2. `seed` initializes the module RNG for deterministic
 * behavior in tests. */
/* Initialize `game` to a running state. Width, height and limits are taken
 * from `cfg` (e.g., `cfg->max_players`, `cfg->max_length`, `cfg->max_food`).
 * `seed` initializes the module RNG for deterministic behavior in tests. */
void game_init(GameState* game, int width, int height, const GameConfig* cfg);

/* Free any resources owned by `game`. Safe to call on partially-initialized
 * games (will clean up allocated memory). */
void game_free(GameState* game);

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
 * `game->max_food` entries from `food` into `game`. Intended for tests. */
void game_set_food(GameState* game, const SnakePoint* food, int count);
