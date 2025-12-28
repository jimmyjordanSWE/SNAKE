#include "unity.h"
#include "oom_overrides.h"
#include "snake/tty.h"

TEST(test_tty_oom) {
    oom_reset(); oom_set_fail_after(0);
    tty_context* t = tty_open(NULL, 1, 1);
    TEST_ASSERT_TRUE(t == NULL);

    oom_reset(); oom_set_fail_after(-1);
    /* try with fail after a few allocations */
    oom_set_fail_after(2);
    tty_context* t2 = tty_open(NULL, 1, 1);
    if (t2) tty_close(t2); /* ok if allocated sometimes */
    oom_reset();
}
