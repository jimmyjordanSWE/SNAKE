#include "snake/persist.h"
#include "unity.h"

static void test_game_config_defaults(void) {
    GameConfig* cfg = game_config_create();
    int w = 0, h = 0;
    game_config_get_board_size(cfg, &w, &h);
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_WIDTH, w);
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_HEIGHT, h);
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_TICK_MS, game_config_get_tick_rate_ms(cfg));
    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_game_config_defaults);
    return UnityEnd();
}
