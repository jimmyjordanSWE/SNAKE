#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "net.h"
#include "game_internal.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    struct GameState gs;
    memset(&gs, 0, sizeof(gs));
    /* call unpack; if it returns true, free allocated memory */
    if (net_unpack_game_state(data, size, (GameState*)&gs)) {
        net_free_unpacked_game_state((GameState*)&gs);
    }
    return 0;
}