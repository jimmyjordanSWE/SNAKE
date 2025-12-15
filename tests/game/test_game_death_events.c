#include "snake/game.h"
#include "snake/game_internal.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 10, 10);
    game_config_set_tick_rate_ms(cfg, 100);
    game_config_set_render_glyphs(cfg, 0);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_enable_external_3d_view(cfg, 0);
    game_config_set_seed(cfg, 42);
    game_config_set_num_players(cfg, 2);
    game_config_set_max_players(cfg, 2);
    game_config_set_max_length(cfg, 8);
    game_config_set_max_food(cfg, 0);
    Game* g = game_create(cfg, game_config_get_seed(cfg));
    if (!g) { game_config_destroy(cfg); return 2; }
    GameState* s = game_test_get_state(g);
    s->status = GAME_STATUS_RUNNING;

    
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

    
    assert(ev.died_count == 2);
    int found0 = 0, found1 = 0;
    for (int i = 0; i < ev.died_count; i++)
    {
        if (ev.died_players[i] == 0) found0 = 1;
        if (ev.died_players[i] == 1) found1 = 1;
    }
    assert(found0 && found1);

    game_destroy(g);
    game_config_destroy(cfg);
    return 0;
}
