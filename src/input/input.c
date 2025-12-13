#include "snake/input.h"

bool input_init(void) {
    return true;
}

void input_shutdown(void) {
}

void input_poll(InputState* out) {
    if (!out) { return; }

    *out = (InputState){0};
}
