#include "unity.h"
#include <string.h>
#include <arpa/inet.h>
#include "net.h"
#include "game_internal.h"

TEST(test_net) {
    unsigned char buf[10] = {0};
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    TEST_ASSERT_FALSE(net_unpack_game_state(buf, sizeof(buf), &gs));

    unsigned char buf2[32] = {0};
    uint32_t v;
    v = htonl(10); memcpy(buf2+0, &v, 4);
    v = htonl(10); memcpy(buf2+4, &v, 4);
    v = htonl(1); memcpy(buf2+16, &v, 4);
    v = htonl(10); memcpy(buf2+20, &v, 4);
    memset(&gs, 0, sizeof(gs));
    TEST_ASSERT_FALSE(net_unpack_game_state(buf2, sizeof(buf2), &gs));

    unsigned char buf3[64] = {0};
    v = htonl(10); memcpy(buf3+0, &v, 4);
    v = htonl(10); memcpy(buf3+4, &v, 4);
    v = htonl(1000000); memcpy(buf3+16, &v, 4);
    v = htonl(0); memcpy(buf3+20, &v, 4);
    memset(&gs, 0, sizeof(gs));
    TEST_ASSERT_FALSE(net_unpack_game_state(buf3, sizeof(buf3), &gs));

    unsigned char buf4[64] = {0};
    v = htonl(0); memcpy(buf4+0, &v, 4);
    v = htonl(10); memcpy(buf4+4, &v, 4);
    memset(&gs, 0, sizeof(gs));
    TEST_ASSERT_FALSE(net_unpack_game_state(buf4, sizeof(buf4), &gs));
}


