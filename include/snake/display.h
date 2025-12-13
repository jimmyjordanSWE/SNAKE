#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Display Abstraction Layer
 *
 * This interface provides an abstraction over platform-specific rendering backends.
 * It separates high-level rendering logic from low-level display implementation
 * (TTY, SDL, framebuffer, etc.).
 *
 * Design goals:
 * - Platform independence: render module doesn't know about TTY/terminal specifics
 * - Testability: can mock display for unit tests
 * - Extensibility: can add new backends without changing render module
 */

/* Opaque display context - implementation-specific */
typedef struct DisplayContext DisplayContext;

/* Color constants (compatible with ANSI 16-color palette) */
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

/* Special characters (Unicode box-drawing) */
#define DISPLAY_CHAR_BLOCK 0x2588  /* █ */
#define DISPLAY_CHAR_BOX_H 0x2500  /* ─ */
#define DISPLAY_CHAR_BOX_V 0x2502  /* │ */
#define DISPLAY_CHAR_BOX_TL 0x250C /* ┌ */
#define DISPLAY_CHAR_BOX_TR 0x2510 /* ┐ */
#define DISPLAY_CHAR_BOX_BL 0x2514 /* └ */
#define DISPLAY_CHAR_BOX_BR 0x2518 /* ┘ */
#define DISPLAY_CHAR_CIRCLE 0x25CF /* ● */

/*
 * Initialize display backend
 *
 * @param min_width  Minimum required display width
 * @param min_height Minimum required display height
 * @return Display context on success, NULL on failure
 */
DisplayContext* display_init(int min_width, int min_height);

/*
 * Shutdown and cleanup display backend
 *
 * @param ctx Display context to shutdown
 */
void display_shutdown(DisplayContext* ctx);

/*
 * Get current display dimensions
 *
 * @param ctx    Display context
 * @param width  Output: current width
 * @param height Output: current height
 */
void display_get_size(DisplayContext* ctx, int* width, int* height);

/*
 * Check if display size meets minimum requirements
 *
 * @param ctx Display context
 * @return true if display size is valid, false otherwise
 */
bool display_size_valid(DisplayContext* ctx);

/*
 * Clear the back buffer to default color
 *
 * @param ctx Display context
 */
void display_clear(DisplayContext* ctx);

/*
 * Draw a single character at specified position
 *
 * @param ctx Display context
 * @param x   X coordinate
 * @param y   Y coordinate
 * @param ch  Character (Unicode codepoint)
 * @param fg  Foreground color (0-15)
 * @param bg  Background color (0-15)
 */
void display_put_char(DisplayContext* ctx, int x, int y, uint16_t ch, uint16_t fg, uint16_t bg);

/*
 * Draw a string at specified position
 *
 * @param ctx   Display context
 * @param x     X coordinate
 * @param y     Y coordinate
 * @param str   Null-terminated string
 * @param fg    Foreground color
 * @param bg    Background color
 */
void display_put_string(DisplayContext* ctx, int x, int y, const char* str, uint16_t fg, uint16_t bg);

/*
 * Draw a horizontal line
 *
 * @param ctx   Display context
 * @param x     Starting X coordinate
 * @param y     Y coordinate
 * @param len   Length of line
 * @param ch    Character to use for line
 * @param fg    Foreground color
 * @param bg    Background color
 */
void display_put_hline(DisplayContext* ctx, int x, int y, int len, uint16_t ch, uint16_t fg, uint16_t bg);

/*
 * Draw a vertical line
 *
 * @param ctx   Display context
 * @param x     X coordinate
 * @param y     Starting Y coordinate
 * @param len   Length of line
 * @param ch    Character to use for line
 * @param fg    Foreground color
 * @param bg    Background color
 */
void display_put_vline(DisplayContext* ctx, int x, int y, int len, uint16_t ch, uint16_t fg, uint16_t bg);

/*
 * Present the back buffer to the display (flip buffers)
 *
 * @param ctx Display context
 */
void display_present(DisplayContext* ctx);

/*
 * Force a complete redraw on next present
 * Useful after screen corruption or terminal resize
 *
 * @param ctx Display context
 */
void display_force_redraw(DisplayContext* ctx);
