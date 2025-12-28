#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "persist.h"

static int run_case(const char* fn) {
    HighScore** arr = NULL;
    int count = persist_read_scores(fn, &arr);
    if (arr) persist_free_scores(arr, count);
    return count;
}

TEST(test_persist_long_lines) {
    const char* fn = "test_long_lines.tmp";
    FILE* f = fopen(fn, "w");
    TEST_ASSERT_TRUE(f != NULL);

    size_t long_len = 3 * 256;
    char* longline = malloc(long_len + 32);
    TEST_ASSERT_TRUE(longline != NULL);
    strcpy(longline, "Alice 100 ");
    for (size_t i = strlen(longline); i < long_len - 2; ++i) longline[i] = 'X';
    longline[long_len - 2] = '\n';
    longline[long_len - 1] = '\0';
    fputs(longline, f);
    free(longline);
    fclose(f);

    int c = run_case(fn);
    TEST_ASSERT_TRUE(c >= 0);
    remove(fn);

    FILE* f2 = fopen(fn, "w");
    TEST_ASSERT_TRUE(f2 != NULL);
    for (int i = 0; i < PERSIST_MAX_SCORES + 5; ++i) fprintf(f2, "N%02d  %d\n", i, i);
    fclose(f2);
    int c2 = run_case(fn);
    TEST_ASSERT_TRUE(c2 <= PERSIST_MAX_SCORES);
    remove(fn);
}


