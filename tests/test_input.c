#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "input.h"

int main(void) {
    InputState out = {0};
    /* Long buffer of printable chars - should not crash and set any_key */
    unsigned char buf[512];
    for (int i = 0; i < (int)sizeof(buf); ++i) buf[i] = 'a' + (i % 26);
    input_poll_from_buf(&out, buf, sizeof(buf));
    assert(out.any_key == true);

    /* Malformed escape sequences should not cause over-reads */
    InputState out2 = {0};
    unsigned char esc1[] = {0x1b, '[', 'Z'}; /* Z is not an arrow code */
    input_poll_from_buf(&out2, esc1, sizeof(esc1));
    /* Should not set move flags but may set any_key */
    assert(out2.move_up == false && out2.move_down == false && out2.move_left == false && out2.move_right == false);

    /* Proper arrow key sequences should set moves */
    InputState out3 = {0};
    unsigned char upseq[] = {0x1b, '[', 'A'};
    input_poll_from_buf(&out3, upseq, sizeof(upseq));
    assert(out3.move_up == true);

    printf("test_input: OK\n");
    return 0;
}
