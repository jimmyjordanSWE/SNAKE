#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "persist.h"

TEST(test_persist_truncation) {
    const char* fn = "test_trunc.tmp";
    FILE* f = fopen(fn, "w");
    TEST_ASSERT_TRUE(f != NULL);

    size_t bufsize = 256;
    char *big = malloc(bufsize + 64);
    TEST_ASSERT_TRUE(big != NULL);
    for (size_t i = 0; i < bufsize + 10; ++i) big[i] = 'A';
    fwrite(big, 1, bufsize + 10, f);
    fprintf(f, "\nBob 10\n");
    free(big);
    fclose(f);

    HighScore** arr = NULL;
    int count = persist_read_scores(fn, &arr);
    TEST_ASSERT_EQUAL_INT(1, count);
    if (arr) persist_free_scores(arr, count);
    remove(fn);
}


