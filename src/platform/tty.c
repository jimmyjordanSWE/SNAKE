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

/* Global signal handler for SIGWINCH */
static volatile sig_atomic_t winch_received = 0;

static void sigwinch_handler(int sig) {
    (void)sig;
    winch_received = 1;
}

/* UTF-16 BMP to UTF-8 conversion */
static int utf16_to_utf8(uint16_t utf16, char* utf8_out) {
    if (utf16 < 0x0080) {
        utf8_out[0] = (char)utf16;
        return 1;
    } else if (utf16 < 0x0800) {
        utf8_out[0] = (char)(0xC0 | (utf16 >> 6));
        utf8_out[1] = (char)(0x80 | (utf16 & 0x3F));
        return 2;
    } else {
        utf8_out[0] = (char)(0xE0 | (utf16 >> 12));
        utf8_out[1] = (char)(0x80 | ((utf16 >> 6) & 0x3F));
        utf8_out[2] = (char)(0x80 | (utf16 & 0x3F));
        return 3;
    }
}

/* Get terminal size */
static bool get_terminal_size(int fd, int* width, int* height) {
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) == -1) { return false; }
    *width = ws.ws_col;
    *height = ws.ws_row;
    return true;
}

/* Open TTY for graphics */
tty_context* tty_open(const char* tty_path, int min_width, int min_height) {
    tty_context* ctx = malloc(sizeof(tty_context));
    if (!ctx) { return NULL; }

    memset(ctx, 0, sizeof(tty_context));
    ctx->min_width = min_width;
    ctx->min_height = min_height;

    /* Open TTY device */
    if (tty_path) {
        strncpy(ctx->tty_path, tty_path, sizeof(ctx->tty_path) - 1);
        ctx->tty_fd = open(tty_path, O_RDWR);
    } else {
        /* Use standard input/output */
        ctx->tty_fd = dup(STDOUT_FILENO);
        strcpy(ctx->tty_path, "/dev/stdout");
    }

    if (ctx->tty_fd == -1) {
        free(ctx);
        return NULL;
    }

    /* Get terminal size */
    int actual_width = 0, actual_height = 0;
    if (!get_terminal_size(ctx->tty_fd, &actual_width, &actual_height)) {
        close(ctx->tty_fd);
        free(ctx);
        return NULL;
    }

    ctx->width = actual_width;
    ctx->height = actual_height;

    /* Check minimum size */
    if ((min_width > 0 && actual_width < min_width) || (min_height > 0 && actual_height < min_height)) {
        ctx->size_valid = false;
    } else {
        ctx->size_valid = true;
    }

    /* Save original termios */
    if (tcgetattr(ctx->tty_fd, &ctx->orig_termios) == -1) {
        close(ctx->tty_fd);
        free(ctx);
        return NULL;
    }

    /* Configure raw mode */
    struct termios raw = ctx->orig_termios;
    raw.c_iflag &= (tcflag_t)(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
    raw.c_oflag &= (tcflag_t)(~(OPOST));
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (tcflag_t)(~(ECHO | ICANON | IEXTEN | ISIG));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(ctx->tty_fd, TCSADRAIN, &raw) == -1) {
        close(ctx->tty_fd);
        free(ctx);
        return NULL;
    }

    /* Allocate buffers */
    size_t buffer_size = (size_t)actual_width * (size_t)actual_height * sizeof(struct ascii_pixel);
    ctx->front = malloc(buffer_size);
    ctx->back = malloc(buffer_size);
    ctx->write_buffer_size = (size_t)actual_width * (size_t)actual_height * 32;
    ctx->write_buffer = malloc(ctx->write_buffer_size);

    if (!ctx->front || !ctx->back || !ctx->write_buffer) {
        free(ctx->front);
        free(ctx->back);
        free(ctx->write_buffer);
        tcsetattr(ctx->tty_fd, TCSADRAIN, &ctx->orig_termios);
        close(ctx->tty_fd);
        free(ctx);
        return NULL;
    }

    /* Initialize buffers to default state */
    struct ascii_pixel default_pixel = PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    for (int i = 0; i < actual_width * actual_height; i++) {
        ctx->front[i] = default_pixel;
        ctx->back[i] = default_pixel;
    }

    /* Clear screen and hide cursor */
    dprintf(ctx->tty_fd, "\x1b[2J");   /* Clear screen */
    dprintf(ctx->tty_fd, "\x1b[?25l"); /* Hide cursor */

    /* Install SIGWINCH handler */
    signal(SIGWINCH, sigwinch_handler);

    ctx->dirty = true;
    return ctx;
}

