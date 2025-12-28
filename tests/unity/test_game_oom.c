#include "unity.h"
#include "oom_overrides.h"
#include "game.h"
#include "persist.h"

TEST(test_game_oom) {
    oom_reset(); oom_set_fail_after(0);
    Game* g = game_create(NULL, 0);
    TEST_ASSERT_TRUE(g == NULL);

    oom_reset(); oom_set_fail_after(-1);
    GameConfig* cfg = game_config_create();
    if (!cfg) TEST_FAIL_MESSAGE("SKIP: could not allocate GameConfig; environment issue");

    oom_set_fail_after(0);
    Game* g2 = game_create(cfg, 0);
    TEST_ASSERT_TRUE(g2 == NULL);

    oom_reset(); oom_set_fail_after(4);
    Game* g3 = game_create(cfg, 0);
    if (g3) game_destroy(g3);

    oom_reset(); oom_set_fail_after(0);
    HighScore* hs = highscore_create("x", 1);
    TEST_ASSERT_TRUE(hs == NULL);

    game_config_destroy(cfg);
    /* Restore normal allocation behavior for subsequent tests */
    oom_reset();
}

