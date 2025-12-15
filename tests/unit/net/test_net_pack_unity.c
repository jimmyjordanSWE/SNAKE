#include "snake/net.h"
#include "snake/input.h"
#include "snake/game.h"
#include "snake/game_internal.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

static void test_pack_unpack_input(void);
static void test_pack_unpack_game_state(void);

void test_pack_unpack_input(void) {
    unsigned char buf[512];
    InputState in = {0};
    in.move_up = true;
    in.quit = true;
    size_t n = net_pack_input(&in, buf, sizeof(buf));
    TEST_ASSERT_TRUE_MSG(n == 1, "net_pack_input size should be 1");
    InputState out = {0};
    TEST_ASSERT_TRUE_MSG(net_unpack_input(buf, n, &out) == true, "net_unpack_input should succeed");
    TEST_ASSERT_TRUE_MSG(out.move_up && out.quit, "unpacked flags should match");
}

void test_pack_unpack_game_state(void) {
    unsigned char buf[1024];
    GameState g = {0};
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 20, 10);
    game_config_set_tick_rate_ms(cfg, 100);
    game_config_set_render_glyphs(cfg, 0);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_enable_external_3d_view(cfg, 1);
    game_config_set_seed(cfg, 42);
    game_config_set_fov_degrees(cfg, 90.0f);
    game_config_set_show_sprite_debug(cfg, 0);
    game_config_set_active_player(cfg, 0);
    game_config_set_num_players(cfg, 1);
    game_config_set_max_players(cfg, 2);
    game_config_set_max_length(cfg, 16);
    game_config_set_max_food(cfg, 4);
    g.max_players = game_config_get_max_players(cfg);
    g.max_length = game_config_get_max_length(cfg);
    g.max_food = game_config_get_max_food(cfg);
    g.players = malloc(sizeof(PlayerState) * (size_t)g.max_players);
    g.food = malloc(sizeof(SnakePoint) * (size_t)g.max_food);
    for(int i=0;i<g.max_players;i++) g.players[i].body = malloc(sizeof(SnakePoint) * (size_t)g.max_length);

    g.width = 20; g.height = 10; g.rng_state = 42; g.status = GAME_STATUS_RUNNING; g.num_players = 1; g.food_count = 1;
    g.food[0].x = 3; g.food[0].y = 4;
    g.players[0].score = 7; g.players[0].length = 5;
    size_t s = net_pack_game_state(&g, buf, sizeof(buf));
    TEST_ASSERT_TRUE_MSG(s > 0, "packed game state should be >0");
    GameState g2 = {0};
    TEST_ASSERT_TRUE_MSG(net_unpack_game_state(buf, s, &g2), "unpack game state should succeed");
    TEST_ASSERT_EQUAL_INT(20, g2.width);
    TEST_ASSERT_EQUAL_INT(10, g2.height);
    TEST_ASSERT_EQUAL_INT(42, g2.rng_state);
    TEST_ASSERT_EQUAL_INT(1, g2.num_players);
    TEST_ASSERT_EQUAL_INT(7, g2.players[0].score);
    TEST_ASSERT_EQUAL_INT(5, g2.players[0].length);
    TEST_ASSERT_EQUAL_INT(1, g2.food_count);
    TEST_ASSERT_EQUAL_INT(3, g2.food[0].x);
    TEST_ASSERT_EQUAL_INT(4, g2.food[0].y);

    InputState bad;
    TEST_ASSERT_TRUE_MSG(!net_unpack_input(NULL, 0, &bad), "unpack_input with NULL should fail");
    unsigned char empty[1] = {0};
    TEST_ASSERT_TRUE_MSG(!net_unpack_game_state(empty, 1, &g2), "unpack_game_state with bad data should fail");

    for(int i=0;i<g.max_players;i++) free(g.players[i].body);
    free(g.players); free(g.food);
    for(int i=0;i<g2.num_players;i++) if(g2.players && g2.players[i].body) free(g2.players[i].body);
    free(g2.players); free(g2.food);
    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_pack_unpack_input);
    RUN_TEST(test_pack_unpack_game_state);
    return UnityEnd();
}
