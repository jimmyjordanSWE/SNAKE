#include "snake/input.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios g_original_termios;
static int g_stdin_flags = -1;
static bool g_initialized = false;

// Restore terminal on normal exit.
static void restore_terminal(void) {
    if (g_initialized) {
        if (g_stdin_flags >= 0) { fcntl(STDIN_FILENO, F_SETFL, g_stdin_flags); }
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
        g_initialized = false;
    }
}

bool input_init(void) {
    if (g_initialized) { return true; }

    // Save original terminal settings.
    if (tcgetattr(STDIN_FILENO, &g_original_termios) < 0) { return false; }

    // Register cleanup handler.
    atexit(restore_terminal);

    // Set raw-ish mode: no echo, no canonical.
    struct termios new_termios = g_original_termios;
    new_termios.c_lflag &= (unsigned int)~(unsigned int)(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;  // Non-blocking read.
    new_termios.c_cc[VTIME] = 0; // No timeout.

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) < 0) { return false; }

    // Save original stdin flags and set non-blocking.
    g_stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (g_stdin_flags < 0) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
        return false;
    }

    if (fcntl(STDIN_FILENO, F_SETFL, g_stdin_flags | O_NONBLOCK) < 0) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
        g_stdin_flags = -1;
        return false;
    }

    g_initialized = true;
    return true;
}

void input_shutdown(void) {
    restore_terminal();
}

// Helper to parse ANSI escape sequence for arrow keys.
// Returns 0=up, 1=down, 2=right, 3=left if valid, or -1 if not.
static int parse_arrow_key(int code) {
    switch (code) {
    case 'A':
        return 0; // up
    case 'B':
        return 1; // down
    case 'C':
        return 2; // right
    case 'D':
        return 3; // left
    default:
        return -1;
    }
}

void input_poll(InputState* out) {
    if (!out) { return; }

    // Zero the output.
    *out = (InputState){0};

    // Read all available input without blocking.
    unsigned char buf[128];
    ssize_t nread = read(STDIN_FILENO, buf, sizeof(buf));

    if (nread <= 0) { return; }

    out->any_key = true;

    // Process each byte.
    for (ssize_t i = 0; i < nread; i++) {
        unsigned char c = buf[i];

        // Check for escape sequence start.
        if (c == '\x1b' && i + 2 < nread && buf[i + 1] == '[') {
            int code = (int)buf[i + 2];
            int dir = parse_arrow_key(code);
            if (dir == 0) {
                out->move_up = true;
            } else if (dir == 1) {
                out->move_down = true;
            } else if (dir == 2) {
                out->move_right = true;
            } else if (dir == 3) {
                out->move_left = true;
            }
            // Skip the escape sequence.
            i += 2;
            continue;
        }

        // Single character commands.
        switch (c) {
        // Player 1 (WASD + arrows).
        case 'w':
        case 'W':
            out->move_up = true;
            break;
        case 's':
        case 'S':
            out->move_down = true;
            break;
        case 'a':
        case 'A':
            out->move_left = true;
            break;
        case 'd':
        case 'D':
            out->move_right = true;
            break;

        // Global commands.
        case 'q':
        case 'Q':
            out->quit = true;
            break;
        case 'r':
        case 'R':
            out->restart = true;
            break;
        case 'p':
        case 'P':
            out->pause_toggle = true;
            break;
        case 'v':
        case 'V':
            out->view_toggle = true;
            break;

        // Ignore unknown keys.
        default:
            break;
        }
    }
}