/* Close TTY */
void tty_close(tty_context* ctx) {
    if (!ctx) { return; }

    /* Show cursor */
    dprintf(ctx->tty_fd, "\x1b[?25h");
    /* Clear screen */
    dprintf(ctx->tty_fd, "\x1b[2J");

    /* Restore original termios */
    tcsetattr(ctx->tty_fd, TCSADRAIN, &ctx->orig_termios);

    /* Free resources */
    free(ctx->front);
    free(ctx->back);
    free(ctx->write_buffer);
    close(ctx->tty_fd);
    free(ctx);
}

/* Get back buffer */
struct ascii_pixel* tty_get_buffer(tty_context* ctx) {
    if (!ctx) { return NULL; }
    return ctx->back;
}

/* Put pixel with bounds checking */
void tty_put_pixel(tty_context* ctx, int x, int y, struct ascii_pixel px) {
    if (!ctx || x < 0 || y < 0 || x >= ctx->width || y >= ctx->height) { return; }
    ctx->back[y * ctx->width + x] = px;
    ctx->dirty = true;
}

/* Get pixel with bounds checking */
struct ascii_pixel tty_get_pixel(tty_context* ctx, int x, int y) {
    static struct ascii_pixel zero = {0, 0};
    if (!ctx || x < 0 || y < 0 || x >= ctx->width || y >= ctx->height) { return zero; }
    return ctx->back[y * ctx->width + x];
}

/* Clear back buffer */
void tty_clear_back(tty_context* ctx) {
    if (!ctx) { return; }
    struct ascii_pixel default_pixel = PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    for (int i = 0; i < ctx->width * ctx->height; i++) { ctx->back[i] = default_pixel; }
    ctx->dirty = true;
}

/* Clear front buffer */
void tty_clear_front(tty_context* ctx) {
    if (!ctx) { return; }
    struct ascii_pixel default_pixel = PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    for (int i = 0; i < ctx->width * ctx->height; i++) { ctx->front[i] = default_pixel; }
}

/* Flip with diff-based rendering */
void tty_flip(tty_context* ctx) {
    if (!ctx || !ctx->dirty) { return; }

    char* buf = ctx->write_buffer;
    size_t pos = 0;
    size_t remaining = ctx->write_buffer_size;

    int current_fg = -1, current_bg = -1;

    for (int y = 0; y < ctx->height; y++) {
        for (int x = 0; x < ctx->width; x++) {
            /* Always check each rendered pixel, not just game board pixels */
            if (*(uint32_t*)&ctx->front[y * ctx->width + x] == *(uint32_t*)&ctx->back[y * ctx->width + x]) { continue; }

            /* Found a change, find a span of changed pixels with the same color */
            int span_start = x;
            uint16_t span_color = ctx->back[y * ctx->width + x].color;

            while (x < ctx->width && *(uint32_t*)&ctx->front[y * ctx->width + x] != *(uint32_t*)&ctx->back[y * ctx->width + x] &&
                   ctx->back[y * ctx->width + x].color == span_color) {
                x++;
            }
            int span_end = x;

            /* Cursor positioning */
            int n = snprintf(buf + pos, remaining, "\x1b[%d;%dH", y + 1, span_start + 1);
            if (n < 0 || (size_t)n >= remaining) { return; }
            pos += (size_t)n;
            remaining -= (size_t)n;

            /* Color change */
            uint8_t fg = COLOR_FG(span_color);
            uint8_t bg = COLOR_BG(span_color);

            if (fg != (uint8_t)current_fg || bg != (uint8_t)current_bg) {
                current_fg = (int)fg;
                current_bg = (int)bg;

                if (fg < 8) {
                    n = snprintf(buf + pos, remaining, "\x1b[3%um", (unsigned int)fg);
                } else {
                    n = snprintf(buf + pos, remaining, "\x1b[9%um", (unsigned int)(fg - 8));
                }
                if (n < 0 || (size_t)n >= remaining) { return; }
                pos += (size_t)n;
                remaining -= (size_t)n;

                if (bg < 8) {
                    n = snprintf(buf + pos, remaining, "\x1b[4%um", (unsigned int)bg);
                } else {
                    n = snprintf(buf + pos, remaining, "\x1b[10%um", (unsigned int)(bg - 8));
                }
                if (n < 0 || (size_t)n >= remaining) { return; }
                pos += (size_t)n;
                remaining -= (size_t)n;
            }

            /* Emit characters */
            for (int i = span_start; i < span_end; i++) {
                uint16_t ch = ctx->back[y * ctx->width + i].pixel;
                char utf8[4];
                int utf8_len = utf16_to_utf8(ch, utf8);
                if ((size_t)utf8_len > remaining) { return; }
                memcpy(buf + pos, utf8, (size_t)utf8_len);
                pos += (size_t)utf8_len;
                remaining -= (size_t)utf8_len;
            }

            /* Update front buffer */
            for (int i = span_start; i < span_end; i++) { ctx->front[y * ctx->width + i] = ctx->back[y * ctx->width + i]; }
        }
    }

    /* Write to TTY */
    ssize_t written = write(ctx->tty_fd, buf, pos);
    (void)written;

    ctx->dirty = false;
}

