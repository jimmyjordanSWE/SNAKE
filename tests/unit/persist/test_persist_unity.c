#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "snake/persist.h"
#include "unity.h"

/* test prototypes */
static void test_write_and_read_scores(void);
static void test_append_score_and_limit(void);
static void test_config_io_and_parsing(void);
static void test_config_clamping_and_booleans(void);
static void test_unknown_keys_ignored(void);
static void test_missing_file_defaults(void);

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_test_unity_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

void test_write_and_read_scores(void) {
    char* fname = make_temp_file();
    HighScore* scores[3] = { highscore_create("alice", 10), highscore_create("bob", 5), highscore_create("carol", 7) };
    TEST_ASSERT_TRUE_MSG(persist_write_scores(fname, scores, 3), "persist_write_scores should succeed");
    for(int i=0;i<3;i++) highscore_destroy(scores[i]);
    HighScore** out = NULL;
    int cnt = persist_read_scores(fname, &out);
    TEST_ASSERT_EQUAL_INT(3, cnt);
    TEST_ASSERT_EQUAL_STRING("alice", highscore_get_name(out[0]));
    persist_free_scores(out, cnt);
    unlink(fname); free(fname);
}

void test_append_score_and_limit(void) {
    char* fname = make_temp_file();
    HighScore* scores[5];
    for(int i=0;i<5;i++) scores[i] = highscore_create("x", i);
    TEST_ASSERT_TRUE_MSG(persist_write_scores(fname, scores, 5), "write 5 scores");
    for(int i=0;i<5;i++) highscore_destroy(scores[i]);
    /* append high score that should be inserted */
    TEST_ASSERT_TRUE_MSG(persist_append_score(fname, "winner", 100), "append should succeed and insert");
    HighScore** out = NULL;
    int cnt = persist_read_scores(fname, &out);
    TEST_ASSERT_TRUE_MSG(cnt >= 1, "read after append");
    persist_free_scores(out, cnt);
    unlink(fname); free(fname);
}

void test_config_io_and_parsing(void) {
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 30, 15);
    game_config_set_tick_rate_ms(cfg, 50);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_seed(cfg, 12345);
    game_config_set_fov_degrees(cfg, 70.5f);
    game_config_set_show_sprite_debug(cfg, 1);
    game_config_set_num_players(cfg, 2);
    game_config_set_max_players(cfg, 4);
    game_config_set_max_length(cfg, 128);
    game_config_set_max_food(cfg, 4);
    game_config_set_player_name(cfg, "tester");
    game_config_set_wall_height_scale(cfg, 1.25f);
    game_config_set_wall_texture(cfg, "assets/wall_custom.png");
    game_config_set_floor_texture(cfg, "assets/floor_custom.png");
    game_config_set_key_up(cfg, 'i');
    game_config_set_key_down(cfg, 'k');
    game_config_set_key_left(cfg, 'j');
    game_config_set_key_right(cfg, 'l');
    game_config_set_key_quit(cfg, 'x');
    game_config_set_key_restart(cfg, 'n');
    game_config_set_key_pause(cfg, 'm');

    char* cfgfile = make_temp_file();
    TEST_ASSERT_TRUE_MSG(persist_write_config(cfgfile, cfg), "persist_write_config should succeed");
    GameConfig* loaded = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(cfgfile, &loaded), "loaded config should return true (file exists)");
    int bw, bh; game_config_get_board_size(loaded, &bw, &bh);
    TEST_ASSERT_EQUAL_INT(30, bw);
    TEST_ASSERT_EQUAL_INT(15, bh);
    TEST_ASSERT_EQUAL_INT(12345, (int)game_config_get_seed(loaded));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 70.5f, game_config_get_fov_degrees(loaded));
    TEST_ASSERT_EQUAL_INT(1, game_config_get_show_sprite_debug(loaded));
    TEST_ASSERT_EQUAL_INT(2, game_config_get_num_players(loaded));
    TEST_ASSERT_EQUAL_STRING("tester", game_config_get_player_name(loaded));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.25f, game_config_get_wall_height_scale(loaded));
    TEST_ASSERT_EQUAL_STRING("assets/wall_custom.png", game_config_get_wall_texture(loaded));
    TEST_ASSERT_EQUAL_STRING("assets/floor_custom.png", game_config_get_floor_texture(loaded));
    TEST_ASSERT_EQUAL_INT('i', (int)game_config_get_key_up(loaded));

    game_config_destroy(cfg);
    game_config_destroy(loaded);
    unlink(cfgfile); free(cfgfile);
}

void test_config_clamping_and_booleans(void) {
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
    TEST_ASSERT_TRUE_MSG(bw >= 20 && bh >= 10, "board sizes clamped to minimums");
    TEST_ASSERT_TRUE_MSG(game_config_get_tick_rate_ms(cfg) >= 10, "tick rate clamped to minimum");
    TEST_ASSERT_TRUE_MSG(sw >= 20 && sh >= 10, "screen sizes clamped");
    game_config_destroy(cfg);
    unlink(f1); free(f1);

    char* f2 = make_temp_file();
    f = fopen(f2, "w");
    fprintf(f, "enable_external_3d_view=YeS\n");
    fprintf(f, "show_sprite_debug=0\n");
    fprintf(f, "render_glyphs=ascii\n");
    fclose(f);
    cfg = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(f2, &cfg) == true, "boolean parsing should accept variants");
    TEST_ASSERT_EQUAL_INT(1, game_config_get_enable_external_3d_view(cfg));
    TEST_ASSERT_EQUAL_INT(0, game_config_get_show_sprite_debug(cfg));
    TEST_ASSERT_EQUAL_INT(1, game_config_get_render_glyphs(cfg));
    game_config_destroy(cfg);
    unlink(f2); free(f2);
}

void test_unknown_keys_ignored(void) {
    char* f4 = make_temp_file();
    FILE* f = fopen(f4, "w");
    fprintf(f, "unused_key=foobar\n");
    fprintf(f, "player_name=tester123\n");
    fclose(f);
    GameConfig* cfg = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(f4, &cfg) == true, "unknown keys should be ignored");
    TEST_ASSERT_EQUAL_STRING("tester123", game_config_get_player_name(cfg));
    game_config_destroy(cfg);
    unlink(f4); free(f4);
}

void test_missing_file_defaults(void) {
    GameConfig* defcfg = NULL;
    TEST_ASSERT_TRUE_MSG(persist_load_config(NULL, &defcfg) == false, "passing NULL filename should return false and provide defaults");
    int bw, bh; game_config_get_board_size(defcfg, &bw, &bh);
    TEST_ASSERT_TRUE_MSG(bw >= 20 && bh >= 10, "defaults should be sane");
    game_config_destroy(defcfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_write_and_read_scores);
    RUN_TEST(test_append_score_and_limit);
    RUN_TEST(test_config_io_and_parsing);
    RUN_TEST(test_config_clamping_and_booleans);
    RUN_TEST(test_unknown_keys_ignored);
    RUN_TEST(test_missing_file_defaults);
    return UnityEnd();
}
