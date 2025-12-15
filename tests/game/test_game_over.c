#include "snake/game.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    GameState g = {0};
    GameConfig cfg = { .board_width = 20, .board_height = 10, .tick_rate_ms = 100, .render_glyphs = 0, .screen_width = 80, .screen_height = 25, .render_mode = 1, .seed = 42, .fov_degrees = 90.0f, .show_minimap = 0, .show_stats = 0, .show_sprite_debug = 0, .active_player = 0, .num_players = 1, .max_players = 2, .max_length = 16, .max_food = 4 };
    /* Allocate minimal structures but do not spawn players, so we can simulate
     * an impossible respawn (width < 2) similar to the previous test. */
    g.max_players = cfg.max_players;
    g.max_length = cfg.max_length;
    g.max_food = cfg.max_food;
    g.players = (PlayerState*)malloc(sizeof(PlayerState) * (size_t)g.max_players);
    for(int i = 0; i < g.max_players; i++) {
        g.players[i].body = (SnakePoint*)malloc(sizeof(SnakePoint) * (size_t)g.max_length);
        g.players[i].active = false;
        g.players[i].needs_reset = false;
        g.players[i].length = 0;
    }
    /* Force an impossible respawn by shrinking board */
    g.width = 1; /* spawn_player rejects width < 2 */
    g.height = 1;
    g.num_players = 1;
    g.status = GAME_STATUS_RUNNING;

    /* Force a respawn attempt that will fail */
    g.players[0].needs_reset = true;

    game_tick(&g);

    if(g.status != GAME_STATUS_GAME_OVER) {
        fprintf(stderr, "FAIL: expected GAME_OVER when no players can be spawned\n");
        for(int i=0;i<g.max_players;i++) free(g.players[i].body);
        free(g.players);
        return 1;
    }
    for(int i=0;i<g.max_players;i++) free(g.players[i].body);
    free(g.players);
    return 0;
}
