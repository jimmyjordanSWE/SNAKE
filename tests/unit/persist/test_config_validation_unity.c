#include "snake/persist.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_test_cfg_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

static void test_clamping_min_max(void) {
    char* f1 = make_temp_file();
    FILE* f = fopen(f1, "w");
    fprintf(f, "board_width=5\n");
    fprintf(f, "board_height=5\n");
    fprintf(f, "tick_rate_ms=1\n");
    fprintf(f, "screen_width=1\n");
    fprintf(f, "screen_height=1\n");
    fclose(f);
    GameConfig* cfg = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(f1, &cfg) == true, "load should return true when file exists");
    int bw, bh, sw, sh;
    game_config_get_board_size(cfg, &bw, &bh);
    game_config_get_screen_size(cfg, &sw, &sh);
    
    TEST_ASSERT_TRUE_MSG(bw >= 20, "board_width should be clamped to >=20");
    TEST_ASSERT_TRUE_MSG(bh >= 10, "board_height should be clamped to >=10");
    TEST_ASSERT_TRUE_MSG(game_config_get_tick_rate_ms(cfg) >= 10, "tick_rate_ms should be clamped to >=10");
    TEST_ASSERT_TRUE_MSG(sw >= 20, "screen_width should be clamped to >=20");
    TEST_ASSERT_TRUE_MSG(sh >= 10, "screen_height should be clamped to >=10");
    game_config_destroy(cfg);

    
    char* f2 = make_temp_file();
    f = fopen(f2, "w");
    fprintf(f, "board_width=1000000\n");
    fprintf(f, "board_height=999999\n");
    fclose(f);
    cfg = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(f2, &cfg) == true, "load should return true when file exists");
    game_config_get_board_size(cfg, &bw, &bh);
    
    TEST_ASSERT_TRUE_MSG(bw <= 100, "board_width should be clamped to <=100");
    TEST_ASSERT_TRUE_MSG(bh <= 100, "board_height should be clamped to <=100");
    game_config_destroy(cfg);

    unlink(f1); free(f1);
    unlink(f2); free(f2);
}

static void test_boolean_and_glyphs_parsing(void) {
    char* f3 = make_temp_file();
    FILE* f = fopen(f3, "w");
    fprintf(f, "enable_external_3d_view=YeS\n");
    fprintf(f, "show_sprite_debug=0\n");
    fprintf(f, "render_glyphs=ascii\n");
    fclose(f);
    GameConfig* cfg = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(f3, &cfg) == true, "boolean parsing should accept variants");
    TEST_ASSERT_EQUAL_INT(1, game_config_get_enable_external_3d_view(cfg));
    TEST_ASSERT_EQUAL_INT(0, game_config_get_show_sprite_debug(cfg));
    TEST_ASSERT_EQUAL_INT(1, game_config_get_render_glyphs(cfg)); 
    game_config_destroy(cfg);
    unlink(f3); free(f3);
}

static void test_unknown_keys_and_missing_file(void) {
    char* f4 = make_temp_file();
    FILE* f = fopen(f4, "w");
    fprintf(f, "unused_key=foobar\n");
    fprintf(f, "player_name=tester123\n");
    fclose(f);
    GameConfig* cfg = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(f4, &cfg) == true, "unknown keys should be ignored");
    TEST_ASSERT_EQUAL_STRING("tester123", game_config_get_player_name(cfg));
    game_config_destroy(cfg);

    GameConfig* defcfg = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(NULL, &defcfg) == false, "passing NULL filename should return false");
    int bw, bh;
    game_config_get_board_size(defcfg, &bw, &bh);
    TEST_ASSERT_TRUE_MSG(bw >= 20 && bh >= 10, "defaults should be sane");
    game_config_destroy(defcfg);

    unlink(f4); free(f4);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_clamping_min_max);
    RUN_TEST(test_boolean_and_glyphs_parsing);
    RUN_TEST(test_unknown_keys_and_missing_file);
    return UnityEnd();
}
