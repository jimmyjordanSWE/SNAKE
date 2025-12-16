#include "snake/game.h"
#include "snake/game_internal.h"
#include "snake/persist.h"
#include <stdio.h>

int main(void) {
    GameConfig* cfg = game_config_create();
    if(!cfg) {
        fprintf(stderr, "FAIL: game_config_create returned NULL\n");
        return 1;
    }
    int bw = 0, bh = 0;
    game_config_get_board_size(cfg, &bw, &bh);
    if(bw != 40 || bh != 20) {
        fprintf(stderr, "FAIL: expected default board size 40x20, got %dx%d\n", bw, bh);
        game_config_destroy(cfg);
        return 1;
    }
    if(game_config_get_tick_rate_ms(cfg) != 100) {
        fprintf(stderr, "FAIL: expected default tick rate 100ms, got %dms\n", game_config_get_tick_rate_ms(cfg));
        game_config_destroy(cfg);
        return 1;
    }

    game_config_set_render_glyphs(cfg, 0);
    Game* g = game_create(cfg, game_config_get_seed(cfg));
    if(!g) {
        fprintf(stderr, "FAIL: game_create failed\n");
        game_config_destroy(cfg);
        return 1;
    }
    const GameState* st = game_get_state(g);
    if(!st) {
        fprintf(stderr, "FAIL: game_get_state returned NULL\n");
        game_destroy(g);
        game_config_destroy(cfg);
        return 1;
    }
    if(st->width != 40 || st->height != 20) {
        fprintf(stderr, "FAIL: runtime game state size mismatch: %dx%d\n", st->width, st->height);
        game_destroy(g);
        game_config_destroy(cfg);
        return 1;
    }

    /* Step a few ticks to ensure the game runs at the default tick rate without crashing */
    for(int i = 0; i < 10; i++) {
        GameEvents ev = {0};
        game_step(g, &ev);
    }

    game_destroy(g);
    game_config_destroy(cfg);
    return 0;
}
