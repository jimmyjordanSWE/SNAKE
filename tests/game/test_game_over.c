#include "snake/game.h"
#include <stdio.h>

int main(void) {
    GameState g = {0};
    g.width = 1; /* spawn_player rejects width < 2 */
    g.height = 1;
    g.num_players = 1;
    g.status = GAME_STATUS_RUNNING;

    /* Force a respawn attempt that will fail */
    g.players[0].needs_reset = true;

    game_tick(&g);

    if(g.status != GAME_STATUS_GAME_OVER) {
        fprintf(stderr, "FAIL: expected GAME_OVER when no players can be spawned\n");
        return 1;
    }
    return 0;
}
