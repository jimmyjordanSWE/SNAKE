#include "snake/game.h"
#include "snake/game_internal.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 5, 5);
    game_config_set_tick_rate_ms(cfg, 100);
    game_config_set_render_glyphs(cfg, 0);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_enable_external_3d_view(cfg, 0);
    game_config_set_seed(cfg, 1337);
    game_config_set_num_players(cfg, 1);
    game_config_set_max_players(cfg, 1);
    game_config_set_max_length(cfg, 8);
    game_config_set_max_food(cfg, 1);
    Game* g = game_create(cfg, game_config_get_seed(cfg));
    if (!g) { game_config_destroy(cfg); return 2; }

    GameState* s = game_test_get_state(g);
    s->players[0].active = true;
    s->players[0].length = 2;
    s->players[0].body[0] = (SnakePoint){2, 2};
    s->players[0].body[1] = (SnakePoint){1, 2};
    s->players[0].current_dir = SNAKE_DIR_RIGHT;
    /* Place food directly in front of head so the next step grows the snake */
    game_test_set_food(g, (const SnakePoint[]){(SnakePoint){3, 2}}, 1);

    GameEvents ev = {0};
    game_step(g, &ev);

    /* After eating, length should increase to 3 and the new tail should be at the
     * previous tail position, with its previous interpolated position set to the
     * same value so it does not animate in from elsewhere. */
    assert(s->players[0].length == 3);
    assert(s->players[0].body[2].x == 1 && s->players[0].body[2].y == 2);
    assert(s->players[0].prev_segment_x != NULL && s->players[0].prev_segment_y != NULL);
    assert(s->players[0].prev_segment_x[2] == (float)s->players[0].body[2].x + 0.5f);
    assert(s->players[0].prev_segment_y[2] == (float)s->players[0].body[2].y + 0.5f);

    game_destroy(g);
    game_config_destroy(cfg);
    return 0;
}
