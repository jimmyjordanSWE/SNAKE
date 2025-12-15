#include "snake/game.h"
#include "snake/game_internal.h"
#include "unity.h"

static void test_eating_food_increments_score_and_respawns(void);

void test_eating_food_increments_score_and_respawns(void) {
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
    TEST_ASSERT_TRUE_MSG(g != NULL, "game_create should succeed");

    GameState* s = game_test_get_state(g);
    s->players[0].active = true;
    s->players[0].length = 2;
    s->players[0].body[0] = (SnakePoint){2, 2};
    s->players[0].body[1] = (SnakePoint){1, 2};
    s->players[0].current_dir = SNAKE_DIR_RIGHT;
    game_test_set_food(g, (const SnakePoint[]){(SnakePoint){3, 2}}, 1);

    GameEvents ev = {0};
    game_step(g, &ev);

    TEST_ASSERT_EQUAL_INT(1, s->players[0].score);
    TEST_ASSERT_TRUE_MSG(ev.food_respawned == true, "food should be respawned");

    game_destroy(g);
    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_eating_food_increments_score_and_respawns);
    return UnityEnd();
}
