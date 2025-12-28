#include "unity.h"
#include "core/game_internal.h"
#include "game.h"

void test_default_player_colors_differ(void) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);
    game_config_set_max_players(cfg, 4);
    game_config_set_num_players(cfg, 2);
    Game* g = game_create(cfg, 0);
    TEST_ASSERT_TRUE(g != NULL);
    const GameState* gs = game_get_state(g);
    TEST_ASSERT_TRUE(gs != NULL);

    uint32_t c0 = gs->players[0].color;
    uint32_t c1 = gs->players[1].color;

    /* Colors must be non-zero and differ for first two players. */
    TEST_ASSERT_TRUE(c0 != 0);
    TEST_ASSERT_TRUE(c1 != 0);
    TEST_ASSERT_TRUE(c0 != c1);

    game_destroy(g);
    game_config_destroy(cfg);
}
