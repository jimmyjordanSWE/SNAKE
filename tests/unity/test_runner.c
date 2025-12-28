#include "unity.h"

/* Forward declarations of test functions (one per legacy test file) */
/* tests implement these functions with names: test_<file> */

/* utils/env */
void test_env(void);

/* input */
void test_input(void);
void test_input_bindings(void);
void test_input_multi(void);

/* highscore */
void test_highscore_name(void);

/* texture */
void test_texture_path(void);
void test_texture_path_extra(void);

/* net */
void test_net(void);
void test_net_integration(void);
void test_net_overflow(void);
void test_net_unpack(void);

/* collision */
void test_collision(void);

/* game */
void test_game_multi(void);
void test_game_oom(void);

/* persist */
void test_persist(void);
void test_persist_config(void);
void test_persist_long_lines(void);
void test_persist_truncation(void);

/* stb / vendor */
void test_stb_artifacts(void);
void test_stb_chunk_size_limit(void);
void test_stb_image_fuzz(void);
void test_stb_leak_plte(void);

/* tty */
void test_tty_buffer_cap(void);
void test_tty_open(void);
void test_tty_path(void);

int main(void) {
    UnityBegin();

    /* Register tests */
    RUN_TEST(test_env);

    RUN_TEST(test_input);
    RUN_TEST(test_input_bindings);
    RUN_TEST(test_input_multi);

    RUN_TEST(test_highscore_name);

    RUN_TEST(test_texture_path);
    RUN_TEST(test_texture_path_extra);

    RUN_TEST(test_net);
    RUN_TEST(test_net_integration);
    RUN_TEST(test_net_overflow);
    RUN_TEST(test_net_unpack);

    RUN_TEST(test_collision);

    RUN_TEST(test_game_multi);
    RUN_TEST(test_game_oom);

    RUN_TEST(test_persist);
    RUN_TEST(test_persist_config);
    RUN_TEST(test_persist_long_lines);
    RUN_TEST(test_persist_truncation);

    RUN_TEST(test_stb_artifacts);
    RUN_TEST(test_stb_chunk_size_limit);
    RUN_TEST(test_stb_image_fuzz);
    RUN_TEST(test_stb_leak_plte);

    RUN_TEST(test_tty_buffer_cap);
    RUN_TEST(test_tty_open);
    RUN_TEST(test_tty_path);

    return UnityEnd();
}
