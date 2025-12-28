#include "unity.h"
#include "input.h"

TEST(test_input_multi) {
    InputState outs[4];
    input_set_player_key_bindings(0, 'a', 'd');
    input_set_player_key_bindings(1, 'j', 'l');

    unsigned char buf1[] = {'a', 'j'};
    input_poll_all_from_buf(outs, 2, buf1, sizeof(buf1));
    TEST_ASSERT_TRUE(outs[0].turn_left);
    /* Flipped for player 1 */
    TEST_ASSERT_TRUE(outs[1].turn_right);

    unsigned char arrow_left[] = {'\x1b', '[', 'D'};
    input_poll_all_from_buf(outs, 2, arrow_left, sizeof(arrow_left));
    TEST_ASSERT_TRUE(outs[0].turn_left && !outs[1].turn_left);
}


