#include "snake/game.h"
#include "snake/collision.h"
#include <stdio.h>

int main(void) {
    GameState g = {0};
    GameConfig cfg = { .board_width = 10, .board_height = 10, .tick_rate_ms = 100, .render_glyphs = 0, .screen_width = 80, .screen_height = 25, .render_mode = 1, .seed = 42, .fov_degrees = 90.0f, .show_minimap = 0, .show_stats = 0, .show_sprite_debug = 0, .active_player = 0, .num_players = 2, .max_players = 4, .max_length = 16, .max_food = 4 };
    game_init(&g, 10, 10, &cfg);
    g.status = GAME_STATUS_RUNNING;

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
        game_free(&g);
        return 1;
    }
    game_free(&g);
    return 0;
}
