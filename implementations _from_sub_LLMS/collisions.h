/* src/core/collision.c */
#include "snake/collision.h"
#include "snake/utils.h"
#include <stdbool.h>

/* Check if point is outside board bounds */
bool collision_is_wall(SnakePoint p, int board_w, int board_h) {
    return p.x < 0 || p.x >= board_w || p.y < 0 || p.y >= board_h;
}

/* Check if point is in snake's body (excluding head) */
bool collision_is_self(SnakePoint p, const Snake* snake) {
    if (snake == NULL || snake->length < 2) { return false; }

    /* Skip the head (index 0), check rest of body */
    for (int i = 1; i < snake->length; i++) {
        SnakePoint segment = snake->body[i];
        if (segment.x == p.x && segment.y == p.y) { return true; }
    }

    return false;
}

/* Check if point is in any part of snake's body */
bool collision_is_snake(SnakePoint p, const Snake* snake) {
    if (snake == NULL || snake->length == 0) { return false; }

    for (int i = 0; i < snake->length; i++) {
        SnakePoint segment = snake->body[i];
        if (segment.x == p.x && segment.y == p.y) { return true; }
    }

    return false;
}

/* Helper: Compute next head position based on direction */
static SnakePoint compute_next_head(SnakePoint current, SnakeDir dir) {
    SnakePoint next = current;

    switch (dir) {
    case DIR_UP:
        next.y--;
        break;
    case DIR_DOWN:
        next.y++;
        break;
    case DIR_LEFT:
        next.x--;
        break;
    case DIR_RIGHT:
        next.x++;
        break;
    }

    return next;
}

/* Main collision detection and resolution */
void collision_detect_and_resolve(GameState* game) {
    if (game == NULL) { return; }

    /* Array to store computed next head positions */
    SnakePoint next_heads[MAX_PLAYERS];
    bool should_reset[MAX_PLAYERS] = {false};

    /* Step 1: Compute all next head positions */
    for (int i = 0; i < game->num_players; i++) {
        PlayerState* player = &game->players[i];

        if (!player->active || player->length == 0) {
            next_heads[i].x = -1;
            next_heads[i].y = -1;
            continue;
        }

        SnakePoint current_head = player->body[0];
        next_heads[i] = compute_next_head(current_head, player->direction);
    }

    /* Step 2: Check collisions against pre-move state */
    for (int i = 0; i < game->num_players; i++) {
        PlayerState* player = &game->players[i];

        if (!player->active || player->length == 0) { continue; }

        SnakePoint next_head = next_heads[i];

        /* Check wall collision */
        if (collision_is_wall(next_head, game->board_width, game->board_height)) {
            should_reset[i] = true;
            continue;
        }

        /* Check self collision (into own body, excluding current head) */
        if (collision_is_self(next_head, player)) {
            should_reset[i] = true;
            continue;
        }

        /* Check collision with other snakes */
        for (int j = 0; j < game->num_players; j++) {
            if (i == j) { continue; /* Don't check against self */ }

            PlayerState* other = &game->players[j];
            if (!other->active || other->length == 0) { continue; }

            /* Check if next head hits other snake's body */
            if (collision_is_snake(next_head, other)) {
                should_reset[i] = true;
                break;
            }
        }
    }

    /* Step 3: Handle head-to-head collisions */
    /* If two heads move into the same cell, both snakes reset */
    for (int i = 0; i < game->num_players; i++) {
        if (!game->players[i].active || game->players[i].length == 0) { continue; }

        for (int j = i + 1; j < game->num_players; j++) {
            if (!game->players[j].active || game->players[j].length == 0) { continue; }

            /* Check if both heads move to same position */
            if (next_heads[i].x == next_heads[j].x && next_heads[i].y == next_heads[j].y) {
                should_reset[i] = true;
                should_reset[j] = true;
            }
        }
    }

    /* Step 4: Mark players for reset */
    for (int i = 0; i < game->num_players; i++) {
        if (should_reset[i]) { game->players[i].needs_reset = true; }
    }
}