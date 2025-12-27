#include "input.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
static inline unsigned char ascii_tolower(unsigned char c) {
    if (c >= 'A' && c <= 'Z')
        return (unsigned char)(c - 'A' + 'a');
    return c;
}
static struct termios g_original_termios;
static int g_stdin_flags = -1;
static bool g_initialized = false;
#include "game.h"
static char s_key_turn_left = 'a';
static char s_key_turn_right = 'd';
static char s_key_quit = 'q';
static char s_key_restart = 'r';
static char s_key_pause = 'p';
// Per-player bindings
static char s_key_up[SNAKE_MAX_PLAYERS];
static char s_key_down[SNAKE_MAX_PLAYERS];
static char s_key_left[SNAKE_MAX_PLAYERS];
static char s_key_right[SNAKE_MAX_PLAYERS];

static void restore_terminal(void) {
    if (g_initialized) {
        if (g_stdin_flags >= 0) {
            if (fcntl(STDIN_FILENO, F_SETFL, g_stdin_flags) == -1) {
                perror("restore_terminal: fcntl(F_SETFL)");
            }
        }
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios) == -1) {
            perror("restore_terminal: tcsetattr");
        }
        g_initialized = false;
    }
}
bool input_init(void) {
    if (g_initialized)
        return true;
    if (tcgetattr(STDIN_FILENO, &g_original_termios) < 0)
        return false;
    if (atexit(restore_terminal) != 0) {
        return false;
    }
    struct termios new_termios = g_original_termios;
    new_termios.c_lflag &= (unsigned int)~(unsigned int)(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) < 0)
        return false;
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
    // default per-player bindings for up to SNAKE_MAX_PLAYERS
    if (SNAKE_MAX_PLAYERS >= 1) { s_key_up[0] = 'w'; s_key_down[0] = 's'; s_key_left[0] = 'a'; s_key_right[0] = 'd'; }
    if (SNAKE_MAX_PLAYERS >= 2) { s_key_up[1] = 'i'; s_key_down[1] = 'k'; s_key_left[1] = 'j'; s_key_right[1] = 'l'; }
    if (SNAKE_MAX_PLAYERS >= 3) { s_key_up[2] = '8'; s_key_down[2] = '5'; s_key_left[2] = '4'; s_key_right[2] = '6'; }
    if (SNAKE_MAX_PLAYERS >= 4) { s_key_up[3] = 't'; s_key_down[3] = 'g'; s_key_left[3] = 'f'; s_key_right[3] = 'h'; }
    return true;
}
void input_shutdown(void) {
    restore_terminal();
}
static int parse_arrow_key(int code) {
    switch (code) {
    case 'A':
        return 0;
    case 'B':
        return 1;
    case 'C':
        return 2;
    case 'D':
        return 3;
    default:
        return -1;
    }
}
void input_poll(InputState* out) {
    if (!out)
        return;
    unsigned char buf[128];
    ssize_t nread = read(STDIN_FILENO, buf, sizeof(buf));
    if (nread <= 0)
        return;
    input_poll_from_buf(out, buf, (size_t)nread);
}
void input_poll_from_buf(InputState* out, const unsigned char* buf, size_t n) {
    if (!out)
        return;
    *out = (InputState){0};
    if (buf == NULL || n == 0)
        return;
    out->any_key = false;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = buf[i];
        if (c == '\x1b' && i + 2 < n && buf[i + 1] == '[') {
            int code = (int)buf[i + 2];
            int dir = parse_arrow_key(code);
            if (dir == 0)
                out->move_up = true;
            else if (dir == 1)
                out->move_down = true;
            else if (dir == 2)
                out->move_right = true;
            else if (dir == 3)
                out->move_left = true;
            out->any_key = true;
            i += 2;
            continue;
        }
        switch (c) {
        case '\r':
        case '\n':
            break;
        default:
            out->any_key = true;
            unsigned char lc = ascii_tolower((unsigned char)c);
            if (lc == ascii_tolower((unsigned char)s_key_turn_left))
                out->turn_left = true;
            else if (lc == ascii_tolower((unsigned char)s_key_turn_right))
                out->turn_right = true;
            else if (lc == ascii_tolower((unsigned char)s_key_quit))
                out->quit = true;
            else if (lc == ascii_tolower((unsigned char)s_key_restart))
                out->restart = true;
            else if (lc == ascii_tolower((unsigned char)s_key_pause))
                out->pause_toggle = true;
            break;
        }
    }
}
void input_set_key_bindings(char up,
                            char down,
                            char turn_left,
                            char turn_right,
                            char quit,
                            char restart,
                            char pause_toggle) {
    (void)up;
    (void)down;
    if (turn_left)
        s_key_turn_left = turn_left;
    if (turn_right)
        s_key_turn_right = turn_right;
    if (quit)
        s_key_quit = quit;
    if (restart)
        s_key_restart = restart;
    if (pause_toggle)
        s_key_pause = pause_toggle;
}

