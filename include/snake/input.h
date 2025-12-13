#pragma once

#include <stdbool.h>

/*
 * Input module - provides raw input state without game-specific interpretation
 *
 * This module is now decoupled from game types to improve reusability and testability.
 * The caller (main or game controller) is responsible for translating raw inputs
 * into game-specific actions.
 */

typedef struct {
    /* Global commands */
    bool quit;
    bool restart;
    bool pause_toggle;
    bool view_toggle; // Toggle between 2D/3D view modes

    /* Generic flag for any keypress */
    bool any_key;

    /* Raw directional input (not game-specific directions) */
    bool move_up;
    bool move_down;
    bool move_left;
    bool move_right;
} InputState;

bool input_init(void);
void input_shutdown(void);
void input_poll(InputState* out);
