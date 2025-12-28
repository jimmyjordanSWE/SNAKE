#include "unity.h"
#include <string.h>
#include "input.h"
#include "persist.h"

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
    unsigned char leftseq[] = {0x1b, '[', 'D'};
    input_poll_from_buf(&out3, leftseq, sizeof(leftseq));
    TEST_ASSERT_TRUE(out3.turn_left == true);

    /* ESC by itself should indicate quit when bound */
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);
    game_config_set_key_quit(cfg, '\x1b');
    input_set_bindings_from_config(cfg);

    InputState out4 = {0};
    unsigned char esc[] = {0x1b};
    input_poll_from_buf(&out4, esc, sizeof(esc));
    TEST_ASSERT_TRUE(out4.quit == true);

    game_config_destroy(cfg);
}