void input_set_player_key_bindings(int player_idx,
                                   char up,
                                   char down,
                                   char turn_left,
                                   char turn_right,
                                   char quit,
                                   char restart,
                                   char pause_toggle) {
    if (player_idx < 0 || player_idx >= SNAKE_MAX_PLAYERS)
        return;
    if (up)
        s_key_up[player_idx] = up;
    if (down)
        s_key_down[player_idx] = down;
    if (turn_left)
        s_key_left[player_idx] = turn_left;
    if (turn_right)
        s_key_right[player_idx] = turn_right;
    (void)quit;
    (void)restart;
    (void)pause_toggle;
}

static void input_poll_all_from_buf_impl(InputState* outs, int max_players, const unsigned char* buf, size_t n) {
    if (!outs || max_players <= 0 || buf == NULL || n == 0)
        return;
    for (int p = 0; p < max_players; ++p)
        outs[p] = (InputState){0};
    for (size_t i = 0; i < n; i++) {
        unsigned char c = buf[i];
        if (c == '\x1b' && i + 2 < n && buf[i + 1] == '[') {
            int code = (int)buf[i + 2];
            int dir = parse_arrow_key(code);
            for (int p = 0; p < max_players; ++p) {
                if (dir == 0)
                    outs[p].move_up = true;
                else if (dir == 1)
                    outs[p].move_down = true;
                else if (dir == 2)
                    outs[p].move_right = true;
                else if (dir == 3)
                    outs[p].move_left = true;
                outs[p].any_key = true;
            }
            i += 2;
            continue;
        }
        if (c == '\r' || c == '\n')
            continue;
        unsigned char lc = ascii_tolower((unsigned char)c);
        for (int p = 0; p < max_players; ++p) {
            if (s_key_left[p] && lc == ascii_tolower((unsigned char)s_key_left[p]))
                outs[p].turn_left = true;
            else if (s_key_right[p] && lc == ascii_tolower((unsigned char)s_key_right[p]))
                outs[p].turn_right = true;
            else if (s_key_up[p] && lc == ascii_tolower((unsigned char)s_key_up[p]))
                outs[p].move_up = true;
            else if (s_key_down[p] && lc == ascii_tolower((unsigned char)s_key_down[p]))
                outs[p].move_down = true;
            if (outs[p].move_up || outs[p].move_down || outs[p].turn_left || outs[p].turn_right)
                outs[p].any_key = true;
        }
        // Global controls
        if (lc == ascii_tolower((unsigned char)s_key_quit)) {
            for (int p = 0; p < max_players; ++p)
                outs[p].quit = true;
        } else if (lc == ascii_tolower((unsigned char)s_key_restart)) {
            for (int p = 0; p < max_players; ++p)
                outs[p].restart = true;
        } else if (lc == ascii_tolower((unsigned char)s_key_pause)) {
            for (int p = 0; p < max_players; ++p)
                outs[p].pause_toggle = true;
        }
    }
}

void input_poll_all_from_buf(InputState* outs, int max_players, const unsigned char* buf, size_t n) {
    input_poll_all_from_buf_impl(outs, max_players, buf, n);
}

void input_poll_all(InputState* outs, int max_players) {
    unsigned char buf[128];
    ssize_t nread = read(STDIN_FILENO, buf, sizeof(buf));
    if (nread <= 0)
        return;
    input_poll_all_from_buf_impl(outs, max_players, buf, (size_t)nread);
}
