#pragma once

#include <stdbool.h>

#include "snake/types.h"

typedef struct {
    bool quit;
    bool restart;
    bool pause_toggle;

    SnakeDir p1_dir;
    bool p1_dir_set;

    SnakeDir p2_dir;
    bool p2_dir_set;
} InputState;

bool input_init(void);
void input_shutdown(void);
void input_poll(InputState* out);
