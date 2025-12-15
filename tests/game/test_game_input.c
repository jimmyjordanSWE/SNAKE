#include "snake/game.h"
#include "snake/game_internal.h"
#include <assert.h>

int main(void)
{

        GameConfig* cfg = game_config_create();
        game_config_set_board_size(cfg, 5, 5);
        game_config_set_tick_rate_ms(cfg, 100);
        game_config_set_render_glyphs(cfg, 0);
        game_config_set_screen_size(cfg, 80, 25);
        game_config_set_enable_external_3d_view(cfg, 1);
        game_config_set_seed(cfg, 42);
        game_config_set_fov_degrees(cfg, 90.0f);
        game_config_set_show_sprite_debug(cfg, 0);
        game_config_set_active_player(cfg, 0);
        game_config_set_num_players(cfg, 1);
        game_config_set_max_players(cfg, 4);
        game_config_set_max_length(cfg, 16);
        game_config_set_max_food(cfg, 4);
        Game* g = game_create(cfg, game_config_get_seed(cfg));
    if (!g) return 2;
    GameState* s = game_test_get_state(g);
    s->players[0].active = true;
    s->players[0].length = 2;
    s->players[0].body[0] = (SnakePoint){2, 2};
    s->players[0].body[1] = (SnakePoint){1, 2};
    s->players[0].current_dir = SNAKE_DIR_UP;

    InputState in = {0};
    in.turn_right = true; 
    game_enqueue_input(g, 0, &in);
    GameEvents ev = {0};
    game_step(g, &ev);

    const GameState* st = game_get_state(g);
    
    assert(st->players[0].current_dir == SNAKE_DIR_RIGHT);

        game_destroy(g);
        game_config_destroy(cfg);
    return 0;
}
