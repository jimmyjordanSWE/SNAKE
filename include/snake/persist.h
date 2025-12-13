#pragma once

#include <stdbool.h>

#include "snake/game.h"

bool persist_load_highscores(const char* path);
bool persist_save_highscores(const char* path, const GameState* game);
