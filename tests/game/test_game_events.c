#include "snake/game.h"
#include "snake/game_internal.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    GameConfig cfg = {.board_width = 5,
                      .board_height = 5,
                      .tick_rate_ms = 100,
                      .render_glyphs = 0,
                      .screen_width = 80,
                      .screen_height = 25,
                      .enable_external_3d_view = 0,
                      .seed = 1337,
                      .num_players = 1,
                      .max_players = 1,
                      .max_length = 8,
                      .max_food = 1};
    Game* g = game_create(&cfg, cfg.seed);
    if (!g) return 2;
    /* Place player so it will eat the single food on next tick. */
    GameState* s = game_test_get_state(g);
    s->players[0].active = true;
    s->players[0].length = 2;
    s->players[0].body[0] = (SnakePoint){2, 2};
    s->players[0].body[1] = (SnakePoint){1, 2};
    s->players[0].current_dir = SNAKE_DIR_RIGHT;
    game_test_set_food(g, (const SnakePoint[]){(SnakePoint){3, 2}}, 1);

    GameEvents ev = {0};
    game_step(g, &ev);

    /* After stepping, player should have eaten, score incremented and
     * food_respawned should be true because there was only one food. */
    assert(s->players[0].score == 1);
    assert(ev.food_respawned == true);

    game_destroy(g);
    return 0;
}
