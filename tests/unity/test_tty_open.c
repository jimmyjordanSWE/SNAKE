#include "unity.h"
#include <string.h>
#include "tty.h"

TEST(test_tty_open) {
    char longpath[512];
    memset(longpath, 'A', sizeof(longpath) - 1);
    longpath[sizeof(longpath) - 1] = '\0';
    tty_context* t = tty_open(longpath, 10, 5);
    TEST_ASSERT_TRUE(t == NULL);
}


