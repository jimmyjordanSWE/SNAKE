#include <stdio.h>
#include <string.h>
#include "net.h"
#include "game_internal.h"

int main(void) {
    unsigned char small[8] = {0};
    struct GameState s;
    memset(&s, 0, sizeof(s));
    bool ok = net_unpack_game_state(small, sizeof(small), (GameState*)&s);
    if (ok) {
        net_free_unpacked_game_state((GameState*)&s);
        printf("FAIL: expected net_unpack_game_state to return false for too-small buffer\n");
        return 1;
    }
    /* ok should be false and s.food/s.players should be NULL */
    if (s.food != NULL || s.players != NULL) {
        net_free_unpacked_game_state((GameState*)&s);
        printf("FAIL: expected no allocations on failure\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
