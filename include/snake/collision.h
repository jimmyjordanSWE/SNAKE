#pragma once
#include "snake/game.h"
#include "snake/types.h"
#include <stdbool.h>
bool       collision_is_wall(SnakePoint p, int board_w, int board_h);
bool       collision_is_self(SnakePoint p, const PlayerState* player);
bool       collision_is_snake(SnakePoint p, const PlayerState* player);
SnakePoint collision_next_head(SnakePoint current, SnakeDir dir);
void       collision_detect_and_resolve(GameState* game);
