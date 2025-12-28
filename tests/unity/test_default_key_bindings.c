#include "unity.h"
#include "persist.h"
#include "game.h"

TEST(test_default_key_bindings) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);

    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_UP, game_config_get_key_up(cfg));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_DOWN, game_config_get_key_down(cfg));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_LEFT, game_config_get_key_left(cfg));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_RIGHT, game_config_get_key_right(cfg));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_QUIT, game_config_get_key_quit(cfg));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_RESTART, game_config_get_key_restart(cfg));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_PAUSE, game_config_get_key_pause(cfg));

#ifdef PERSIST_CONFIG_DEFAULT_KEY_UP_2
    if (SNAKE_MAX_PLAYERS >= 2) TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_UP_2, game_config_get_player_key_up(cfg, 1));
#endif
#ifdef PERSIST_CONFIG_DEFAULT_KEY_UP_3
    if (SNAKE_MAX_PLAYERS >= 3) TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_UP_3, game_config_get_player_key_up(cfg, 2));
#endif
#ifdef PERSIST_CONFIG_DEFAULT_KEY_UP_4
    if (SNAKE_MAX_PLAYERS >= 4) TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_UP_4, game_config_get_player_key_up(cfg, 3));
#endif

    game_config_destroy(cfg);
}


