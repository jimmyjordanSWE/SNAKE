#include "unity.h"
#include "input.h"

TEST(test_input_multi) {
    InputState outs[4];
    input_set_player_key_bindings(0, 'w', 's', 'a', 'd', 0, 0, 0);
    input_set_player_key_bindings(1, 'i', 'k', 'j', 'l', 0, 0, 0);

    unsigned char buf1[] = {'a', 'j'};
    input_poll_all_from_buf(outs, 2, buf1, sizeof(buf1));
    TEST_ASSERT_TRUE(outs[0].turn_left);
    TEST_ASSERT_TRUE(outs[1].turn_left);

    unsigned char arrow_up[] = {'\x1b', '[', 'A'};
    input_poll_all_from_buf(outs, 2, arrow_up, sizeof(arrow_up));
    TEST_ASSERT_TRUE(outs[0].move_up && outs[1].move_up);
}


