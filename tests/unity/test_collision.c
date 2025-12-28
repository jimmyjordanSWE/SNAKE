#include "unity.h"
#include <string.h>
#include <stdlib.h>
#include "collision.h"
#include "game_internal.h"

TEST(test_collision) {
    SnakePoint p1 = { .x = -1, .y = 0 };
    TEST_ASSERT_TRUE(collision_is_wall(p1, 10, 10));
    SnakePoint p2 = { .x = 5, .y = 5 };
    TEST_ASSERT_FALSE(collision_is_wall(p2, 10, 10));

    SnakePoint cur = { .x = 2, .y = 2 };
    SnakePoint up = collision_next_head(cur, SNAKE_DIR_UP);
    TEST_ASSERT_EQUAL_INT(2, up.x);
    TEST_ASSERT_EQUAL_INT(1, up.y);

    struct GameState g;
    memset(&g, 0, sizeof(g));
    g.width = 5; g.height = 5; g.num_players = 2; g.max_players = 2; g.max_length = 4;
    g.players = calloc((size_t)g.max_players, sizeof(struct PlayerState));
    for (int i = 0; i < g.max_players; i++) {
        g.players[i].max_length = g.max_length;
        g.players[i].body = calloc((size_t)g.max_length, sizeof(SnakePoint));
    }
    g.players[0].active = true; g.players[0].length = 1;
    g.players[0].body[0] = (SnakePoint){2,2}; g.players[0].current_dir = SNAKE_DIR_RIGHT;
    g.players[1].active = true; g.players[1].length = 1;
    g.players[1].body[0] = (SnakePoint){3,2}; g.players[1].current_dir = SNAKE_DIR_LEFT;
    collision_detect_and_resolve(&g);
    TEST_ASSERT_TRUE(g.players[0].needs_reset);
    TEST_ASSERT_TRUE(g.players[1].needs_reset);

    for (int i = 0; i < g.max_players; i++) free(g.players[i].body);
    free(g.players);
}


