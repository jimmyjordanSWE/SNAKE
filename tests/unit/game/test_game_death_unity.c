#include "snake/game.h"
#include "snake/game_internal.h"
#include "unity.h"

static void test_death_resets_player_and_records_score(void);

void test_death_resets_player_and_records_score(void) {
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 10, 10);
    game_config_set_tick_rate_ms(cfg, 100);
    game_config_set_render_glyphs(cfg, 0);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_enable_external_3d_view(cfg, 0);
    game_config_set_seed(cfg, 42);
    game_config_set_num_players(cfg, 2);
    game_config_set_max_players(cfg, 2);
    game_config_set_max_length(cfg, 8);
    game_config_set_max_food(cfg, 0);
    Game* g = game_create(cfg, game_config_get_seed(cfg));
    TEST_ASSERT_TRUE_MSG(g != NULL, "game_create should succeed");
    GameState* s = game_test_get_state(g);
    s->status = GAME_STATUS_RUNNING;

    s->players[0].active = true;
    s->players[0].length = 2;
    s->players[0].body[0] = (SnakePoint){4, 5};
    s->players[0].body[1] = (SnakePoint){4, 4};
    s->players[0].current_dir = SNAKE_DIR_RIGHT;

    s->players[1].active = true;
    s->players[1].length = 2;
    s->players[1].body[0] = (SnakePoint){6, 5};
    s->players[1].body[1] = (SnakePoint){7, 5};
    s->players[1].current_dir = SNAKE_DIR_LEFT;

    GameEvents ev = {0};
    game_step(g, &ev);

    int died_count = ev.died_count;
    int found0 = 0, found1 = 0;
    for (int i = 0; i < died_count; i++) {
        if (ev.died_players[i] == 0) found0 = 1;
        if (ev.died_players[i] == 1) found1 = 1;
    }

    game_destroy(g);
    game_config_destroy(cfg);

    TEST_ASSERT_TRUE_MSG(died_count == 2 && found0 && found1, "both players should die in head-on collision");
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_death_resets_player_and_records_score);
    return UnityEnd();
}
