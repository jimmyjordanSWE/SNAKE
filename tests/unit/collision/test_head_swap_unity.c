#include "snake/game.h"
#include "snake/game_internal.h"
#include "snake/collision.h"
#include "unity.h"

static void test_head_swap_resets_both_players(void);

void test_head_swap_resets_both_players(void) {
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
    TEST_ASSERT_TRUE_MSG(g != NULL, "game_create should succeed");
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

    TEST_ASSERT_TRUE_MSG(a->needs_reset && b->needs_reset, "both players should be reset on head-swap");
    game_destroy(g);
    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_head_swap_resets_both_players);
    return UnityEnd();
}
