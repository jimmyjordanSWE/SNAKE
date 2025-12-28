/* Legacy test moved to Unity: tests/unity/test_game_multi.c */
#include <stdio.h>
int main(void) { fprintf(stderr, "Deprecated: use tests/unity/test_game_multi.c\n"); return 77; }

    PlayerCfg* pc0 = player_cfg_create();
    player_cfg_set_name(pc0, "P0");
    player_cfg_set_color(pc0, 0xFF0000FF);
    int idx0 = game_add_player(g, pc0);
    assert(idx0 >= 0);
    assert(game_get_num_players(g) == initial + 1);

    PlayerCfg* pc1 = player_cfg_create();
    player_cfg_set_name(pc1, "P1");
    player_cfg_set_color(pc1, 0xFFFF0000);
    int idx1 = game_add_player(g, pc1);
    assert(idx1 >= 0);
    assert(game_get_num_players(g) == initial + 2);

    const GameState* gs = game_get_state(g);
    assert(gs != NULL);
    // validate names/colors set on added indices
    assert(strcmp(gs->players[idx0].name, "P0") == 0);
    assert(gs->players[idx0].color == 0xFF0000FF);
    assert(strcmp(gs->players[idx1].name, "P1") == 0);
    assert(gs->players[idx1].color == 0xFFFF0000);

    // cleanup
    player_cfg_destroy(pc0);
    player_cfg_destroy(pc1);
    game_destroy(g);
    game_config_destroy(cfg);
    printf("OK\n");
    return 0;
}
