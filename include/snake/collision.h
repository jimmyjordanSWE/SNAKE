#pragma once
#include "snake/types.h"
#include <stdbool.h>
/* Forward-declare `PlayerState` and `GameState` tags - users don't need the
 * full definitions to call collision APIs. Internal code should include
 * `game_internal.h`. */
struct PlayerState;
struct GameState;
bool collision_is_wall(SnakePoint p, int board_w, int board_h);
bool collision_is_self(SnakePoint p, const struct PlayerState* player);
bool collision_is_snake(SnakePoint p, const struct PlayerState* player);
SnakePoint collision_next_head(SnakePoint current, SnakeDir dir);
void collision_detect_and_resolve(struct GameState* game);
