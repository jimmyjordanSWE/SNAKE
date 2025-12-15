#include "snake/tty.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
static volatile sig_atomic_t winch_received= 0;
static void sigwinch_handler(int sig) {
(void)sig;
winch_received= 1;
}
/* Internal definition of the tty context (kept private to the implementation)
 * This mirrors the previous public definition but hides internals from users.
 */
struct tty_context {
int tty_fd;
struct termios orig_termios;
char tty_path[256];
int width;
int height;
int min_width;
int min_height;
struct ascii_pixel* front;
struct ascii_pixel* back;
bool dirty;
bool size_valid;
char* write_buffer;
size_t write_buffer_size;
volatile sig_atomic_t resized;
void (*on_resize)(struct tty_context* ctx, int old_width, int old_height, int new_width, int new_height, void* userdata);
void (*on_size_invalid)(struct tty_context* ctx, int current_width, int current_height, int min_width, int min_height, void* userdata);
void* callback_userdata;
};
/* Test overrides (used by unit tests to avoid relying on ioctl). */
static bool use_test_size= false;
static int test_width= 0;
static int test_height= 0;
void tty_set_test_size(int width, int height) {
use_test_size= true;
test_width= width;
test_height= height;
}
void tty_clear_test_size(void) { use_test_size= false; }
/* Test helper to simulate SIGWINCH for unit tests. */
void tty_simulate_winch(void) { winch_received= 1; }
static int utf16_to_utf8(uint16_t utf16, char* utf8_out) {
if(utf16 < 0x0080) {
utf8_out[0]= (char)utf16;
return 1;
} else if(utf16 < 0x0800) {
utf8_out[0]= (char)(0xC0 | (utf16 >> 6));
utf8_out[1]= (char)(0x80 | (utf16 & 0x3F));
return 2;
} else {
utf8_out[0]= (char)(0xE0 | (utf16 >> 12));
utf8_out[1]= (char)(0x80 | ((utf16 >> 6) & 0x3F));
utf8_out[2]= (char)(0x80 | (utf16 & 0x3F));
return 3;
}
}
static int utf16_surrogate_pair_to_utf8(uint16_t high, uint16_t low, char* out) {
/* Validate the surrogate pair and convert to codepoint */
if(high < 0xD800 || high > 0xDBFF || low < 0xDC00 || low > 0xDFFF) return 0;
uint32_t high_t= (uint32_t)(high - 0xD800);
uint32_t low_t= (uint32_t)(low - 0xDC00);
uint32_t codepoint= 0x10000u + ((high_t << 10) | low_t);
/* Encode to UTF-8 (4 bytes) */
out[0]= (char)(0xF0 | ((codepoint >> 18) & 0x07));
out[1]= (char)(0x80 | ((codepoint >> 12) & 0x3F));
out[2]= (char)(0x80 | ((codepoint >> 6) & 0x3F));
out[3]= (char)(0x80 | (codepoint & 0x3F));
return 4;
}
static inline bool ascii_pixel_equal(struct ascii_pixel a, struct ascii_pixel b) { return a.pixel == b.pixel && a.color == b.color; }
static bool get_terminal_size(int fd, int* width, int* height) {
if(use_test_size) {
if(width) *width= test_width;
if(height) *height= test_height;
return true;
}
struct winsize ws;
if(ioctl(fd, TIOCGWINSZ, &ws) == -1) return false;
*width= ws.ws_col;
*height= ws.ws_row;
return true;
}
tty_context* tty_open(const char* tty_path, int min_width, int min_height) {
tty_context* ctx= malloc(sizeof(tty_context));
if(!ctx) return NULL;
memset(ctx, 0, sizeof(tty_context));
ctx->min_width= min_width;
ctx->min_height= min_height;
if(tty_path) {
strncpy(ctx->tty_path, tty_path, sizeof(ctx->tty_path) - 1);
ctx->tty_fd= open(tty_path, O_RDWR);
} else {
ctx->tty_fd= dup(STDOUT_FILENO);
strcpy(ctx->tty_path, "/dev/stdout");
}
if(ctx->tty_fd == -1) {
free(ctx);
return NULL;
}
int actual_width= 0, actual_height= 0;
if(!get_terminal_size(ctx->tty_fd, &actual_width, &actual_height)) {
close(ctx->tty_fd);
free(ctx);
return NULL;
}
ctx->width= actual_width;
ctx->height= actual_height;
if((min_width > 0 && actual_width < min_width) || (min_height > 0 && actual_height < min_height))
ctx->size_valid= false;
else
ctx->size_valid= true;
if(tcgetattr(ctx->tty_fd, &ctx->orig_termios) == -1) {
close(ctx->tty_fd);
free(ctx);
return NULL;
}
struct termios raw= ctx->orig_termios;
raw.c_iflag&= (tcflag_t)(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
raw.c_oflag&= (tcflag_t)(~(OPOST));
raw.c_cflag|= (CS8);
raw.c_lflag&= (tcflag_t)(~(ECHO | ICANON | IEXTEN | ISIG));
raw.c_cc[VMIN]= 0;
raw.c_cc[VTIME]= 0;
if(tcsetattr(ctx->tty_fd, TCSADRAIN, &raw) == -1) {
close(ctx->tty_fd);
free(ctx);
return NULL;
}
size_t buffer_size= (size_t)actual_width * (size_t)actual_height * sizeof(struct ascii_pixel);
ctx->front= malloc(buffer_size);
ctx->back= malloc(buffer_size);
ctx->write_buffer_size= (size_t)actual_width * (size_t)actual_height * 32;
ctx->write_buffer= malloc(ctx->write_buffer_size);
if(!ctx->front || !ctx->back || !ctx->write_buffer) {
free(ctx->front);
free(ctx->back);
free(ctx->write_buffer);
tcsetattr(ctx->tty_fd, TCSADRAIN, &ctx->orig_termios);
close(ctx->tty_fd);
free(ctx);
return NULL;
}
struct ascii_pixel default_pixel= PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
for(int i= 0; i < actual_width * actual_height; i++) {
ctx->front[i]= default_pixel;
ctx->back[i]= default_pixel;
}
dprintf(ctx->tty_fd, "\x1b[2J");
dprintf(ctx->tty_fd, "\x1b[?25l");
signal(SIGWINCH, sigwinch_handler);
ctx->dirty= true;
return ctx;
}
void tty_close(tty_context* ctx) {
if(!ctx) return;
dprintf(ctx->tty_fd, "\x1b[0m");
dprintf(ctx->tty_fd, "\x1b[?25h");
dprintf(ctx->tty_fd, "\x1b[2J");
dprintf(ctx->tty_fd, "\x1b[H");
tcsetattr(ctx->tty_fd, TCSADRAIN, &ctx->orig_termios);
free(ctx->front);
free(ctx->back);
free(ctx->write_buffer);
close(ctx->tty_fd);
free(ctx);
}
struct ascii_pixel* tty_get_buffer(tty_context* ctx) {
if(!ctx) return NULL;
return ctx->back;
}
void tty_put_pixel(tty_context* ctx, int x, int y, struct ascii_pixel px) {
if(!ctx || x < 0 || y < 0 || x >= ctx->width || y >= ctx->height) return;
ctx->back[y * ctx->width + x]= px;
ctx->dirty= true;
}
struct ascii_pixel tty_get_pixel(tty_context* ctx, int x, int y) {
static struct ascii_pixel zero= {0, 0};
if(!ctx || x < 0 || y < 0 || x >= ctx->width || y >= ctx->height) return zero;
return ctx->back[y * ctx->width + x];
}
void tty_clear_back(tty_context* ctx) {
if(!ctx) return;
struct ascii_pixel default_pixel= PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
for(int i= 0; i < ctx->width * ctx->height; i++) ctx->back[i]= default_pixel;
ctx->dirty= true;
}
void tty_clear_front(tty_context* ctx) {
if(!ctx) return;
struct ascii_pixel default_pixel= PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
for(int i= 0; i < ctx->width * ctx->height; i++) ctx->front[i]= default_pixel;
}
void tty_flip(tty_context* ctx) {
if(!ctx || !ctx->dirty) return;
char* buf= ctx->write_buffer;
size_t pos= 0;
size_t remaining= ctx->write_buffer_size;
int current_fg= -1, current_bg= -1;
for(int y= 0; y < ctx->height; y++) {
for(int x= 0; x < ctx->width; x++) {
if(ascii_pixel_equal(ctx->front[y * ctx->width + x], ctx->back[y * ctx->width + x])) continue;
int span_start= x;
uint16_t span_color= ctx->back[y * ctx->width + x].color;
while(x < ctx->width && !ascii_pixel_equal(ctx->front[y * ctx->width + x], ctx->back[y * ctx->width + x]) && ctx->back[y * ctx->width + x].color == span_color) x++;
int span_end= x;
int n= snprintf(buf + pos, remaining, "\x1b[%d;%dH", y + 1, span_start + 1);
if(n < 0 || (size_t)n >= remaining) return;
pos+= (size_t)n;
remaining-= (size_t)n;
uint8_t fg= COLOR_FG(span_color);
uint8_t bg= COLOR_BG(span_color);
if(fg != (uint8_t)current_fg || bg != (uint8_t)current_bg) {
current_fg= (int)fg;
current_bg= (int)bg;
if(fg < 8)
n= snprintf(buf + pos, remaining, "\x1b[3%um", (unsigned int)fg);
else
n= snprintf(buf + pos, remaining, "\x1b[9%um", (unsigned int)(fg - 8));
if(n < 0 || (size_t)n >= remaining) return;
pos+= (size_t)n;
remaining-= (size_t)n;
if(bg < 8)
n= snprintf(buf + pos, remaining, "\x1b[4%um", (unsigned int)bg);
else
n= snprintf(buf + pos, remaining, "\x1b[10%um", (unsigned int)(bg - 8));
if(n < 0 || (size_t)n >= remaining) return;
pos+= (size_t)n;
remaining-= (size_t)n;
}
for(int i= span_start; i < span_end; i++) {
uint16_t ch= ctx->back[y * ctx->width + i].pixel;
char utf8[4];
int utf8_len= 0;
/* Handle UTF-16 surrogate pairs if a high surrogate is
                 * followed by a low surrogate in the buffer. In that case
                 * emit a 4-byte UTF-8 sequence and skip the low surrogate.
                 */
if(ch >= 0xD800 && ch <= 0xDBFF && i + 1 < span_end) {
uint16_t ch2= ctx->back[y * ctx->width + (i + 1)].pixel;
if(ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
utf8_len= utf16_surrogate_pair_to_utf8(ch, ch2, utf8);
i++; /* skip low surrogate (consumed) */
}
}
if(utf8_len == 0) { utf8_len= utf16_to_utf8(ch, utf8); }
if((size_t)utf8_len > remaining) return;
memcpy(buf + pos, utf8, (size_t)utf8_len);
pos+= (size_t)utf8_len;
remaining-= (size_t)utf8_len;
}
for(int i= span_start; i < span_end; i++) ctx->front[y * ctx->width + i]= ctx->back[y * ctx->width + i];
x= span_end - 1;
}
}
ssize_t written= write(ctx->tty_fd, buf, pos);
(void)written;
ctx->dirty= false;
}
void tty_force_redraw(tty_context* ctx) {
if(!ctx) return;
memset(ctx->front, 0, (size_t)ctx->width * (size_t)ctx->height * sizeof(struct ascii_pixel));
ctx->dirty= true;
tty_flip(ctx);
}
void tty_get_size(tty_context* ctx, int* width, int* height) {
if(!ctx) return;
if(width) *width= ctx->width;
if(height) *height= ctx->height;
}
bool tty_check_resize(tty_context* ctx) {
if(!ctx || !winch_received) return false;
winch_received= 0;
int new_width= 0, new_height= 0;
if(!get_terminal_size(ctx->tty_fd, &new_width, &new_height)) return false;
if(new_width == ctx->width && new_height == ctx->height) return false;
int old_width= ctx->width;
int old_height= ctx->height;
size_t new_size= (size_t)new_width * (size_t)new_height * sizeof(struct ascii_pixel);
struct ascii_pixel* new_front= malloc(new_size);
struct ascii_pixel* new_back= malloc(new_size);
size_t new_write_buffer_size= (size_t)new_width * (size_t)new_height * 32;
char* new_write_buffer= malloc(new_write_buffer_size);
if(!new_front || !new_back || !new_write_buffer) {
free(new_front);
free(new_back);
free(new_write_buffer);
return false;
}
struct ascii_pixel default_pixel= PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
for(int i= 0; i < new_width * new_height; i++) {
new_front[i]= default_pixel;
new_back[i]= default_pixel;
}
int copy_width= (new_width < old_width) ? new_width : old_width;
int copy_height= (new_height < old_height) ? new_height : old_height;
for(int y= 0; y < copy_height; y++) {
for(int x= 0; x < copy_width; x++) {
new_front[y * new_width + x]= ctx->front[y * old_width + x];
new_back[y * new_width + x]= ctx->back[y * old_width + x];
}
}
free(ctx->front);
free(ctx->back);
free(ctx->write_buffer);
ctx->front= new_front;
ctx->back= new_back;
ctx->width= new_width;
ctx->height= new_height;
ctx->write_buffer= new_write_buffer;
ctx->write_buffer_size= new_write_buffer_size;
bool old_size_valid= ctx->size_valid;
if((ctx->min_width > 0 && new_width < ctx->min_width) || (ctx->min_height > 0 && new_height < ctx->min_height))
ctx->size_valid= false;
else
ctx->size_valid= true;
if(ctx->on_resize) ctx->on_resize(ctx, old_width, old_height, new_width, new_height, ctx->callback_userdata);
if(!ctx->size_valid && old_size_valid && ctx->on_size_invalid) ctx->on_size_invalid(ctx, new_width, new_height, ctx->min_width, ctx->min_height, ctx->callback_userdata);
ctx->dirty= true;
return true;
}
void tty_set_resize_callback(tty_context* ctx, void (*callback)(tty_context*, int, int, int, int, void*), void* userdata) {
if(ctx) {
ctx->on_resize= callback;
ctx->callback_userdata= userdata;
}
}
void tty_set_size_invalid_callback(tty_context* ctx, void (*callback)(tty_context*, int, int, int, int, void*), void* userdata) {
if(ctx) {
ctx->on_size_invalid= callback;
ctx->callback_userdata= userdata;
}
}
bool tty_size_valid(tty_context* ctx) {
if(!ctx) return false;
return ctx->size_valid;
}
void tty_get_min_size(tty_context* ctx, int* min_width, int* min_height) {
if(!ctx) return;
if(min_width) *min_width= ctx->min_width;
if(min_height) *min_height= ctx->min_height;
}
/* Compute minimum render size for a board (matches previous logic from app)
 * Minimums are: field = board + 2; board+2 + 2 padding => board + 4
 * The external UI prefers slightly larger render area for headers, etc.
 */
void tty_get_board_min_size(int board_width, int board_height, int* min_width, int* min_height) {
if(min_width) *min_width= board_width + 4;
if(min_height) *min_height= board_height + 4;
}
bool tty_size_sufficient_for_board(int term_width, int term_height, int board_width, int board_height) {
const int field_width= board_width + 2;
const int field_height= board_height + 2;
int min_h= field_height + 2;
int min_w= field_width + 2;
(void)board_width;
(void)board_height; /* silence unused in some build configs */
return term_width >= min_w && term_height >= min_h;
}
