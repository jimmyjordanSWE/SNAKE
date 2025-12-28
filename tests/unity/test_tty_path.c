#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include "tty.h"

TEST(test_tty_path) {
    size_t long_len = 1024;
    char *longpath = malloc(long_len + 1);
    TEST_ASSERT_TRUE(longpath != NULL);
    for (size_t i = 0; i < long_len; ++i) longpath[i] = 'a';
    longpath[long_len] = '\0';
    tty_context* ctx = tty_open(longpath, 10, 10);
    TEST_ASSERT_TRUE(ctx == NULL);
    free(longpath);

    tty_context* ctx2 = tty_open(NULL, 1, 1);
    if (ctx2) tty_close(ctx2);
}


