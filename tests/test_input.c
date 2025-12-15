#include "snake/input.h"
#include <assert.h>

int main(void)
{
    InputState s = {0};
    /* 'w' is default key_up; we expect move_up to be true after parsing. */
    const unsigned char buf[] = {'w'};
    input_poll_from_buf(&s, buf, sizeof(buf));
    assert(s.move_up == true);
    return 0;
}
