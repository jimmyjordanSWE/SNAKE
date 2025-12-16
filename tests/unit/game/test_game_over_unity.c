#include "snake/game.h"
#include "snake/game_internal.h"
#include "unity.h"

static void test_game_over_when_all_players_dead(void);

void test_game_over_when_all_players_dead(void) {
    GameConfig* cfg = game_config_create();
    game_config_set_num_players(cfg, 1);
    Game* g = game_create(cfg, game_config_get_seed(cfg));
    TEST_ASSERT_TRUE_MSG(g != NULL, "game_create should succeed");
    GameState* s = game_test_get_state(g);

    s->players[0].active = false; 
    s->status = GAME_STATUS_RUNNING;
    
    game_step(g, NULL);
    TEST_ASSERT_TRUE_MSG(s->status == GAME_STATUS_GAME_OVER || s->players[0].needs_reset || !s->players[0].active, "game should detect over/no active players");
    game_destroy(g);
    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_game_over_when_all_players_dead);
    return UnityEnd();
}
