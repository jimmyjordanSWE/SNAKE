#include "snake/persist.h"
#include "unity.h"

static void test_game_config_setters_getters(void);

void test_game_config_setters_getters(void) {
    GameConfig* cfg = game_config_create();
    game_config_set_render_glyphs(cfg, 1);
    TEST_ASSERT_EQUAL_INT(1, game_config_get_render_glyphs(cfg));
    game_config_set_render_glyphs(cfg, 0);
    TEST_ASSERT_EQUAL_INT(0, game_config_get_render_glyphs(cfg));

    game_config_set_enable_external_3d_view(cfg, 0);
    TEST_ASSERT_EQUAL_INT(0, game_config_get_enable_external_3d_view(cfg));
    game_config_set_enable_external_3d_view(cfg, 1);
    TEST_ASSERT_EQUAL_INT(1, game_config_get_enable_external_3d_view(cfg));

    game_config_set_player_name(cfg, "xyz");
    TEST_ASSERT_EQUAL_STRING("xyz", game_config_get_player_name(cfg));

    game_config_set_key_up(cfg, 'u');
    TEST_ASSERT_EQUAL_INT('u', (int)game_config_get_key_up(cfg));

    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_game_config_setters_getters);
    return UnityEnd();
}
