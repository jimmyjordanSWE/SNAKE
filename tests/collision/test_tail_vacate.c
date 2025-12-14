#include "snake/game.h"
#include "snake/collision.h"
#include <stdio.h>

int main(void) {
    GameState g = {0};
    g.width = 10; g.height = 10; g.num_players = 2; g.status = GAME_STATUS_RUNNING;

    /* Player 1: tail at (6,6) will vacate (not eating). */
    PlayerState *p1 = &g.players[1];
    p1->active = true; p1->length = 3;
    p1->body[0] = (SnakePoint){5,5};
    p1->body[1] = (SnakePoint){5,6};
    p1->body[2] = (SnakePoint){6,6};
    p1->current_dir = SNAKE_DIR_UP;

    /* Player 0 will move into (6,6) which is p1's tail and should be allowed. */
    PlayerState *p0 = &g.players[0];
    p0->active = true; p0->length = 2;
    p0->body[0] = (SnakePoint){6,5};
    p0->body[1] = (SnakePoint){6,4};
    p0->current_dir = SNAKE_DIR_DOWN;

    collision_detect_and_resolve(&g);

    if(p0->needs_reset) {
        fprintf(stderr, "FAIL: player 0 should NOT be reset when moving into vacated tail\n");
        return 1;
    }
    return 0;
}
