#include "display.h"
#include "tty.h"
#include <stdlib.h>
#include <string.h>
struct DisplayContext {
    tty_context* tty;
};
static inline uint16_t map_color(uint16_t display_color) {
    return display_color;
}
DisplayContext* display_init(int min_width, int min_height) {
    DisplayContext* ctx = malloc(sizeof *ctx);
    if (!ctx)
        return NULL;
    ctx->tty = tty_open(NULL, min_width, min_height);
    if (!ctx->tty) {
        free(ctx);
        return NULL;
    }
    if (!tty_size_valid(ctx->tty)) {
        tty_close(ctx->tty);
        free(ctx);
        return NULL;
    }
    return ctx;
}
void display_shutdown(DisplayContext* ctx) {
    if (!ctx)
        return;
    if (ctx->tty)
        tty_close(ctx->tty);
    free(ctx);
}
void display_get_size(const DisplayContext* ctx, int* width, int* height) {
    if (!ctx || !ctx->tty) {
        if (width)
            *width = 0;
        if (height)
            *height = 0;
        return;
    }
    tty_get_size(ctx->tty, width, height);
}
bool display_size_valid(const DisplayContext* ctx) {
    if (!ctx || !ctx->tty)
        return false;
    return tty_size_valid(ctx->tty);
}
void display_clear(DisplayContext* ctx) {
    if (!ctx || !ctx->tty)
        return;
    tty_clear_back(ctx->tty);
    int width = 0, height = 0;
    tty_get_size(ctx->tty, &width, &height);
    struct ascii_pixel black = PIXEL_MAKE(' ', COLOR_BLACK, COLOR_BLACK);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            tty_put_pixel(ctx->tty, x, y, black);
}
void display_put_char(DisplayContext* ctx, int x, int y, uint16_t ch, uint16_t fg, uint16_t bg) {
    if (!ctx || !ctx->tty)
        return;
    struct ascii_pixel pixel = PIXEL_MAKE(ch, map_color(fg), map_color(bg));
    tty_put_pixel(ctx->tty, x, y, pixel);
}
void display_put_string(DisplayContext* ctx, int x, int y, const char* str, uint16_t fg, uint16_t bg) {
    if (!ctx || !ctx->tty || !str)
        return;
    int width = 0, height = 0;
    tty_get_size(ctx->tty, &width, &height);
    for (int i = 0; str[i] && x + i < width && y >= 0 && y < height; i++)
        display_put_char(ctx, x + i, y, (uint16_t)str[i], fg, bg);
}
void display_put_hline(DisplayContext* ctx, int x, int y, int len, uint16_t ch, uint16_t fg, uint16_t bg) {
    if (!ctx || !ctx->tty || len <= 0)
        return;
    int width = 0, height = 0;
    tty_get_size(ctx->tty, &width, &height);
    for (int i = 0; i < len && x + i < width; i++)
        display_put_char(ctx, x + i, y, ch, fg, bg);
}
void display_put_vline(DisplayContext* ctx, int x, int y, int len, uint16_t ch, uint16_t fg, uint16_t bg) {
    if (!ctx || !ctx->tty || len <= 0)
        return;
    int width = 0, height = 0;
    tty_get_size(ctx->tty, &width, &height);
    for (int i = 0; i < len && y + i < height; i++)
        display_put_char(ctx, x, y + i, ch, fg, bg);
}
void display_present(DisplayContext* ctx) {
    if (!ctx || !ctx->tty)
        return;
    tty_flip(ctx->tty);
}
void display_force_redraw(DisplayContext* ctx) {
    if (!ctx || !ctx->tty)
        return;
    tty_force_redraw(ctx->tty);
}
struct ascii_pixel* display_get_back_buffer(DisplayContext* ctx) {
    if (!ctx || !ctx->tty)
        return NULL;
    return tty_get_buffer(ctx->tty);
}
