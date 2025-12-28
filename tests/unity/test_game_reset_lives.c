#include "unity.h"
#include "game.h"
#include "core/game_internal.h"

void test_game_reset_restores_lives_and_clears_elimination(void) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);
    game_config_set_max_players(cfg, 4);
    game_config_set_num_players(cfg, 2);

    Game* g = game_create(cfg, 0);
    TEST_ASSERT_TRUE(g != NULL);
    GameState* gs = (GameState*)game_get_state(g);
    TEST_ASSERT_TRUE(gs != NULL);

    /* Simulate that player 0 was eliminated and player 1 lost one life */
    gs->players[0].lives = 0;
    gs->players[0].eliminated = true;
    gs->players[0].active = false;

    gs->players[1].lives = 2;
    gs->players[1].eliminated = false;

    /* Call reset */
    game_reset(g);

    /* After reset (multiplayer) both players should have 3 lives and not be eliminated */
    TEST_ASSERT_EQUAL_INT(3, gs->players[0].lives);
    TEST_ASSERT_FALSE(gs->players[0].eliminated);
    TEST_ASSERT_TRUE(gs->players[0].active);
    TEST_ASSERT_TRUE(gs->players[0].length > 0);

    TEST_ASSERT_EQUAL_INT(3, gs->players[1].lives);
    TEST_ASSERT_FALSE(gs->players[1].eliminated);
    TEST_ASSERT_TRUE(gs->players[1].active);
    TEST_ASSERT_TRUE(gs->players[1].length > 0);

    game_destroy(g);
    game_config_destroy(cfg);
}
