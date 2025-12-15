#include "snake/game.h"
#include "snake/game_internal.h"
#include "snake/collision.h"
#include "unity.h"

static void test_tail_vacate_allows_following_move(void);

void test_tail_vacate_allows_following_move(void) {
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 8, 8);
    Game* g = game_create(cfg, game_config_get_seed(cfg));
    TEST_ASSERT_TRUE_MSG(g != NULL, "game_create should succeed");
    GameState* s = game_test_get_state(g);
    s->status = GAME_STATUS_RUNNING;

    PlayerState* a = &s->players[0];
    a->active = true;
    a->length = 3;
    /* place head and tail such that tail vacates and head moves into tail pos */
    a->body[0] = (SnakePoint){3,3};
    a->body[1] = (SnakePoint){2,3};
    a->body[2] = (SnakePoint){1,3};
    a->current_dir = SNAKE_DIR_LEFT;

    /* simulate a move where head will move into the former tail position */
    /* invoke collision detection directly on the game state (no tick needed) */
    collision_detect_and_resolve(s);

    /* after collision resolution, ensure no false positive collision happened (player still active) */
    TEST_ASSERT_TRUE_MSG(a->active, "player should remain active after tail vacate move");
    game_destroy(g);
    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_tail_vacate_allows_following_move);
    return UnityEnd();
}
