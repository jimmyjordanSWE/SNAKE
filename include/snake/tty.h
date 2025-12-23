#pragma once
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <termios.h>
struct ascii_pixel {
uint16_t pixel;
uint16_t color;
} __attribute__((packed));
typedef struct tty_context tty_context;
tty_context* tty_open(const char* tty_path, int min_width, int min_height);
void tty_close(tty_context* ctx);
struct ascii_pixel* tty_get_buffer(tty_context* ctx);
void tty_put_pixel(tty_context* ctx, int x, int y, struct ascii_pixel px);
struct ascii_pixel tty_get_pixel(tty_context* ctx, int x, int y);
void tty_flip(tty_context* ctx);
void tty_force_redraw(tty_context* ctx);
void tty_clear_back(tty_context* ctx);
void tty_clear_front(tty_context* ctx);
void tty_get_size(const tty_context* ctx, int* width, int* height);
bool tty_check_resize(tty_context* ctx);
void tty_set_resize_callback(tty_context* ctx, void (*callback)(tty_context*, int, int, int, int, void*), void* userdata);
void tty_set_size_invalid_callback(tty_context* ctx, void (*callback)(tty_context*, int, int, int, int, void*), void* userdata);
bool tty_size_valid(const tty_context* ctx);
void tty_get_min_size(const tty_context* ctx, int* min_width, int* min_height);
void tty_get_board_min_size(int board_width, int board_height, int* min_width, int* min_height);
bool tty_size_sufficient_for_board(int term_width, int term_height, int board_width, int board_height);
void tty_simulate_winch(void);
void tty_set_test_size(int width, int height);
void tty_clear_test_size(void);
#define COLOR_MAKE(fg, bg) ((uint16_t)(((bg) << 4) | (fg)))
#define COLOR_FG(c) ((uint8_t)((c) & 0x0F))
#define COLOR_BG(c) ((uint8_t)(((c) >> 4) & 0x0F))
#define COLOR_BLACK 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6
#define COLOR_WHITE 7
#define COLOR_BRIGHT_BLACK 8
#define COLOR_BRIGHT_RED 9
#define COLOR_BRIGHT_GREEN 10
#define COLOR_BRIGHT_YELLOW 11
#define COLOR_BRIGHT_BLUE 12
#define COLOR_BRIGHT_MAGENTA 13
#define COLOR_BRIGHT_CYAN 14
#define COLOR_BRIGHT_WHITE 15
#define COLOR_DEFAULT_FG COLOR_WHITE
#define COLOR_DEFAULT_BG COLOR_BLACK
#define COLOR_DEFAULT COLOR_MAKE(COLOR_DEFAULT_FG, COLOR_DEFAULT_BG)
#define PIXEL_CHAR(ch) ((uint16_t)(ch))
#define PIXEL_MAKE(ch, fg, bg) ((struct ascii_pixel){.pixel= PIXEL_CHAR(ch), .color= COLOR_MAKE(fg, bg)})
#define PIXEL_SPACE 0x0020
#define PIXEL_BLOCK 0x2588
#define PIXEL_SHADE_L 0x2591
#define PIXEL_SHADE_M 0x2592
#define PIXEL_SHADE_D 0x2593
#define PIXEL_BOX_H 0x2500
#define PIXEL_BOX_V 0x2502
#define PIXEL_BOX_TL 0x250C
#define PIXEL_BOX_TR 0x2510
#define PIXEL_BOX_BL 0x2514
#define PIXEL_BOX_BR 0x2518
