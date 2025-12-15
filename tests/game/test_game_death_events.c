#include "snake/game.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    GameConfig cfg = {.board_width = 10,
                      .board_height = 10,
                      .tick_rate_ms = 100,
                      .render_glyphs = 0,
                      .screen_width = 80,
                      .screen_height = 25,
                      .enable_external_3d_view = 0,
                      .seed = 42,
                      .num_players = 2,
                      .max_players = 2,
                      .max_length = 8,
                      .max_food = 0};
    Game* g = game_create(&cfg, cfg.seed);
    if (!g) return 2;
    GameState* s = game_test_get_state(g);
    s->status = GAME_STATUS_RUNNING;

    /* Setup head-swap scenario */
    s->players[0].active = true;
    s->players[0].length = 2;
    s->players[0].body[0] = (SnakePoint){4, 5};
    s->players[0].body[1] = (SnakePoint){4, 4};
    s->players[0].current_dir = SNAKE_DIR_RIGHT;

    s->players[1].active = true;
    s->players[1].length = 2;
    s->players[1].body[0] = (SnakePoint){6, 5};
    s->players[1].body[1] = (SnakePoint){7, 5};
    s->players[1].current_dir = SNAKE_DIR_LEFT;

    GameEvents ev = {0};
    game_step(g, &ev);

    /* Both should die in a head-swap */
    assert(ev.died_count == 2);
    int found0 = 0, found1 = 0;
    for (int i = 0; i < ev.died_count; i++)
    {
        if (ev.died_players[i] == 0) found0 = 1;
        if (ev.died_players[i] == 1) found1 = 1;
    }
    assert(found0 && found1);

    game_destroy(g);
    return 0;
}
