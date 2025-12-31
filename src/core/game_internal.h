
#pragma once
#include "persist.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
struct PlayerState {
SnakeDir current_dir, queued_dir;
int score;
bool died_this_tick;
int score_at_death;
char name[PERSIST_PLAYER_NAME_MAX];
uint32_t color;
SnakePoint body[SNAKE_BODY_MAX_LEN];
int length;
bool active, needs_reset;
/* Multiplayer lives: number of remaining lives (0 for single-player mode).
   Decremented on death; player is eliminated when reaches 0. */
int lives;
/* Mark permanently eliminated (no further respawns) for clarity. */
bool eliminated;
/* Interpolation state for smooth 3D rendering (not used for game logic) */
SnakePointF prev_head;
SnakePointF prev_segment[SNAKE_BODY_MAX_LEN];
bool is_remote;
};


struct GameState {
int width, height;
uint32_t rng_state;
GameStatus status;
int num_players;
int max_players;
SnakePoint* food;
int food_count;
int max_food;
bool last_food_respawned;
struct PlayerState* players;
int max_length;
bool food_sync_only;
};
void game_init(struct GameState* game, int width, int height, const GameConfig* cfg);
void game_free(struct GameState* game);
void game_tick(struct GameState* game);

