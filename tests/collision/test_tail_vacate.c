#include "snake/game.h"
#include "snake/game_internal.h"
#include "snake/collision.h"
#include <stdio.h>

int main(void) {
    GameConfig cfg = {.board_width = 10,
                      .board_height = 10,
                      .tick_rate_ms = 100,
                      .render_glyphs = 0,
                      .screen_width = 80,
                      .screen_height = 25,
                      .enable_external_3d_view = 1,
                      .seed = 42,
                      .fov_degrees = 90.0f,
                      .show_sprite_debug = 0,
                      .active_player = 0,
                      .num_players = 2,
                      .max_players = 4,
                      .max_length = 16,
                      .max_food = 4};
    Game* g = game_create(&cfg, cfg.seed);
    if (!g)
    {
        fprintf(stderr, "FAIL: game_create failed\n");
        return 1;
    }
    GameState* s = game_test_get_state(g);
    s->status = GAME_STATUS_RUNNING;

    /* Player 1: tail at (6,6) will vacate (not eating). */
    PlayerState* p1 = &s->players[1];
    p1->active = true;
    p1->length = 3;
    p1->body[0] = (SnakePoint){5, 5};
    p1->body[1] = (SnakePoint){5, 6};
    p1->body[2] = (SnakePoint){6, 6};
    p1->current_dir = SNAKE_DIR_UP;

    /* Player 0 will move into (6,6) which is p1's tail and should be allowed. */
    PlayerState* p0 = &s->players[0];
    p0->active = true;
    p0->length = 2;
    p0->body[0] = (SnakePoint){6, 5};
    p0->body[1] = (SnakePoint){6, 4};
    p0->current_dir = SNAKE_DIR_DOWN;

    collision_detect_and_resolve(s);

    if (p0->needs_reset)
    {
        fprintf(stderr,
                "FAIL: player 0 should NOT be reset when moving into vacated tail\n");
        game_destroy(g);
        return 1;
    }
    game_destroy(g);
    return 0;
}
