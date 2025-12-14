#include <assert.h>
#include <string.h>
#include "snake/input.h"

int main(void) {
    InputState st;

    /* simple key */
    memset(&st, 0, sizeof(st));
    unsigned char w[] = "w";
    input_poll_from_buf(&st, w, sizeof(w) - 1);
    assert(st.any_key && st.move_up && !st.move_down);

    /* arrow up */
    memset(&st, 0, sizeof(st));
    unsigned char up[] = "\x1b[A";
    input_poll_from_buf(&st, up, sizeof(up) - 1);
    assert(st.any_key && st.move_up);

    /* multiple keys */
    memset(&st, 0, sizeof(st));
    unsigned char many[] = "sdaQ"; /* down, strafe-right, strafe-left, quit (case insens.) */
    input_poll_from_buf(&st, many, sizeof(many) - 1);
    assert(st.any_key && st.move_down && st.move_strafe_right && st.move_strafe_left && st.quit);

    /* ignored key */
    memset(&st, 0, sizeof(st));
    unsigned char v[] = "v";
    input_poll_from_buf(&st, v, sizeof(v) - 1);
    assert(st.any_key && !st.move_up && !st.move_down && !st.quit);

    return 0;
}
