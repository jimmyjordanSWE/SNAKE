#include "snake/game.h"
#include "snake/collision.h"
#include <stdio.h>

int main(void) {
    GameState g = {0};
    g.width = 10; g.height = 10; g.num_players = 2; g.status = GAME_STATUS_RUNNING;

    PlayerState *a = &g.players[0];
    PlayerState *b = &g.players[1];

    a->active = true; a->length = 2; a->body[0] = (SnakePoint){5,5}; a->body[1] = (SnakePoint){4,5}; a->current_dir = SNAKE_DIR_RIGHT;
    b->active = true; b->length = 2; b->body[0] = (SnakePoint){6,5}; b->body[1] = (SnakePoint){7,5}; b->current_dir = SNAKE_DIR_LEFT;

    collision_detect_and_resolve(&g);

    if(!a->needs_reset || !b->needs_reset) {
        fprintf(stderr, "FAIL: both players should be reset on head-swap\n");
        return 1;
    }
    return 0;
}
