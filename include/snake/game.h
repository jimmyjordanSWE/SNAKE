#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "snake/types.h"

#define SNAKE_MAX_PLAYERS 2

typedef struct {
    SnakeDir current_dir;
    SnakeDir queued_dir;
    int score;
} PlayerState;

typedef struct {
    int width;
    int height;
    uint32_t rng_state;
    GameStatus status;

    SnakePoint food;
    bool food_present;

    PlayerState players[SNAKE_MAX_PLAYERS];
} GameState;

void game_init(GameState* game, int width, int height, uint32_t seed);
void game_reset(GameState* game);
void game_tick(GameState* game);
