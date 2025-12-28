#include "unity.h"
#include "input.h"
#include "persist.h"

TEST(test_input_bindings) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);

    game_config_set_key_quit(cfg, '\x1b');
    game_config_set_key_restart(cfg, 'x');
    game_config_set_key_pause(cfg, 'p');

    game_config_set_player_key_left(cfg, 0, 'a');
    game_config_set_player_key_right(cfg, 0, 'd');

    game_config_set_player_key_left(cfg, 1, 'j');
    game_config_set_player_key_right(cfg, 1, 'l');

    input_set_bindings_from_config(cfg);

    InputState outs[2];
    unsigned char buf1[] = {'a', 'j'};
    input_poll_all_from_buf(outs, 2, buf1, sizeof(buf1));
    TEST_ASSERT_TRUE(outs[0].turn_left);
    /* Due to flipped bindings for non-primary players, player 1's 'j' maps to
       a right turn. */
    TEST_ASSERT_TRUE(outs[1].turn_right);

    unsigned char buf2[] = {'\x1b'};
    input_poll_all_from_buf(outs, 2, buf2, sizeof(buf2));
    TEST_ASSERT_TRUE(outs[0].quit && outs[1].quit);

    unsigned char arrow_left[] = {'\x1b', '[', 'D'};
    input_poll_all_from_buf(outs, 2, arrow_left, sizeof(arrow_left));
    TEST_ASSERT_TRUE(outs[0].turn_left && !outs[1].turn_left);

    game_config_destroy(cfg);
}


