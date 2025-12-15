#include "snake/game.h"
#include "snake/game_internal.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 20, 10);
    game_config_set_tick_rate_ms(cfg, 100);
    game_config_set_render_glyphs(cfg, 0);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_enable_external_3d_view(cfg, 1);
    game_config_set_seed(cfg, 42);
    game_config_set_fov_degrees(cfg, 90.0f);
    game_config_set_show_sprite_debug(cfg, 0);
    game_config_set_active_player(cfg, 0);
    game_config_set_num_players(cfg, 1);
    game_config_set_max_players(cfg, 2);
    game_config_set_max_length(cfg, 16);
    game_config_set_max_food(cfg, 4);
    Game* g = game_create(cfg, game_config_get_seed(cfg));
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
        game_config_destroy(cfg);
        return 1;
    }
    game_destroy(g);
    game_config_destroy(cfg);
    return 0;
}
