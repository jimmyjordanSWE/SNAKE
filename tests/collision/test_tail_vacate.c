#include "snake/game.h"
#include "snake/game_internal.h"
#include "snake/collision.h"
#include <stdio.h>

int main(void) {
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 10, 10);
    game_config_set_tick_rate_ms(cfg, 100);
    game_config_set_render_glyphs(cfg, 0);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_enable_external_3d_view(cfg, 1);
    game_config_set_seed(cfg, 42);
    game_config_set_fov_degrees(cfg, 90.0f);
    game_config_set_show_sprite_debug(cfg, 0);
    game_config_set_active_player(cfg, 0);
    game_config_set_num_players(cfg, 2);
    game_config_set_max_players(cfg, 4);
    game_config_set_max_length(cfg, 16);
    game_config_set_max_food(cfg, 4);
    Game* g = game_create(cfg, game_config_get_seed(cfg));
    if (!g)
    {
        fprintf(stderr, "FAIL: game_create failed\n");
        return 1;
    }
    GameState* s = game_test_get_state(g);
    s->status = GAME_STATUS_RUNNING;

    
    PlayerState* p1 = &s->players[1];
    p1->active = true;
    p1->length = 3;
    p1->body[0] = (SnakePoint){5, 5};
    p1->body[1] = (SnakePoint){5, 6};
    p1->body[2] = (SnakePoint){6, 6};
    p1->current_dir = SNAKE_DIR_UP;

    
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
    game_config_destroy(cfg);
    return 0;
}