/* Force redraw */
void tty_force_redraw(tty_context* ctx) {
    if (!ctx) { return; }
    /* Ensure every cell is treated as changed by making front differ from back. */
    memset(ctx->front, 0, (size_t)ctx->width * (size_t)ctx->height * sizeof(struct ascii_pixel));
    ctx->dirty = true;
    tty_flip(ctx);
}

/* Get size */
void tty_get_size(tty_context* ctx, int* width, int* height) {
    if (!ctx) { return; }
    if (width) { *width = ctx->width; }
    if (height) { *height = ctx->height; }
}

/* Check resize */
bool tty_check_resize(tty_context* ctx) {
    if (!ctx || !winch_received) { return false; }

    winch_received = 0;

    int new_width = 0, new_height = 0;
    if (!get_terminal_size(ctx->tty_fd, &new_width, &new_height)) { return false; }

    if (new_width == ctx->width && new_height == ctx->height) { return false; }

    int old_width = ctx->width;
    int old_height = ctx->height;

    /* Allocate new buffers */
    size_t new_size = (size_t)new_width * (size_t)new_height * sizeof(struct ascii_pixel);
    struct ascii_pixel* new_front = malloc(new_size);
    struct ascii_pixel* new_back = malloc(new_size);

    size_t new_write_buffer_size = (size_t)new_width * (size_t)new_height * 32;
    char* new_write_buffer = malloc(new_write_buffer_size);

    if (!new_front || !new_back || !new_write_buffer) {
        free(new_front);
        free(new_back);
        free(new_write_buffer);
        return false;
    }

    /* Initialize with default pixels */
    struct ascii_pixel default_pixel = PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    for (int i = 0; i < new_width * new_height; i++) {
        new_front[i] = default_pixel;
        new_back[i] = default_pixel;
    }

    /* Copy old content (clipped) */
    int copy_width = (new_width < old_width) ? new_width : old_width;
    int copy_height = (new_height < old_height) ? new_height : old_height;

    for (int y = 0; y < copy_height; y++) {
        for (int x = 0; x < copy_width; x++) {
            new_front[y * new_width + x] = ctx->front[y * old_width + x];
            new_back[y * new_width + x] = ctx->back[y * old_width + x];
        }
    }

    /* Free old buffers */
    free(ctx->front);
    free(ctx->back);
    free(ctx->write_buffer);

    /* Update context */
    ctx->front = new_front;
    ctx->back = new_back;
    ctx->width = new_width;
    ctx->height = new_height;

    ctx->write_buffer = new_write_buffer;
    ctx->write_buffer_size = new_write_buffer_size;

    /* Check size validity */
    bool old_size_valid = ctx->size_valid;
    if ((ctx->min_width > 0 && new_width < ctx->min_width) || (ctx->min_height > 0 && new_height < ctx->min_height)) {
        ctx->size_valid = false;
    } else {
        ctx->size_valid = true;
    }

    /* Call resize callback */
    if (ctx->on_resize) { ctx->on_resize(ctx, old_width, old_height, new_width, new_height, ctx->callback_userdata); }

    /* Call size invalid callback if needed */
    if (!ctx->size_valid && old_size_valid && ctx->on_size_invalid) {
        ctx->on_size_invalid(ctx, new_width, new_height, ctx->min_width, ctx->min_height, ctx->callback_userdata);
    }

    ctx->dirty = true;
    return true;
}

/* Set resize callback */
void tty_set_resize_callback(tty_context* ctx, void (*callback)(tty_context*, int, int, int, int, void*), void* userdata) {
    if (ctx) {
        ctx->on_resize = callback;
        ctx->callback_userdata = userdata;
    }
}

/* Set size invalid callback */
void tty_set_size_invalid_callback(tty_context* ctx, void (*callback)(tty_context*, int, int, int, int, void*), void* userdata) {
    if (ctx) {
        ctx->on_size_invalid = callback;
        ctx->callback_userdata = userdata;
    }
}

/* Check size validity */
bool tty_size_valid(tty_context* ctx) {
    if (!ctx) { return false; }
    return ctx->size_valid;
}

/* Get min size */
void tty_get_min_size(tty_context* ctx, int* min_width, int* min_height) {
    if (!ctx) { return; }
    if (min_width) { *min_width = ctx->min_width; }
    if (min_height) { *min_height = ctx->min_height; }
}
