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
// Returns SNAKE_DIR_UP/DOWN/LEFT/RIGHT if valid, or -1 if not.
static int parse_arrow_key(int code) {
    switch (code) {
    case 'A':
        return SNAKE_DIR_UP;
    case 'B':
        return SNAKE_DIR_DOWN;
    case 'C':
        return SNAKE_DIR_RIGHT;
    case 'D':
        return SNAKE_DIR_LEFT;
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

    // Process each byte.
    for (ssize_t i = 0; i < nread; i++) {
        unsigned char c = buf[i];

        // Check for escape sequence start.
        if (c == '\x1b' && i + 2 < nread && buf[i + 1] == '[') {
            int code = (int)buf[i + 2];
            int dir = parse_arrow_key(code);
            if (dir >= 0) {
                if (dir == SNAKE_DIR_UP || dir == SNAKE_DIR_DOWN) {
                    out->p1_dir = (SnakeDir)dir;
                    out->p1_dir_set = true;
                } else if (dir == SNAKE_DIR_LEFT || dir == SNAKE_DIR_RIGHT) {
                    out->p1_dir = (SnakeDir)dir;
                    out->p1_dir_set = true;
                }
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
            out->p1_dir = SNAKE_DIR_UP;
            out->p1_dir_set = true;
            break;
        case 's':
        case 'S':
            out->p1_dir = SNAKE_DIR_DOWN;
            out->p1_dir_set = true;
            break;
        case 'a':
        case 'A':
            out->p1_dir = SNAKE_DIR_LEFT;
            out->p1_dir_set = true;
            break;
        case 'd':
        case 'D':
            out->p1_dir = SNAKE_DIR_RIGHT;
            out->p1_dir_set = true;
            break;

        // Player 2 (IJKL).
        case 'i':
        case 'I':
            out->p2_dir = SNAKE_DIR_UP;
            out->p2_dir_set = true;
            break;
        case 'k':
        case 'K':
            out->p2_dir = SNAKE_DIR_DOWN;
            out->p2_dir_set = true;
            break;
        case 'j':
        case 'J':
            out->p2_dir = SNAKE_DIR_LEFT;
            out->p2_dir_set = true;
            break;
        case 'l':
        case 'L':
            out->p2_dir = SNAKE_DIR_RIGHT;
            out->p2_dir_set = true;
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

        // Ignore unknown keys.
        default:
            break;
        }
    }
}
