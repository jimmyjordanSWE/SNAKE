#include "unity.h"
#include "persist.h"
#include "game.h"

TEST(test_default_key_bindings) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);

    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_QUIT, game_config_get_key_quit(cfg));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_RESTART, game_config_get_key_restart(cfg));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_PAUSE, game_config_get_key_pause(cfg));

    /* Player 0 defaults */
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_LEFT, game_config_get_player_key_left(cfg, 0));
    TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_RIGHT, game_config_get_player_key_right(cfg, 0));

#ifdef PERSIST_CONFIG_DEFAULT_KEY_LEFT_2
    if (SNAKE_MAX_PLAYERS >= 2) TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_LEFT_2, game_config_get_player_key_left(cfg, 1));
#endif
#ifdef PERSIST_CONFIG_DEFAULT_KEY_LEFT_3
    if (SNAKE_MAX_PLAYERS >= 3) TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_LEFT_3, game_config_get_player_key_left(cfg, 2));
#endif
#ifdef PERSIST_CONFIG_DEFAULT_KEY_LEFT_4
    if (SNAKE_MAX_PLAYERS >= 4) TEST_ASSERT_EQUAL_INT(PERSIST_CONFIG_DEFAULT_KEY_LEFT_4, game_config_get_player_key_left(cfg, 3));
#endif

    game_config_destroy(cfg);
}


