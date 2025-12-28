#include "tty.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
/* Cap write buffer allocations to a reasonable value (10 MiB) */
#define TTY_WRITE_BUFFER_CAP ((size_t)10 * 1024 * 1024)
static volatile sig_atomic_t winch_received = 0;
static void sigwinch_handler(int sig) {
    (void)sig;
    winch_received = 1;
}
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
    void (*on_resize)(struct tty_context* ctx,
                      int old_width,
                      int old_height,
                      int new_width,
                      int new_height,
                      void* userdata);
    void (*on_size_invalid)(struct tty_context* ctx,
                            int current_width,
                            int current_height,
                            int min_width,
                            int min_height,
                            void* userdata);
    void* callback_userdata;
};
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
static int utf16_surrogate_pair_to_utf8(uint16_t high, uint16_t low, char* out) {
    if (high < 0xD800 || high > 0xDBFF || low < 0xDC00 || low > 0xDFFF)
        return 0;
    uint32_t high_t = (uint32_t)(high - 0xD800);
    uint32_t low_t = (uint32_t)(low - 0xDC00);
    uint32_t codepoint = 0x10000u + ((high_t << 10) | low_t);
    out[0] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
    out[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
    out[3] = (char)(0x80 | (codepoint & 0x3F));
    return 4;
}
static inline bool ascii_pixel_equal(struct ascii_pixel a, struct ascii_pixel b) {
    return a.pixel == b.pixel && a.color == b.color;
}
/* Small, fast buffer append helpers to avoid snprintf overhead in hot path */
static inline int buf_safe_append_char(char* buf, size_t* pos, size_t* remaining, char c) {
    if (*remaining == 0)
        return -1;
    buf[*pos] = c;
    (*pos)++;
    (*remaining)--;
    return 0;
}
static int buf_append_uint(char* buf, size_t* pos, size_t* remaining, unsigned int v) {
    char tmp[16];
    int len = 0;
    if (v == 0) {
        tmp[len++] = '0';
    } else {
        unsigned int t = v;
        while (t) {
            tmp[len++] = (char)('0' + (t % 10));
            t /= 10;
        }
        /* reverse */
        for (int i = 0; i < len / 2; i++) {
            char c = tmp[i];
            tmp[i] = tmp[len - 1 - i];
            tmp[len - 1 - i] = c;
        }
    }
    if (*remaining < (size_t)len)
        return -1;
    memcpy(buf + *pos, tmp, (size_t)len);
    *pos += (size_t)len;
    *remaining -= (size_t)len;
    return len;
}
static const char* const ANSI_FG[16] = {"\x1b[30m", "\x1b[31m", "\x1b[32m", "\x1b[33m", "\x1b[34m", "\x1b[35m",
                                        "\x1b[36m", "\x1b[37m", "\x1b[90m", "\x1b[91m", "\x1b[92m", "\x1b[93m",
                                        "\x1b[94m", "\x1b[95m", "\x1b[96m", "\x1b[97m"};
static const char* const ANSI_BG[16] = {"\x1b[40m",  "\x1b[41m",  "\x1b[42m",  "\x1b[43m",  "\x1b[44m",  "\x1b[45m",
                                        "\x1b[46m",  "\x1b[47m",  "\x1b[100m", "\x1b[101m", "\x1b[102m", "\x1b[103m",
                                        "\x1b[104m", "\x1b[105m", "\x1b[106m", "\x1b[107m"};
static const size_t ANSI_FG_LEN[16] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
static const size_t ANSI_BG_LEN[16] = {5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6};
static bool get_terminal_size(int fd, int* width, int* height) {
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) == -1)
        return false;
    *width = ws.ws_col;
    *height = ws.ws_row;
    return true;
}
tty_context* tty_open(const char* tty_path, int min_width, int min_height) {
    tty_context* ctx = NULL;
    int err = 0;
    int have_orig = 0;
    ctx = malloc(sizeof *ctx);
    if (!ctx)
        return NULL;
    memset(ctx, 0, sizeof *ctx);
    ctx->tty_fd = -1;
    ctx->min_width = min_width;
    ctx->min_height = min_height;
    if (tty_path) {
        /* Reject paths that don't fit our buffer to avoid truncation surprises */
        size_t tp_len = strlen(tty_path);
        if (tp_len >= sizeof(ctx->tty_path)) {
            err = 1;
            goto out;
        }
        (void)snprintf(ctx->tty_path, sizeof(ctx->tty_path), "%s", tty_path);
        ctx->tty_fd = open(ctx->tty_path, O_RDWR);
    } else {
        ctx->tty_fd = dup(STDOUT_FILENO);
        (void)snprintf(ctx->tty_path, sizeof(ctx->tty_path), "/dev/stdout");
    }
    if (ctx->tty_fd == -1) {
        err = 2;
        goto out;
    }
    int actual_width = 0, actual_height = 0;
    if (!get_terminal_size(ctx->tty_fd, &actual_width, &actual_height)) {
        err = 3;
        goto out;
    }
    ctx->width = actual_width;
    ctx->height = actual_height;
    ctx->size_valid = !((min_width > 0 && actual_width < min_width) || (min_height > 0 && actual_height < min_height));
    if (tcgetattr(ctx->tty_fd, &ctx->orig_termios) == -1) {
        err = 4;
        goto out;
    }
    have_orig = 1;
    struct termios raw = ctx->orig_termios;
    raw.c_iflag &= (tcflag_t)(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
    raw.c_oflag &= (tcflag_t)(~(OPOST));
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (tcflag_t)(~(ECHO | ICANON | IEXTEN | ISIG));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(ctx->tty_fd, TCSADRAIN, &raw) == -1) {
        err = 5;
        goto out;
    }
    size_t n_cells = (size_t)actual_width * (size_t)actual_height;
    if (n_cells == 0 || n_cells / (size_t)actual_width != (size_t)actual_height) {
        err = 6;
        goto out;
    }
    if (n_cells > SIZE_MAX / sizeof(struct ascii_pixel)) {
        err = 7;
        goto out;
    }
    ctx->front = calloc(n_cells, sizeof(struct ascii_pixel));
    ctx->back = calloc(n_cells, sizeof(struct ascii_pixel));
    if (!ctx->front || !ctx->back) {
        err = 8;
        goto out;
    }
    /* Cap write buffer size to protect low-memory environments */
    if (n_cells > SIZE_MAX / 32) { /* overflow check for write buffer multiplier */
        err = 9;
        goto out;
    }
    size_t proposed_write_buffer_size = n_cells * 32;
    if (proposed_write_buffer_size > TTY_WRITE_BUFFER_CAP)
        proposed_write_buffer_size = TTY_WRITE_BUFFER_CAP;
    ctx->write_buffer_size = proposed_write_buffer_size;
    ctx->write_buffer = malloc(ctx->write_buffer_size);
    if (!ctx->write_buffer) {
        err = 10;
        goto out;
    }
    struct ascii_pixel default_pixel = PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    for (int i = 0; i < actual_width * actual_height; i++) {
        ctx->front[i] = default_pixel;
        ctx->back[i] = default_pixel;
    }
    {
        ssize_t r = write(ctx->tty_fd, "\x1b[2J", sizeof("\x1b[2J") - 1);
        (void)r;
    }
    {
        ssize_t r = write(ctx->tty_fd, "\x1b[?25l", sizeof("\x1b[?25l") - 1);
        (void)r;
    }
    if (signal(SIGWINCH, sigwinch_handler) == SIG_ERR) {
        perror("signal(SIGWINCH) in tty");
    }
    ctx->dirty = true;
    return ctx;
out:
    if (ctx) {
        free(ctx->front);
        free(ctx->back);
        free(ctx->write_buffer);
        if (have_orig) {
            if (tcsetattr(ctx->tty_fd, TCSADRAIN, &ctx->orig_termios) == -1) {
                perror("tty_open: tcsetattr cleanup");
            }
        }
        if (ctx->tty_fd != -1)
            (void)close(ctx->tty_fd);
        free(ctx);
    }
    (void)err;
    return NULL;
}
void tty_close(tty_context* ctx) {
    if (!ctx)
        return;
    {
        ssize_t r = write(ctx->tty_fd, "\x1b[0m", sizeof("\x1b[0m") - 1);
        (void)r;
    }
    {
        ssize_t r = write(ctx->tty_fd, "\x1b[?25h", sizeof("\x1b[?25h") - 1);
        (void)r;
    }
    {
        ssize_t r = write(ctx->tty_fd, "\x1b[2J", sizeof("\x1b[2J") - 1);
        (void)r;
    }
    {
        ssize_t r = write(ctx->tty_fd, "\x1b[H", sizeof("\x1b[H") - 1);
        (void)r;
    }
    if (tcsetattr(ctx->tty_fd, TCSADRAIN, &ctx->orig_termios) == -1) {
        perror("tty_close: tcsetattr");
    }
    free(ctx->front);
    free(ctx->back);
    free(ctx->write_buffer);
    close(ctx->tty_fd);
    free(ctx);
}
size_t tty_get_write_buffer_size(tty_context* ctx) {
    if (!ctx)
        return 0;
    return ctx->write_buffer_size;
}
struct ascii_pixel* tty_get_buffer(tty_context* ctx) {
    if (!ctx)
        return NULL;
    return ctx->back;
}
void tty_put_pixel(tty_context* ctx, int x, int y, struct ascii_pixel px) {
    if (!ctx || x < 0 || y < 0 || x >= ctx->width || y >= ctx->height)
        return;
    ctx->back[y * ctx->width + x] = px;
    ctx->dirty = true;
}
struct ascii_pixel tty_get_pixel(tty_context* ctx, int x, int y) {
    static struct ascii_pixel zero = {0, 0};
    if (!ctx || x < 0 || y < 0 || x >= ctx->width || y >= ctx->height)
        return zero;
    return ctx->back[y * ctx->width + x];
}
void tty_clear_back(tty_context* ctx) {
    if (!ctx)
        return;
    struct ascii_pixel default_pixel = PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    for (int i = 0; i < ctx->width * ctx->height; i++)
        ctx->back[i] = default_pixel;
    ctx->dirty = true;
}
void tty_clear_front(tty_context* ctx) {
    if (!ctx)
        return;
    struct ascii_pixel default_pixel = PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    for (int i = 0; i < ctx->width * ctx->height; i++)
        ctx->front[i] = default_pixel;
}
void tty_flip(tty_context* ctx) {
    if (!ctx || !ctx->dirty)
        return;
    char* buf = ctx->write_buffer;
    size_t pos = 0;
    size_t remaining = ctx->write_buffer_size;
    int current_fg = -1, current_bg = -1;
    for (int y = 0; y < ctx->height; y++) {
        for (int x = 0; x < ctx->width; x++) {
            if (ascii_pixel_equal(ctx->front[y * ctx->width + x], ctx->back[y * ctx->width + x]))
                continue;
            int span_start = x;
            uint16_t span_color = ctx->back[y * ctx->width + x].color;
            while (x < ctx->width &&
                   !ascii_pixel_equal(ctx->front[y * ctx->width + x], ctx->back[y * ctx->width + x]) &&
                   ctx->back[y * ctx->width + x].color == span_color)
                x++;
            int span_end = x;
            /* Cursor move: "\x1b[%d;%dH" (fast append)
             * Use the small helpers to avoid snprintf overhead. */
            if (buf_safe_append_char(buf, &pos, &remaining, '\x1b') < 0)
                return;
            if (buf_safe_append_char(buf, &pos, &remaining, '[') < 0)
                return;
            if (buf_append_uint(buf, &pos, &remaining, (unsigned int)(y + 1)) < 0)
                return;
            if (buf_safe_append_char(buf, &pos, &remaining, ';') < 0)
                return;
            if (buf_append_uint(buf, &pos, &remaining, (unsigned int)(span_start + 1)) < 0)
                return;
            if (buf_safe_append_char(buf, &pos, &remaining, 'H') < 0)
                return;

            uint8_t fg = COLOR_FG(span_color);
            uint8_t bg = COLOR_BG(span_color);
            if (fg != (uint8_t)current_fg || bg != (uint8_t)current_bg) {
                current_fg = (int)fg;
                current_bg = (int)bg;
                /* Append precomputed ANSI sequences for fg/bg */
                {
                    const unsigned idx_fg = fg & 0x0F;
                    const char* seq = ANSI_FG[idx_fg];
                    size_t len = ANSI_FG_LEN[idx_fg];
                    if (len > remaining)
                        return;
                    memcpy(buf + pos, seq, len);
                    pos += len;
                    remaining -= len;
                }
                {
                    const unsigned idx_bg = bg & 0x0F;
                    const char* seq = ANSI_BG[idx_bg];
                    size_t len = ANSI_BG_LEN[idx_bg];
                    if (len > remaining)
                        return;
                    memcpy(buf + pos, seq, len);
                    pos += len;
                    remaining -= len;
                }
            }
            for (int i = span_start; i < span_end; i++) {
                uint16_t ch = ctx->back[y * ctx->width + i].pixel;
                char utf8[4];
                int utf8_len = 0;
                if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < span_end) {
                    uint16_t ch2 = ctx->back[y * ctx->width + (i + 1)].pixel;
                    if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
                        utf8_len = utf16_surrogate_pair_to_utf8(ch, ch2, utf8);
                        i++;
                    }
                }
                if (utf8_len == 0) {
                    utf8_len = utf16_to_utf8(ch, utf8);
                }
                if ((size_t)utf8_len > remaining)
                    return;
                memcpy(buf + pos, utf8, (size_t)utf8_len);
                pos += (size_t)utf8_len;
                remaining -= (size_t)utf8_len;
            }
            for (int i = span_start; i < span_end; i++)
                ctx->front[y * ctx->width + i] = ctx->back[y * ctx->width + i];
            x = span_end - 1;
        }
    }
    ssize_t written = write(ctx->tty_fd, buf, pos);
    (void)written;
    ctx->dirty = false;
}
void tty_force_redraw(tty_context* ctx) {
    if (!ctx)
        return;
    memset(ctx->front, 0, (size_t)ctx->width * (size_t)ctx->height * sizeof(struct ascii_pixel));
    ctx->dirty = true;
    tty_flip(ctx);
}
void tty_get_size(const tty_context* ctx, int* width, int* height) {
    if (!ctx)
        return;
    if (width)
        *width = ctx->width;
    if (height)
        *height = ctx->height;
}
bool tty_calc_resize_requirements(long new_width,
                                  long new_height,
                                  size_t* out_cells,
                                  size_t* out_pixel_bytes,
                                  size_t* out_write_buffer_size) {
    if (new_width <= 0 || new_height <= 0)
        return false;
    size_t w = (size_t)new_width;
    size_t h = (size_t)new_height;
    /* check overflow for w * h */
    if (h > SIZE_MAX / w)
        return false;
    size_t cells = w * h;
    if (cells == 0)
        return false;
    if (cells > SIZE_MAX / sizeof(struct ascii_pixel))
        return false;
    size_t pixel_bytes = cells * sizeof(struct ascii_pixel);
    /* check write buffer size overflow and cap */
    if (cells > SIZE_MAX / 32)
        return false;
    size_t write_sz = cells * 32;
    if (write_sz > TTY_WRITE_BUFFER_CAP)
        write_sz = TTY_WRITE_BUFFER_CAP;
    if (out_cells)
        *out_cells = cells;
    if (out_pixel_bytes)
        *out_pixel_bytes = pixel_bytes;
    if (out_write_buffer_size)
        *out_write_buffer_size = write_sz;
    return true;
}
bool tty_check_resize(tty_context* ctx) {
    if (!ctx || !winch_received)
        return false;
    winch_received = 0;
    int new_width = 0, new_height = 0;
    if (!get_terminal_size(ctx->tty_fd, &new_width, &new_height))
        return false;
    if (new_width == ctx->width && new_height == ctx->height)
        return false;
    int old_width = ctx->width;
    int old_height = ctx->height;
    size_t new_cells = 0, new_pixel_bytes = 0, new_write_buffer_size = 0;
    if (!tty_calc_resize_requirements((long)new_width, (long)new_height, &new_cells, &new_pixel_bytes,
                                      &new_write_buffer_size))
        return false;
    struct ascii_pixel* new_front = calloc(new_cells, sizeof(struct ascii_pixel));
    struct ascii_pixel* new_back = calloc(new_cells, sizeof(struct ascii_pixel));
    char* new_write_buffer = malloc(new_write_buffer_size);
    if (!new_front || !new_back || !new_write_buffer) {
        free(new_front);
        free(new_back);
        free(new_write_buffer);
        return false;
    }
    struct ascii_pixel default_pixel = PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    for (size_t i = 0; i < new_cells; i++) {
        new_front[i] = default_pixel;
        new_back[i] = default_pixel;
    }
    int copy_width = (new_width < old_width) ? new_width : old_width;
    int copy_height = (new_height < old_height) ? new_height : old_height;
    for (int y = 0; y < copy_height; y++) {
        for (int x = 0; x < copy_width; x++) {
            new_front[y * new_width + x] = ctx->front[y * old_width + x];
            new_back[y * new_width + x] = ctx->back[y * old_width + x];
        }
    }
    free(ctx->front);
    free(ctx->back);
    free(ctx->write_buffer);
    ctx->front = new_front;
    ctx->back = new_back;
    ctx->width = new_width;
    ctx->height = new_height;
    ctx->write_buffer = new_write_buffer;
    ctx->write_buffer_size = new_write_buffer_size;
    bool old_size_valid = ctx->size_valid;
    if ((ctx->min_width > 0 && new_width < ctx->min_width) || (ctx->min_height > 0 && new_height < ctx->min_height))
        ctx->size_valid = false;
    else
        ctx->size_valid = true;
    /* Callbacks: copy to local to avoid race between check and call */
    void (*on_resize_cb)(tty_context*, int, int, int, int, void*) = ctx->on_resize;
    if (on_resize_cb)
        on_resize_cb(ctx, old_width, old_height, new_width, new_height, ctx->callback_userdata);
    if (!ctx->size_valid && old_size_valid) {
        void (*on_size_invalid_cb)(tty_context*, int, int, int, int, void*) = ctx->on_size_invalid;
        if (on_size_invalid_cb)
            on_size_invalid_cb(ctx, new_width, new_height, ctx->min_width, ctx->min_height, ctx->callback_userdata);
    }
    ctx->dirty = true;
    return true;
}
void tty_set_resize_callback(tty_context* ctx,
                             void (*callback)(tty_context*, int, int, int, int, void*),
                             void* userdata) {
    if (ctx) {
        ctx->on_resize = callback;
        ctx->callback_userdata = userdata;
    }
}
void tty_set_size_invalid_callback(tty_context* ctx,
                                   void (*callback)(tty_context*, int, int, int, int, void*),
                                   void* userdata) {
    if (ctx) {
        ctx->on_size_invalid = callback;
        ctx->callback_userdata = userdata;
    }
}
bool tty_size_valid(const tty_context* ctx) {
    if (!ctx)
        return false;
    return ctx->size_valid;
}
void tty_get_min_size(const tty_context* ctx, int* min_width, int* min_height) {
    if (!ctx)
        return;
    if (min_width)
        *min_width = ctx->min_width;
    if (min_height)
        *min_height = ctx->min_height;
}
void tty_get_board_min_size(int board_width, int board_height, int* min_width, int* min_height) {
    if (min_width)
        *min_width = board_width + 4;
    if (min_height)
        *min_height = board_height + 4;
}
bool tty_size_sufficient_for_board(int term_width, int term_height, int board_width, int board_height) {
    const int field_width = board_width + 2;
    const int field_height = board_height + 2;
    int min_h = field_height + 2;
    int min_w = field_width + 2;
    (void)board_width;
    (void)board_height;
    return term_width >= min_w && term_height >= min_h;
}
