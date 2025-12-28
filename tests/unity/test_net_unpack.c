#include "unity.h"
#include <string.h>
#include "net.h"
#include "game_internal.h"

TEST(test_net_unpack) {
    unsigned char small[8] = {0};
    GameState s;
    memset(&s, 0, sizeof(s));
    int ok = net_unpack_game_state(small, sizeof(small), &s);
    TEST_ASSERT_FALSE(ok);
    /* ok should be false and s.food/s.players should be NULL */
    TEST_ASSERT_TRUE(s.food == NULL && s.players == NULL);
}
