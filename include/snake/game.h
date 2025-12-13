#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "snake/types.h"

#define SNAKE_MAX_PLAYERS 2

#define SNAKE_MAX_LENGTH 1024

typedef struct {
    SnakeDir current_dir;
    SnakeDir queued_dir;
    int score;

    /* Per-tick event bookkeeping (cleared at start of each tick) */
    bool died_this_tick;
    int score_at_death;

    SnakePoint body[SNAKE_MAX_LENGTH];
    int length;
    bool active;
    bool needs_reset;
} PlayerState;

typedef struct {
    int width;
    int height;
    uint32_t rng_state;
    GameStatus status;

    int num_players;

    SnakePoint food;
    bool food_present;

    PlayerState players[SNAKE_MAX_PLAYERS];
} GameState;

void game_init(GameState* game, int width, int height, uint32_t seed);
void game_reset(GameState* game);
void game_tick(GameState* game);
