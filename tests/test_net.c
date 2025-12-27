#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "net.h"
#include "game_internal.h"
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>

int main(void) {
    /* too small buffer */
    unsigned char buf[10] = {0};
    struct GameState gs;
    memset(&gs, 0, sizeof(gs));
    assert(net_unpack_game_state(buf, sizeof(buf), (GameState*)&gs) == false);

    /* header but truncated payload */
    unsigned char buf2[32] = {0};
    /* set width/height/..., num_players=1, food_count=10 (but payload missing) */
    uint32_t v;
    v = htonl(10); memcpy(buf2+0, &v, 4);
    v = htonl(10); memcpy(buf2+4, &v, 4);
    v = htonl(1); memcpy(buf2+16, &v, 4); /* num_players */
    v = htonl(10); memcpy(buf2+20, &v, 4); /* food_count */
    memset(&gs, 0, sizeof(gs));
    assert(net_unpack_game_state(buf2, sizeof(buf2), (GameState*)&gs) == false);

    /* huge num_players indicated in header (too large) */
    unsigned char buf3[64] = {0};
    v = htonl(10); memcpy(buf3+0, &v, 4);
    v = htonl(10); memcpy(buf3+4, &v, 4);
    v = htonl(1000000); memcpy(buf3+16, &v, 4); /* num_players */
    v = htonl(0); memcpy(buf3+20, &v, 4); /* food_count */
    memset(&gs, 0, sizeof(gs));
    assert(net_unpack_game_state(buf3, sizeof(buf3), (GameState*)&gs) == false);

    /* zero width/height invalid */
    unsigned char buf4[64] = {0};
    v = htonl(0); memcpy(buf4+0, &v, 4);
    v = htonl(10); memcpy(buf4+4, &v, 4);
    memset(&gs, 0, sizeof(gs));
    assert(net_unpack_game_state(buf4, sizeof(buf4), (GameState*)&gs) == false);

    printf("test_net: OK\n");
    return 0;
}