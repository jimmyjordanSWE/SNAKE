#include "snake/persist.h"
#include "unity.h"

static void test_game_config_defaults(void) {
    GameConfig* cfg = game_config_create();
    int w = 0, h = 0;
    game_config_get_board_size(cfg, &w, &h);
    TEST_ASSERT_EQUAL_INT(40, w);
    TEST_ASSERT_EQUAL_INT(20, h);
    TEST_ASSERT_EQUAL_INT(100, game_config_get_tick_rate_ms(cfg));
    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_game_config_defaults);
    return UnityEnd();
}
