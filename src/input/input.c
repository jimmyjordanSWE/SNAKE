#include "input.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
static inline unsigned char ascii_tolower(unsigned char c) {
if(c >= 'A' && c <= 'Z') return (unsigned char)(c - 'A' + 'a');
return c;
}
static struct termios g_original_termios;
static int g_stdin_flags= -1;
static bool g_initialized= false;
#include "game.h"
#include "persist.h"
/* Do not hard-code defaults here; bindings are applied from GameConfig via input_set_bindings_from_config().
 * Initialize to 0 so behavior comes strictly from config (or macros used by game_config_create()). */
static char s_key_quit= '\0';
static char s_key_restart= '\0';
static char s_key_pause= '\0';
/* Per-player bindings (zero-initialized until set from config) */
static char s_key_left[SNAKE_MAX_PLAYERS]= {0};
static char s_key_right[SNAKE_MAX_PLAYERS]= {0};
static void restore_terminal(void) {
if(g_initialized) {
if(g_stdin_flags >= 0) {
if(fcntl(STDIN_FILENO, F_SETFL, g_stdin_flags) == -1) { perror("restore_terminal: fcntl(F_SETFL)"); }
}
if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios) == -1) { perror("restore_terminal: tcsetattr"); }
g_initialized= false;
}
}
bool input_init(void) {
if(g_initialized) return true;
if(tcgetattr(STDIN_FILENO, &g_original_termios) < 0) return false;
if(atexit(restore_terminal) != 0) { return false; }
struct termios new_termios= g_original_termios;
new_termios.c_lflag&= (unsigned int)~(unsigned int)(ICANON | ECHO);
new_termios.c_cc[VMIN]= 0;
new_termios.c_cc[VTIME]= 0;
if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) < 0) return false;
g_stdin_flags= fcntl(STDIN_FILENO, F_GETFL, 0);
if(g_stdin_flags < 0) {
(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
return false;
}
if(fcntl(STDIN_FILENO, F_SETFL, g_stdin_flags | O_NONBLOCK) < 0) {
(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
g_stdin_flags= -1;
return false;
}
g_initialized= true;
/* bindings are set from config or tests; no hard-coded per-player defaults here. */
return true;
}
void input_shutdown(void) { restore_terminal(); }
static int parse_arrow_key(int code) {
switch(code) {
case 'A': return 0;
case 'B': return 1;
case 'C': return 2;
case 'D': return 3;
default: return -1;
}
}
void input_poll(InputState* out) {
if(!out) return;
unsigned char buf[128];
ssize_t nread= read(STDIN_FILENO, buf, sizeof(buf));
if(nread <= 0) return;
input_poll_from_buf(out, buf, (size_t)nread);
}
void input_poll_from_buf(InputState* out, const unsigned char* buf, size_t n) {
if(!out) return;
*out= (InputState){0};
if(buf == NULL || n == 0) return;
out->any_key= false;
for(size_t i= 0; i < n; i++) {
unsigned char c= buf[i];
if(c == '\x1b' && i + 2 < n && buf[i + 1] == '[') {
int code= (int)buf[i + 2];
int dir= parse_arrow_key(code);
/* Only left/right from arrow keys are meaningful (turns). Ignore up/down. */
if(dir == 2)
out->turn_right= true;
else if(dir == 3)
out->turn_left= true;
out->any_key= true;
i+= 2;
continue;
}
switch(c) {
case '\r':
case '\n': break;
default:
out->any_key= true;
unsigned char lc= ascii_tolower((unsigned char)c);
/* single-player char keys map to player 0's left/right */
if(s_key_left[0] && lc == ascii_tolower((unsigned char)s_key_left[0]))
out->turn_left= true;
else if(s_key_right[0] && lc == ascii_tolower((unsigned char)s_key_right[0]))
out->turn_right= true;
else if(lc == ascii_tolower((unsigned char)s_key_quit))
out->quit= true;
else if(lc == ascii_tolower((unsigned char)s_key_restart))
out->restart= true;
else if(lc == ascii_tolower((unsigned char)s_key_pause))
out->pause_toggle= true;
break;
}
}
}
void input_set_player_key_bindings(int player_idx, char left, char right) {
if(player_idx < 0 || player_idx >= SNAKE_MAX_PLAYERS) return;
/* For non-primary players we swap left/right to match historical layout
           expectations (fixes Q/W inversion reported by users). */
if(player_idx == 0) {
if(left) s_key_left[player_idx]= left;
if(right) s_key_right[player_idx]= right;
} else {
if(right) s_key_left[player_idx]= right;
if(left) s_key_right[player_idx]= left;
}
}
void input_set_bindings_from_config(const GameConfig* cfg) {
if(!cfg) return;
int max_players= game_config_get_max_players(cfg);
if(max_players > SNAKE_MAX_PLAYERS) max_players= SNAKE_MAX_PLAYERS;
/* global controls */
s_key_quit= game_config_get_key_quit(cfg);
s_key_restart= game_config_get_key_restart(cfg);
s_key_pause= game_config_get_key_pause(cfg);
for(int p= 0; p < max_players; ++p) { input_set_player_key_bindings(p, game_config_get_player_key_left(cfg, p), game_config_get_player_key_right(cfg, p)); }
}
static void input_poll_all_from_buf_impl(InputState* outs, int max_players, const unsigned char* buf, size_t n) {
if(!outs || max_players <= 0 || buf == NULL || n == 0) return;
for(int p= 0; p < max_players; ++p) outs[p]= (InputState){0};
for(size_t i= 0; i < n; i++) {
unsigned char c= buf[i];
if(c == '\x1b' && i + 2 < n && buf[i + 1] == '[') {
int code= (int)buf[i + 2];
int dir= parse_arrow_key(code);
/* Only left/right arrows mapped to player 0 turns */
if(dir == 2) {
if(max_players > 0) {
outs[0].turn_right= true;
outs[0].any_key= true;
}
} else if(dir == 3) {
if(max_players > 0) {
outs[0].turn_left= true;
outs[0].any_key= true;
}
}
i+= 2;
continue;
}
if(c == '\r' || c == '\n') continue;
unsigned char lc= ascii_tolower((unsigned char)c);
for(int p= 0; p < max_players; ++p) {
if(s_key_left[p] && lc == ascii_tolower((unsigned char)s_key_left[p]))
outs[p].turn_left= true;
else if(s_key_right[p] && lc == ascii_tolower((unsigned char)s_key_right[p]))
outs[p].turn_right= true;
if(outs[p].turn_left || outs[p].turn_right) outs[p].any_key= true;
}
// Global controls
if(lc == ascii_tolower((unsigned char)s_key_quit)) {
for(int p= 0; p < max_players; ++p) outs[p].quit= true;
} else if(lc == ascii_tolower((unsigned char)s_key_restart)) {
for(int p= 0; p < max_players; ++p) outs[p].restart= true;
} else if(lc == ascii_tolower((unsigned char)s_key_pause)) {
for(int p= 0; p < max_players; ++p) outs[p].pause_toggle= true;
}
}
}
void input_poll_all_from_buf(InputState* outs, int max_players, const unsigned char* buf, size_t n) { input_poll_all_from_buf_impl(outs, max_players, buf, n); }
void input_poll_all(InputState* outs, int max_players) {
unsigned char buf[128];
ssize_t nread= read(STDIN_FILENO, buf, sizeof(buf));
if(nread <= 0) return;
input_poll_all_from_buf_impl(outs, max_players, buf, (size_t)nread);
}
