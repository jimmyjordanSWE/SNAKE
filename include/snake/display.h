#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct DisplayContext DisplayContext;
#define DISPLAY_COLOR_BLACK 0
#define DISPLAY_COLOR_RED 1
#define DISPLAY_COLOR_GREEN 2
#define DISPLAY_COLOR_YELLOW 3
#define DISPLAY_COLOR_BLUE 4
#define DISPLAY_COLOR_MAGENTA 5
#define DISPLAY_COLOR_CYAN 6
#define DISPLAY_COLOR_WHITE 7
#define DISPLAY_COLOR_BRIGHT_BLACK 8
#define DISPLAY_COLOR_BRIGHT_RED 9
#define DISPLAY_COLOR_BRIGHT_GREEN 10
#define DISPLAY_COLOR_BRIGHT_YELLOW 11
#define DISPLAY_COLOR_BRIGHT_BLUE 12
#define DISPLAY_COLOR_BRIGHT_MAGENTA 13
#define DISPLAY_COLOR_BRIGHT_CYAN 14
#define DISPLAY_COLOR_BRIGHT_WHITE 15
#define DISPLAY_CHAR_BLOCK 0x2588
#define DISPLAY_CHAR_BOX_H 0x2500
#define DISPLAY_CHAR_BOX_V 0x2502
#define DISPLAY_CHAR_BOX_TL 0x250C
#define DISPLAY_CHAR_BOX_TR 0x2510
#define DISPLAY_CHAR_BOX_BL 0x2514
#define DISPLAY_CHAR_BOX_BR 0x2518
#define DISPLAY_CHAR_CIRCLE 0x25CF
DisplayContext* display_init(int min_width, int min_height);
void display_shutdown(DisplayContext* ctx);
void display_clear(DisplayContext* ctx);
void display_put_char(DisplayContext* ctx, int x, int y, uint16_t ch, uint16_t fg, uint16_t bg);
void display_fill_rect(DisplayContext* ctx, int x, int y, int w, int h, uint16_t ch, uint16_t fg, uint16_t bg);
void display_flush(DisplayContext* ctx);
void display_get_size(const DisplayContext* ctx, int* width, int* height);
bool display_size_valid(const DisplayContext* ctx);
void display_put_string(DisplayContext* ctx, int x, int y, const char* str, uint16_t fg, uint16_t bg);
void display_draw_line_h(DisplayContext* ctx, int x, int y, int len, uint16_t ch, uint16_t fg, uint16_t bg);
void display_draw_line_v(DisplayContext* ctx, int x, int y, int len, uint16_t ch, uint16_t fg, uint16_t bg);
void display_put_hline(DisplayContext* ctx, int x, int y, int len, uint16_t ch, uint16_t fg, uint16_t bg);
void display_put_vline(DisplayContext* ctx, int x, int y, int len, uint16_t ch, uint16_t fg, uint16_t bg);
void display_present(DisplayContext* ctx);
void display_force_redraw(DisplayContext* ctx);

/* Testing helper: return pointer to the back buffer (array of struct ascii_pixel) or NULL. */
struct ascii_pixel* display_get_back_buffer(DisplayContext* ctx);
