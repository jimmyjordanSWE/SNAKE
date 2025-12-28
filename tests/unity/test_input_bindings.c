#include "unity.h"
#include "input.h"
#include "persist.h"

TEST(test_input_bindings) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);

    game_config_set_key_up(cfg, 'u');
    game_config_set_key_down(cfg, 'd');
    game_config_set_key_left(cfg, 'l');
    game_config_set_key_right(cfg, 'r');
    game_config_set_key_quit(cfg, 'q');
    game_config_set_key_restart(cfg, 'x');
    game_config_set_key_pause(cfg, 'p');

    game_config_set_player_key_up(cfg, 0, 'w');
    game_config_set_player_key_down(cfg, 0, 's');
    game_config_set_player_key_left(cfg, 0, 'a');
    game_config_set_player_key_right(cfg, 0, 'd');

    game_config_set_player_key_up(cfg, 1, 'i');
    game_config_set_player_key_down(cfg, 1, 'k');
    game_config_set_player_key_left(cfg, 1, 'j');
    game_config_set_player_key_right(cfg, 1, 'l');

    input_set_bindings_from_config(cfg);

    InputState outs[2];
    unsigned char buf1[] = {'a', 'j'};
    input_poll_all_from_buf(outs, 2, buf1, sizeof(buf1));
    TEST_ASSERT_TRUE(outs[0].turn_left);
    TEST_ASSERT_TRUE(outs[1].turn_left);

    unsigned char buf2[] = {'q'};
    input_poll_all_from_buf(outs, 2, buf2, sizeof(buf2));
    TEST_ASSERT_TRUE(outs[0].quit && outs[1].quit);

    unsigned char arrow_up[] = {'\x1b', '[', 'A'};
    input_poll_all_from_buf(outs, 2, arrow_up, sizeof(arrow_up));
    TEST_ASSERT_TRUE(outs[0].move_up && outs[1].move_up);

    game_config_destroy(cfg);
}


