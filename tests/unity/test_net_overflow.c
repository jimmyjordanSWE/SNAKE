#include "unity.h"
#include <string.h>
#include <arpa/inet.h>
#include "net.h"
#include "game_internal.h"

TEST(test_net_overflow) {
    unsigned char buf[24] = {0};
    uint32_t v;
    v = htonl(1); memcpy(buf + 0, &v, 4);
    v = htonl(1); memcpy(buf + 4, &v, 4);
    v = htonl(0); memcpy(buf + 8, &v, 4);
    v = htonl(0); memcpy(buf + 12, &v, 4);
    v = htonl(0x7fffffff); memcpy(buf + 16, &v, 4);
    v = htonl(0x7fffffff); memcpy(buf + 20, &v, 4);
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    TEST_ASSERT_FALSE(net_unpack_game_state(buf, sizeof(buf), &gs));
}


