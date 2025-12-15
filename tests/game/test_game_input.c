#include "snake/game.h"
#include <assert.h>

int main(void)
{
    GameConfig cfg = {.board_width = 5,
                      .board_height = 5,
                      .tick_rate_ms = 100,
                      .render_glyphs = 0,
                      .screen_width = 80,
                      .screen_height = 25,
                      .enable_external_3d_view = 0,
                      .seed = 999,
                      .num_players = 1,
                      .max_players = 1,
                      .max_length = 8,
                      .max_food = 0};
    Game* g = game_create(&cfg, cfg.seed);
    if (!g) return 2;
    GameState* s = game_test_get_state(g);
    s->players[0].active = true;
    s->players[0].length = 2;
    s->players[0].body[0] = (SnakePoint){2, 2};
    s->players[0].body[1] = (SnakePoint){1, 2};
    s->players[0].current_dir = SNAKE_DIR_UP;

    InputState in = {0};
    in.turn_right = true; /* from UP, turn_right -> RIGHT */
    game_enqueue_input(g, 0, &in);
    GameEvents ev = {0};
    game_step(g, &ev);

    const GameState* st = game_get_state(g);
    /* After a tick, current_dir should reflect the turn */
    assert(st->players[0].current_dir == SNAKE_DIR_RIGHT);

    game_destroy(g);
    return 0;
}
