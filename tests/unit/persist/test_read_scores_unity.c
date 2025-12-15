#include "snake/persist.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void test_read_scores_malformed_lines(void);

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_test_rs_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

void test_read_scores_malformed_lines(void) {
    char* fname = make_temp_file();
    FILE* f = fopen(fname, "w");
    /* valid */ fprintf(f, "alice 10\n");
    /* missing score */ fprintf(f, "bob\n");
    /* non-numeric */ fprintf(f, "carol twelve\n");
    /* negative */ fprintf(f, "dave -5\n");
    /* too long name */ fprintf(f, "this_name_is_way_too_long_for_the_limit_and_should_be_ignored 3\n");
    /* valid */ fprintf(f, "eve 7\n");
    fclose(f);

    HighScore** out = NULL;
    int cnt = persist_read_scores(fname, &out);
    /* only alice and eve should be accepted */
    TEST_ASSERT_EQUAL_INT(2, cnt);
    TEST_ASSERT_EQUAL_STRING("alice", highscore_get_name(out[0]));
    TEST_ASSERT_EQUAL_INT(10, highscore_get_score(out[0]));
    TEST_ASSERT_EQUAL_STRING("eve", highscore_get_name(out[1]));
    TEST_ASSERT_EQUAL_INT(7, highscore_get_score(out[1]));
    persist_free_scores(out, cnt);
    unlink(fname); free(fname);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_read_scores_malformed_lines);
    return UnityEnd();
}
