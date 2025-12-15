#include "snake/input.h"
#include <assert.h>

int main(void)
{
    InputState s = {0};
    /* 'w' is default key_up; we expect move_up to be true after parsing. */
    const unsigned char buf[] = {'w'};
    input_poll_from_buf(&s, buf, sizeof(buf));
    assert(s.move_up == true);
    /* Newline-only input should not set any_key */
    const unsigned char nl[] = {'\n', '\r'};
    s = (InputState){0};
    input_poll_from_buf(&s, nl, sizeof(nl));
    assert(s.any_key == false);
    /* Arrow up escape sequence should set move_up and any_key */
    const unsigned char esc_up[] = {'\x1b', '[', 'A'};
    s = (InputState){0};
    input_poll_from_buf(&s, esc_up, sizeof(esc_up));
    assert(s.move_up == true);
    assert(s.any_key == true);
    return 0;
}
