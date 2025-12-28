#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

typedef struct test_entry {
    const char* name;
    UnityTestFunction func;
    int isolated;
} test_entry;

static test_entry tests[] = {
    {"test_env", test_env, 0},

    {"test_input", test_input, 0},
    {"test_input_bindings", test_input_bindings, 0},
    {"test_input_multi", test_input_multi, 0},

    {"test_highscore_name", test_highscore_name, 0},

    {"test_texture_path", test_texture_path, 0},
    {"test_texture_path_extra", test_texture_path_extra, 0},

    {"test_net", test_net, 0},
    {"test_net_integration", test_net_integration, 0},
    {"test_net_overflow", test_net_overflow, 0},
    {"test_net_unpack", test_net_unpack, 0},

    {"test_collision", test_collision, 0},

    {"test_game_multi", test_game_multi, 0},
    /* This test overrides malloc/free; run it isolated in its own process */
    {"test_game_oom", test_game_oom, 1},

    {"test_persist", test_persist, 0},
    {"test_persist_config", test_persist_config, 0},
    {"test_persist_long_lines", test_persist_long_lines, 0},
    {"test_persist_truncation", test_persist_truncation, 0},

    {"test_stb_artifacts", test_stb_artifacts, 0},
    {"test_stb_chunk_size_limit", test_stb_chunk_size_limit, 0},
    {"test_stb_image_fuzz", test_stb_image_fuzz, 0},
    {"test_stb_leak_plte", test_stb_leak_plte, 0},

    {"test_tty_buffer_cap", test_tty_buffer_cap, 0},
    {"test_tty_open", test_tty_open, 0},
    {"test_tty_path", test_tty_path, 0},
};

static size_t tests_count(void) {
    return sizeof(tests) / sizeof(tests[0]);
}

static int run_single_by_name(const char* name) {
    for (size_t i = 0; i < tests_count(); ++i) {
        if (strcmp(tests[i].name, name) == 0) {
            run_single_test(name, tests[i].func, __FILE__, 0);
            return 0;
        }
    }
    fprintf(stderr, "Unknown test: %s\n", name);
    return 1;
}

int main(int argc, char** argv) {
    const char* single = getenv("UNITY_SINGLE_TEST");

    if (single) {
        UnityBegin();
        int rc = run_single_by_name(single);
        return UnityEnd() || rc;
    }

    UnityBegin();
    int isolated_fail = 0;

    for (size_t i = 0; i < tests_count(); ++i) {
        if (tests[i].isolated) {
            pid_t pid = fork();
            if (pid == 0) {
                /* child: run only this test */
                if (setenv("UNITY_SINGLE_TEST", tests[i].name, 1) != 0) {
                    perror("setenv");
                    _exit(127);
                }
                char* const newargv[] = { argv[0], NULL };
                execv(argv[0], newargv);
                perror("execv");
                _exit(127);
            } else if (pid > 0) {
                int status = 0;
                if (waitpid(pid, &status, 0) == -1) {
                    perror("waitpid");
                    isolated_fail++;
                } else if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0)) {
                    fprintf(stderr, "ISOLATED FAIL: %s (status=%d)\n", tests[i].name, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
                    isolated_fail++;
                }
            } else {
                perror("fork");
                isolated_fail++;
            }
        } else {
            run_single_test(tests[i].name, tests[i].func, __FILE__, 0);
        }
    }

    int rc = UnityEnd();
    return rc || (isolated_fail ? 1 : 0);
}
