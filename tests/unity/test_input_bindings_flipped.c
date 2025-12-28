#include "unity.h"
#include "input.h"
#include "persist.h"
#include "game.h"

TEST(test_input_bindings_flipped) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);

    /* Explicitly set p2/p3/p4 keys to known values */
    if (SNAKE_MAX_PLAYERS >= 2) {
        game_config_set_player_key_left(cfg, 1, 'w');
        game_config_set_player_key_right(cfg, 1, 'q');
    }
    if (SNAKE_MAX_PLAYERS >= 3) {
        game_config_set_player_key_left(cfg, 2, 't');
        game_config_set_player_key_right(cfg, 2, 'y');
    }
    if (SNAKE_MAX_PLAYERS >= 4) {
        game_config_set_player_key_left(cfg, 3, 'o');
        game_config_set_player_key_right(cfg, 3, 'p');
    }

    input_set_bindings_from_config(cfg);

    InputState outs[4] = {0};

    /* For p2: since we flip, 'w' should map to turn_right and 'q' to turn_left */
    if (SNAKE_MAX_PLAYERS >= 2) {
        unsigned char buf_w[] = {'w'};
        input_poll_all_from_buf(outs, 2, buf_w, sizeof(buf_w));
        TEST_ASSERT_TRUE(outs[1].turn_right);

        unsigned char buf_q[] = {'q'};
        input_poll_all_from_buf(outs, 2, buf_q, sizeof(buf_q));
        TEST_ASSERT_TRUE(outs[1].turn_left);
    }

    /* p3/p4 also flipped similarly */
    if (SNAKE_MAX_PLAYERS >= 3) {
        unsigned char buf_t[] = {'t'};
        input_poll_all_from_buf(outs, 3, buf_t, sizeof(buf_t));
        TEST_ASSERT_TRUE(outs[2].turn_right);

        unsigned char buf_y[] = {'y'};
        input_poll_all_from_buf(outs, 3, buf_y, sizeof(buf_y));
        TEST_ASSERT_TRUE(outs[2].turn_left);
    }

    if (SNAKE_MAX_PLAYERS >= 4) {
        unsigned char buf_o[] = {'o'};
        input_poll_all_from_buf(outs, 4, buf_o, sizeof(buf_o));
        TEST_ASSERT_TRUE(outs[3].turn_right);

        unsigned char buf_p[] = {'p'};
        input_poll_all_from_buf(outs, 4, buf_p, sizeof(buf_p));
        TEST_ASSERT_TRUE(outs[3].turn_left);
    }

    game_config_destroy(cfg);
}
