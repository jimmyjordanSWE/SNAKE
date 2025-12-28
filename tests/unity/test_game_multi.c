#include "unity.h"
#include <string.h>
#include "game.h"
#include "player.h"
#include "game_internal.h"

TEST(test_game_multi) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);
    game_config_set_max_players(cfg, 4);
    Game* g = game_create(cfg, 0);
    TEST_ASSERT_TRUE(g != NULL);
    int initial = game_get_num_players(g);

    PlayerCfg* pc0 = player_cfg_create();
    player_cfg_set_name(pc0, "P0");
    player_cfg_set_color(pc0, 0xFF0000FF);
    int idx0 = game_add_player(g, pc0);
    TEST_ASSERT_TRUE(idx0 >= 0);
    TEST_ASSERT_EQUAL_INT(initial + 1, game_get_num_players(g));

    PlayerCfg* pc1 = player_cfg_create();
    player_cfg_set_name(pc1, "P1");
    player_cfg_set_color(pc1, 0xFFFF0000);
    int idx1 = game_add_player(g, pc1);
    TEST_ASSERT_TRUE(idx1 >= 0);
    TEST_ASSERT_EQUAL_INT(initial + 2, game_get_num_players(g));

    const GameState* gs = game_get_state(g);
    TEST_ASSERT_TRUE(gs != NULL);
    TEST_ASSERT_EQUAL_STRING("P0", gs->players[idx0].name);
    TEST_ASSERT_EQUAL_INT(0xFF0000FF, gs->players[idx0].color);
    TEST_ASSERT_EQUAL_STRING("P1", gs->players[idx1].name);
    TEST_ASSERT_EQUAL_INT(0xFFFF0000, gs->players[idx1].color);

    player_cfg_destroy(pc0);
    player_cfg_destroy(pc1);
    game_destroy(g);
    game_config_destroy(cfg);
}


