#include "unity.h"
#include "persist.h"
#include "game.h"
#include <unistd.h>

TEST(test_persist_player_keys_pstyle) {
    char template[] = "/tmp/snake_pkeys.XXXXXX";
    int fd = mkstemp(template);
    TEST_ASSERT_TRUE(fd >= 0);
    FILE* f = fdopen(fd, "w");
    TEST_ASSERT_TRUE(f != NULL);
    /* Write p-style player keys */
    fputs("p1_left=w\n", f);
    fputs("p1_right=q\n", f);
    fputs("p2_left=t\n", f);
    fputs("p2_right=y\n", f);
    fclose(f);

    GameConfig* cfg = NULL;
    int ok = persist_load_config(template, &cfg);
    unlink(template);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(cfg != NULL);
    TEST_ASSERT_EQUAL_INT('w', game_config_get_player_key_left(cfg, 0));
    TEST_ASSERT_EQUAL_INT('q', game_config_get_player_key_right(cfg, 0));
    TEST_ASSERT_EQUAL_INT('t', game_config_get_player_key_left(cfg, 1));
    TEST_ASSERT_EQUAL_INT('y', game_config_get_player_key_right(cfg, 1));


    game_config_destroy(cfg);
}
