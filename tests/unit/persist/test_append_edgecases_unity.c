#include "snake/persist.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void test_append_reject_when_low(void);
static void test_append_replace_lowest_when_higher(void);

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_test_append_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

void test_append_reject_when_low(void) {
    char* fname = make_temp_file();
    FILE* f = fopen(fname, "w");
    /* write PERSIST_MAX_SCORES low entries */
    for(int i=0;i<PERSIST_MAX_SCORES;i++) fprintf(f, "p%d %d\n", i, i);
    fclose(f);
    /* append a lower score (0) should be rejected since not greater than lowest */
    bool ok = persist_append_score(fname, "loser", 0);
    TEST_ASSERT_TRUE_MSG(ok == false, "append of too-low score should be rejected");
    unlink(fname); free(fname);
}

void test_append_replace_lowest_when_higher(void) {
    char* fname = make_temp_file();
    FILE* f = fopen(fname, "w");
    /* write PERSIST_MAX_SCORES low entries */
    for(int i=0;i<PERSIST_MAX_SCORES;i++) fprintf(f, "p%d %d\n", i, i);
    fclose(f);
    /* append a high score that should replace lowest */
    bool ok = persist_append_score(fname, "winner", 100);
    TEST_ASSERT_TRUE_MSG(ok == true, "append of high score should succeed");
    HighScore** out = NULL;
    int cnt = persist_read_scores(fname, &out);
    TEST_ASSERT_TRUE_MSG(cnt >= 1, "read after append");
    bool found = false;
    for(int i=0;i<cnt;i++) {
        if(strcmp(highscore_get_name(out[i]), "winner") == 0 && highscore_get_score(out[i]) == 100) { found = true; break; }
    }
    TEST_ASSERT_TRUE_MSG(found, "winner entry should be present after append");
    persist_free_scores(out, cnt);
    unlink(fname); free(fname);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_append_reject_when_low);
    RUN_TEST(test_append_replace_lowest_when_higher);
    return UnityEnd();
}
