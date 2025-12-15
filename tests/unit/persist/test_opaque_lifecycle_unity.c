#include "snake/persist.h"
#include "unity.h"

static void test_highscore_lifecycle(void);
static void test_gameconfig_lifecycle(void);

void test_highscore_lifecycle(void) {
    HighScore* h = highscore_create("life", 1);
    TEST_ASSERT_TRUE_MSG(h != NULL, "highscore_create returns non-null");
    highscore_set_score(h, 99);
    TEST_ASSERT_EQUAL_INT(99, highscore_get_score(h));
    highscore_set_name(h, "changed");
    TEST_ASSERT_EQUAL_STRING("changed", highscore_get_name(h));
    highscore_destroy(h);
}

void test_gameconfig_lifecycle(void) {
    GameConfig* g = game_config_create();
    TEST_ASSERT_TRUE_MSG(g != NULL, "game_config_create returns non-null");
    game_config_set_board_size(g, 40, 20);
    int w,h; game_config_get_board_size(g, &w, &h);
    TEST_ASSERT_EQUAL_INT(40, w);
    TEST_ASSERT_EQUAL_INT(20, h);
    game_config_destroy(g);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_highscore_lifecycle);
    RUN_TEST(test_gameconfig_lifecycle);
    return UnityEnd();
}
