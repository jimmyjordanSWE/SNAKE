#include "snake/game.h"
#include "snake/game_internal.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    GameConfig cfg = {.board_width = 20,
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
                      .num_players = 1,
                      .max_players = 2,
                      .max_length = 16,
                      .max_food = 4};
    Game* g = game_create(&cfg, cfg.seed);
    if (!g)
    {
        fprintf(stderr, "FAIL: game_create failed\n");
        return 1;
    }
    
    game_test_set_dimensions(g, 1, 1); 
    game_test_set_num_players(g, 1);
    
    GameState* s = game_test_get_state(g);
    s->players[0].active = false;
    s->players[0].length = 0;

    GameEvents ev = {0};
    game_step(g, &ev);

    const GameState* st = game_get_state(g);
    if (st->status != GAME_STATUS_GAME_OVER)
    {
        fprintf(stderr,
                "FAIL: expected GAME_OVER when no players can be spawned\n");
        game_destroy(g);
        return 1;
    }
    game_destroy(g);
    return 0;
}
