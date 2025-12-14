#include <assert.h>
#include <string.h>
#include "snake/net.h"
#include "snake/input.h"
#include "snake/game.h"

int main(void) {
    unsigned char buf[512];
    InputState in = {0};
    in.move_up = true;
    in.quit = true;
    size_t n = net_pack_input(&in, buf, sizeof(buf));
    assert(n == 1);
    InputState out = {0};
    assert(net_unpack_input(buf, n, &out));
    assert(out.move_up && out.quit);

    GameState g = {0};
    g.width = 20; g.height = 10; g.rng_state = 42; g.status = GAME_STATUS_RUNNING; g.num_players = 1; g.food_count = 1;
    g.food[0].x = 3; g.food[0].y = 4;
    g.players[0].score = 7; g.players[0].length = 5;
    size_t s = net_pack_game_state(&g, buf, sizeof(buf));
    assert(s > 0);
    GameState g2 = {0};
    assert(net_unpack_game_state(buf, s, &g2));
    assert(g2.width == 20 && g2.height == 10 && g2.rng_state == 42);
    assert(g2.num_players == 1 && g2.players[0].score == 7 && g2.players[0].length == 5);
    assert(g2.food_count == 1 && g2.food[0].x == 3 && g2.food[0].y == 4);
    /* Negative/error cases */
    InputState bad;
    assert(!net_unpack_input(NULL, 0, &bad));
    unsigned char empty[1] = {0};
    assert(!net_unpack_game_state(empty, 1, &g2));
    return 0;
}
