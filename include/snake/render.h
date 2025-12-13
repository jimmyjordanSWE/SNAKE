#pragma once

#include <stdbool.h>

#include "snake/game.h"

bool render_init(int min_width, int min_height);
void render_shutdown(void);
void render_draw(const GameState* game);
