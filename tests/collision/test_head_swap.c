#include "snake/game.h"
#include "snake/collision.h"
#include <stdio.h>

int main(void)
{
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

    PlayerState* a = &s->players[0];
    PlayerState* b = &s->players[1];

    a->active = true;
    a->length = 2;
    a->body[0] = (SnakePoint){5, 5};
    a->body[1] = (SnakePoint){4, 5};
    a->current_dir = SNAKE_DIR_RIGHT;

    b->active = true;
    b->length = 2;
    b->body[0] = (SnakePoint){6, 5};
    b->body[1] = (SnakePoint){7, 5};
    b->current_dir = SNAKE_DIR_LEFT;

    collision_detect_and_resolve(s);

    if (!a->needs_reset || !b->needs_reset)
    {
        fprintf(stderr, "FAIL: both players should be reset on head-swap\n");
        game_destroy(g);
        return 1;
    }
    game_destroy(g);
    return 0;
}
