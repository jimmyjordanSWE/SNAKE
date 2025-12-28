#include "unity.h"
#include "persist.h"
#include "game.h"
#include <unistd.h>

TEST(test_persist_player_meta) {
    char template[] = "/tmp/snake_pmeta.XXXXXX";
    int fd = mkstemp(template);
    TEST_ASSERT_TRUE(fd >= 0);
    FILE* f = fdopen(fd, "w");
    TEST_ASSERT_TRUE(f != NULL);
    fputs("p1_name=Alice\n", f);
    fputs("p1_color=0x00ff00ff\n", f);
    fputs("p2_name=Bob\n", f);
    fputs("p2_color=0x0000ffff\n", f);
    fclose(f);

    GameConfig* cfg = NULL;
    int ok = persist_load_config(template, &cfg);
    unlink(template);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(cfg != NULL);

    TEST_ASSERT_EQUAL_STRING("Alice", game_config_get_player_name_for(cfg, 0));
    TEST_ASSERT_EQUAL_INT( (int)0x00ff00ffu, (int)game_config_get_player_color(cfg, 0));
    TEST_ASSERT_EQUAL_STRING("Bob", game_config_get_player_name_for(cfg, 1));
    TEST_ASSERT_EQUAL_INT( (int)0x0000ffffu, (int)game_config_get_player_color(cfg, 1));

    game_config_destroy(cfg);
}
