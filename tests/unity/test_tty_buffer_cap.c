#include "unity.h"
#include "tty.h"

TEST(test_tty_buffer_cap) {
    tty_context* ctx = tty_open(NULL, 1, 1);
    if (ctx) {
        size_t cap = tty_get_write_buffer_size(ctx);
        TEST_ASSERT_TRUE(cap > 0 && cap <= 10 * 1024 * 1024);
        tty_close(ctx);
    }
}


