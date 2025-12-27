#pragma once
#include "persist.h"
typedef struct SnakeGame SnakeGame;
SnakeGame* snake_game_new(const GameConfig* config_in, int* err_out);
int snake_game_run(SnakeGame* game);
void snake_game_free(SnakeGame* game);
