#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include "net.h"
#include "game_internal.h"

int main(void) {
    unsigned char buf[24] = {0};
    uint32_t v;
    v = htonl(1); memcpy(buf + 0, &v, 4);
    v = htonl(1); memcpy(buf + 4, &v, 4);
    v = htonl(0); memcpy(buf + 8, &v, 4);
    v = htonl(0); memcpy(buf + 12, &v, 4);
    v = htonl(0x7fffffff); memcpy(buf + 16, &v, 4); /* num_players */
    v = htonl(0x7fffffff); memcpy(buf + 20, &v, 4); /* food_count */
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    /* Should not crash or allocate huge memory; must return false */
    assert(net_unpack_game_state(buf, sizeof(buf), &gs) == false);
    printf("test_net_overflow: OK\n");
    return 0;
}
