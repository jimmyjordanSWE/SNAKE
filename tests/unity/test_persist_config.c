#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "persist.h"
#include "game.h"

static int run_case(const char* content, int expect_ok) {
    const char* fn = "test_config.tmp";
    FILE* f = fopen(fn, "w");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    GameConfig* cfg = NULL;
    int ok = persist_load_config(fn, &cfg);
    remove(fn);
    if (cfg) game_config_destroy(cfg);
    return (ok == expect_ok) ? 1 : 0;
}

TEST(test_persist_config) {
    const char* good = "player_name=Alice\nboard_width=30\nboard_height=20\nwall_texture=/tmp/wall.png\nfloor_texture=/tmp/floor.png\n";
    TEST_ASSERT_EQUAL_INT(1, run_case(good, 1));

    char longname[1024];
    for (int i = 0; i < (int)sizeof(longname)-2; ++i) longname[i] = 'X';
    longname[sizeof(longname)-2] = '\n';
    longname[sizeof(longname)-1] = '\0';
    TEST_ASSERT_EQUAL_INT(1, run_case(longname, 1));

    const char* badnum = "board_width=notanumber\n";
    TEST_ASSERT_EQUAL_INT(1, run_case(badnum, 1));

    const char* outofrange = "board_width=100000\n";
    TEST_ASSERT_EQUAL_INT(1, run_case(outofrange, 1));

    GameConfig* cfg = NULL;
    int ok = persist_load_config("/path/that/does/not/exist.conf", &cfg);
    TEST_ASSERT_TRUE(cfg != NULL);
    game_config_destroy(cfg);
    (void)ok;
}


