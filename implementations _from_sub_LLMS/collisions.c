/* include/snake/collision.h */
#ifndef SNAKE_COLLISION_H
#define SNAKE_COLLISION_H

#include "game.h"
#include "types.h"

/**
 * Collision detection and resolution module.
 *
 * Implements deterministic collision rules:
 * - Snake vs wall
 * - Snake vs itself
 * - Snake vs snake (multiplayer)
 * - Head-to-head special cases
 *
 * All collision checks are performed against the game state at the
 * start of the tick (pre-move), ensuring reproducible results.
 */

/**
 * Check if a point is outside the game board bounds.
 *
 * @param p The point to check
 * @param board_w Board width
 * @param board_h Board height
 * @return true if point is outside bounds, false otherwise
 */
bool collision_is_wall(SnakePoint p, int board_w, int board_h);

/**
 * Check if a point collides with a snake's body (excluding head).
 * Used for self-collision detection.
 *
 * @param p The point to check
 * @param snake The snake to check against
 * @return true if point is in snake body (not head), false otherwise
 */
bool collision_is_self(SnakePoint p, const Snake* snake);

/**
 * Check if a point collides with any part of another snake's body.
 *
 * @param p The point to check
 * @param snake The snake to check against
 * @return true if point is in snake body, false otherwise
 */
bool collision_is_snake(SnakePoint p, const Snake* snake);

/**
 * Perform collision detection for all players and update their states.
 *
 * This function:
 * 1. Computes next head positions for all active snakes
 * 2. Checks wall, self, and snake-vs-snake collisions
 * 3. Handles head-to-head special cases
 * 4. Marks collided snakes for reset
 *
 * Collision evaluation uses the board state at the start of the tick
 * to ensure deterministic behavior with simultaneous movement.
 *
 * @param game The game state to check and update
 */
void collision_detect_and_resolve(GameState* game);

#endif /* SNAKE_COLLISION_H */