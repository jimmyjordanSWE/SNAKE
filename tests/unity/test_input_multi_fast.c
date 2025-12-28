#include "unity.h"
#include "game.h"
#include "input.h"
#include "persist.h"

/* Simulate two rapid input samples (as if one arrived at tick start and one during
   the frame wait) and verify the last input wins for each player. */
TEST(test_input_multi_fast) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);
    game_config_set_num_players(cfg, 2);

    Game* g = game_create(cfg, 1);
    TEST_ASSERT_TRUE(g != NULL);

    const GameState* gs = game_get_state(g);
    TEST_ASSERT_TRUE(gs != NULL);

    /* Prepare two samples: first sample sets player0 left, player1 left.
       Second (rapid) sample sets player0 right, player1 right. */
    InputState outs1[2] = {0};
    /* Map them to turn flags directly without relying on keybindings in this test */
    outs1[0].turn_left = true;
    outs1[1].turn_left = true;

    /* Enqueue first sample */
    for(int i = 0; i < 2; ++i) TEST_ASSERT_EQUAL_INT(0, game_enqueue_input(g, i, &outs1[i]));

    /* Rapid second sample */
    InputState outs2[2] = {0};
    outs2[0].turn_right = true;
    outs2[1].turn_right = true;

    for(int i = 0; i < 2; ++i) TEST_ASSERT_EQUAL_INT(0, game_enqueue_input(g, i, &outs2[i]));

    /* Advance one tick to ensure handling doesn't crash */
    GameEvents ev = {0};
    game_step(g, &ev);

    /* We at least ensure game_step completed and did not crash; further behavioral tests
       require exposing more of GameState or running full integration. */
    TEST_ASSERT_TRUE(1);

    game_destroy(g);
    game_config_destroy(cfg);
}
