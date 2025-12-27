#include "input.h"
#include <assert.h>
#include <string.h>

int main(void) {
    InputState outs[4];
    // set custom bindings
    input_set_player_key_bindings(0, 'w', 's', 'a', 'd', 0, 0, 0);
    input_set_player_key_bindings(1, 'i', 'k', 'j', 'l', 0, 0, 0);

    unsigned char buf1[] = {'a', 'j'};
    input_poll_all_from_buf(outs, 2, buf1, sizeof(buf1));
    // player0 'a' => turn_left
    assert(outs[0].turn_left && "player0 should have turn_left");
    // player1 'j' => turn_left
    assert(outs[1].turn_left && "player1 should have turn_left");

    // arrow key up should set move_up for both players
    unsigned char arrow_up[] = {'\x1b', '[', 'A'};
    input_poll_all_from_buf(outs, 2, arrow_up, sizeof(arrow_up));
    assert(outs[0].move_up && outs[1].move_up);

    return 0;
}
