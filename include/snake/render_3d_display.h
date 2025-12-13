#ifndef SNAKE_RENDER_3D_DISPLAY_H
#define SNAKE_RENDER_3D_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Display3D - Frame buffer and low-level drawing primitives for 3D rendering.
 *
 * Manages:
 * - Frame buffer (character + color grid)
 * - Column-by-column rendering
 * - Vertical line drawing
 * - HUD overlay management
 */

typedef struct {
    char character;
    uint16_t color;
} Framebuffer3DCell;

typedef struct {
    int width;
    int height;
    Framebuffer3DCell* buffer; /* width * height cells */
} Display3D;

/**
 * Initialize 3D display context.
 *
 * @param disp          Display3D context to initialize
 * @param width, height Display dimensions in characters
 */
void display_3d_init(Display3D* disp, int width, int height);

/**
 * Clear the frame buffer to specified character and color.
 *
 * @param disp      Display3D context
 * @param ch        Character to fill with (usually space)
 * @param color     Color code to fill with
 */
void display_3d_clear(Display3D* disp, char ch, uint16_t color);

/**
 * Draw a vertical line of characters in a single column.
 *
 * Useful for wall slices and other vertical elements.
 *
 * @param disp              Display3D context
 * @param col               Screen column (X)
 * @param start_row, end_row  Y range (inclusive)
 * @param ch                Character to draw
 * @param color             Color code
 */
void display_3d_draw_column(Display3D* disp, int col, int start_row, int end_row, char ch, uint16_t color);

/**
 * Draw a single cell.
 *
 * @param disp      Display3D context
 * @param x, y      Screen coordinates
 * @param ch        Character to draw
 * @param color     Color code
 */
void display_3d_draw_cell(Display3D* disp, int x, int y, char ch, uint16_t color);

/**
 * Draw a horizontal line of characters.
 *
 * Useful for floors, ceilings, and HUD separators.
 *
 * @param disp              Display3D context
 * @param row               Screen row (Y)
 * @param start_col, end_col  X range (inclusive)
 * @param ch                Character to draw
 * @param color             Color code
 */
void display_3d_draw_row(Display3D* disp, int row, int start_col, int end_col, char ch, uint16_t color);

/**
 * Get frame buffer cell for reading.
 *
 * @param disp  Display3D context
 * @param x, y  Coordinates
 * @return      Pointer to cell, or NULL if out of bounds
 */
const Framebuffer3DCell* display_3d_get_cell(const Display3D* disp, int x, int y);

/**
 * Present the frame buffer to terminal and return control.
 *
 * Platform-specific: implemented by display backend (e.g., tty).
 *
 * @param disp  Display3D context
 */
void display_3d_present(Display3D* disp);

/**
 * Shutdown display and free resources.
 */
void display_3d_shutdown(Display3D* disp);

#endif /* SNAKE_RENDER_3D_DISPLAY_H */
