#include "unity.h"
#include <string.h>
#include "input.h"

TEST(test_input) {
    InputState out = {0};
    unsigned char buf[512];
    for (size_t i = 0; i < sizeof(buf); ++i) buf[i] = (unsigned char)('a' + (int)(i % 26));
    input_poll_from_buf(&out, buf, sizeof(buf));
    TEST_ASSERT_TRUE(out.any_key == true);

    InputState out2 = {0};
    unsigned char esc1[] = {0x1b, '[', 'Z'};
    input_poll_from_buf(&out2, esc1, sizeof(esc1));
    TEST_ASSERT_FALSE(out2.move_up);
    TEST_ASSERT_FALSE(out2.move_down);
    TEST_ASSERT_FALSE(out2.move_left);
    TEST_ASSERT_FALSE(out2.move_right);

    InputState out3 = {0};
    unsigned char upseq[] = {0x1b, '[', 'A'};
    input_poll_from_buf(&out3, upseq, sizeof(upseq));
    TEST_ASSERT_TRUE(out3.move_up == true);
}


