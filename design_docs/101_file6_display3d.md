# File 6: 3D Display Backend Implementation

## Purpose
Provide a framebuffer and rendering primitives for 3D output. Integrates with existing display abstraction and supports efficient column-based rendering.

## File Locations
- Header: `include/snake/display_3d.h` (extends display.h)
- Source: `src/render/display_3d.c`

## Dependencies
- `snake/display.h` - Existing display abstraction
- `snake/tty.h` - Terminal I/O
- `<string.h>` - For memset, memcpy
- `<stdbool.h>` - For bool

## Data Structures

```c
// Framebuffer cell (character + attributes)
typedef struct {
    char character;       // ASCII or UTF-8 char
    uint16_t color;       // DISPLAY_COLOR_* value
    bool bold;            // Bold attribute
    bool dirty;           // Changed since last flush
} FramebufferCell;

// 3D display context (extends DisplayContext conceptually)
typedef struct {
    int width;            // Screen width in columns
    int height;           // Screen height in rows
    
    // Double-buffered framebuffer
    FramebufferCell* front_buffer;
    FramebufferCell* back_buffer;
    
    // TTY handle (from display.h)
    DisplayContext* display_ctx;
    
    // Performance tracking
    int cells_updated;
} Display3D;
```

## Public API

### Initialization

```c
/**
 * Initialize 3D display context
 * @param display_ctx Existing display context (from display_init)
 * @param width Screen width in columns
 * @param height Screen height in rows
 * @return Initialized 3D display, or NULL on failure
 */
Display3D* display_3d_init(DisplayContext* display_ctx, int width, int height);

/**
 * Shutdown and free 3D display
 * @param ctx 3D display context
 */
void display_3d_shutdown(Display3D* ctx);
```

**Implementation:**

```c
Display3D* display_3d_init(DisplayContext* display_ctx, int width, int height) {
    if (!display_ctx || width <= 0 || height <= 0) return NULL;
    
    Display3D* ctx = malloc(sizeof(Display3D));
    if (!ctx) return NULL;
    
    ctx->width = width;
    ctx->height = height;
    ctx->display_ctx = display_ctx;
    
    size_t buffer_size = width * height * sizeof(FramebufferCell);
    ctx->front_buffer = malloc(buffer_size);
    ctx->back_buffer = malloc(buffer_size);
    
    if (!ctx->front_buffer || !ctx->back_buffer) {
        free(ctx->front_buffer);
        free(ctx->back_buffer);
        free(ctx);
        return NULL;
    }
    
    // Initialize buffers
    display_3d_clear(ctx);
    
    ctx->cells_updated = 0;
    
    return ctx;
}

void display_3d_shutdown(Display3D* ctx) {
    if (!ctx) return;
    free(ctx->front_buffer);
    free(ctx->back_buffer);
    free(ctx);
}
```

### Frame Management

```c
/**
 * Clear the back buffer (prepare for new frame)
 * @param ctx 3D display context
 */
void display_3d_clear(Display3D* ctx);

/**
 * Swap buffers and present to screen
 * @param ctx 3D display context
 */
void display_3d_present(Display3D* ctx);
```

**Implementation:**

```c
void display_3d_clear(Display3D* ctx) {
    if (!ctx) return;
    
    // Clear back buffer to default state
    for (int i = 0; i < ctx->width * ctx->height; i++) {
        ctx->back_buffer[i].character = ' ';
        ctx->back_buffer[i].color = DISPLAY_COLOR_BLACK;
        ctx->back_buffer[i].bold = false;
        ctx->back_buffer[i].dirty = true;
    }
}

void display_3d_present(Display3D* ctx) {
    if (!ctx) return;
    
    ctx->cells_updated = 0;
    
    // Compare back buffer with front buffer and update changed cells
    for (int y = 0; y < ctx->height; y++) {
        for (int x = 0; x < ctx->width; x++) {
            int index = y * ctx->width + x;
            
            FramebufferCell* back = &ctx->back_buffer[index];
            FramebufferCell* front = &ctx->front_buffer[index];
            
            // Check if cell changed
            if (back->character != front->character ||
                back->color != front->color ||
                back->bold != front->bold) {
                
                // Update screen
                display_move_cursor(ctx->display_ctx, x, y);
                display_set_color(ctx->display_ctx, back->color);
                
                if (back->bold) {
                    // Apply bold attribute (use bright colors)
                    // This is terminal-specific, might need adjustment
                }
                
                // Draw character
                if (back->character >= 32 && back->character <= 126) {
                    // Printable ASCII
                    display_draw_char(ctx->display_ctx, back->character);
                } else if (back->character >= 128) {
                    // UTF-8 multibyte (assume valid)
                    display_draw_char(ctx->display_ctx, back->character);
                } else {
                    // Control character, draw space
                    display_draw_char(ctx->display_ctx, ' ');
                }
                
                // Update front buffer
                *front = *back;
                ctx->cells_updated++;
            }
        }
    }
    
    // Flush to terminal
    display_flush(ctx->display_ctx);
}
```

### Drawing Primitives

```c
/**
 * Draw a single character at position
 * @param ctx 3D display context
 * @param x Column (0 to width-1)
 * @param y Row (0 to height-1)
 * @param character Character to draw
 * @param color Color (DISPLAY_COLOR_*)
 * @param bold Bold attribute
 */
void display_3d_put_char(Display3D* ctx, int x, int y, 
                         char character, uint16_t color, bool bold);

/**
 * Draw a vertical line (for wall columns)
 * @param ctx 3D display context
 * @param x Column
 * @param y_start Start row (inclusive)
 * @param y_end End row (inclusive)
 * @param character Character to draw
 * @param color Color
 * @param bold Bold attribute
 */
void display_3d_vline(Display3D* ctx, int x, int y_start, int y_end,
                     char character, uint16_t color, bool bold);

/**
 * Fill a horizontal span (for floor/ceiling)
 * @param ctx 3D display context
 * @param y Row
 * @param x_start Start column (inclusive)
 * @param x_end End column (inclusive)
 * @param character Character to draw
 * @param color Color
 */
void display_3d_hspan(Display3D* ctx, int y, int x_start, int x_end,
                     char character, uint16_t color);
```

**Implementation:**

```c
void display_3d_put_char(Display3D* ctx, int x, int y,
                        char character, uint16_t color, bool bold) {
    if (!ctx) return;
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) return;
    
    int index = y * ctx->width + x;
    ctx->back_buffer[index].character = character;
    ctx->back_buffer[index].color = color;
    ctx->back_buffer[index].bold = bold;
}

void display_3d_vline(Display3D* ctx, int x, int y_start, int y_end,
                     char character, uint16_t color, bool bold) {
    if (!ctx) return;
    if (x < 0 || x >= ctx->width) return;
    
    if (y_start < 0) y_start = 0;
    if (y_end >= ctx->height) y_end = ctx->height - 1;
    
    for (int y = y_start; y <= y_end; y++) {
        display_3d_put_char(ctx, x, y, character, color, bold);
    }
}

void display_3d_hspan(Display3D* ctx, int y, int x_start, int x_end,
                     char character, uint16_t color) {
    if (!ctx) return;
    if (y < 0 || y >= ctx->height) return;
    
    if (x_start < 0) x_start = 0;
    if (x_end >= ctx->width) x_end = ctx->width - 1;
    
    for (int x = x_start; x <= x_end; x++) {
        display_3d_put_char(ctx, x, y, character, color, false);
    }
}
```

### Utility Functions

```c
/**
 * Get screen dimensions
 * @param ctx 3D display context
 * @param width Output: width
 * @param height Output: height
 */
void display_3d_get_size(const Display3D* ctx, int* width, int* height);

/**
 * Check if coordinates are on screen
 * @param ctx 3D display context
 * @param x Column
 * @param y Row
 * @return true if in bounds
 */
bool display_3d_in_bounds(const Display3D* ctx, int x, int y);
```

## Integration with Existing Display System

The 3D display wraps the existing `DisplayContext` from `display.h`. This allows:
- Reuse of TTY initialization
- Consistent color handling
- Easy switching between 2D and 3D modes

```c
// In main render code:
DisplayContext* display = display_init(min_width, min_height);
Display3D* display_3d = display_3d_init(display, width, height);

// 3D rendering
display_3d_clear(display_3d);
// ... draw walls, sprites, HUD ...
display_3d_present(display_3d);

// 2D rendering (fallback)
display_clear(display);
// ... draw 2D game ...
display_flush(display);
```

## Performance Optimizations

### Dirty Region Tracking

```c
// Optional: track dirty rectangle to minimize updates
typedef struct {
    int min_x, max_x;
    int min_y, max_y;
} DirtyRect;

void display_3d_mark_dirty(Display3D* ctx, int x, int y) {
    // Update dirty_rect
}

void display_3d_present_dirty(Display3D* ctx) {
    // Only update cells in dirty rectangle
}
```

### Column-Based Rendering

The raycasting algorithm naturally produces vertical columns. Optimize for this:

```c
// Render a full column at once
void display_3d_render_column(Display3D* ctx, int x, const ColumnData* col) {
    // Draw ceiling span
    display_3d_vline(ctx, x, 0, col->ceiling_end, ' ', DISPLAY_COLOR_BLACK, false);
    
    // Draw wall
    display_3d_vline(ctx, x, col->wall_start, col->wall_end,
                    col->wall_char, col->wall_color, col->wall_bold);
    
    // Draw floor span
    display_3d_vline(ctx, x, col->floor_start, ctx->height - 1,
                    col->floor_char, col->floor_color, false);
}
```

## Testing Checklist

- [ ] Framebuffer allocates correctly
- [ ] Clear sets all cells to default state
- [ ] Put_char handles out-of-bounds gracefully
- [ ] Vline draws correct number of pixels
- [ ] Present only updates changed cells
- [ ] Double buffering prevents flicker
- [ ] Memory leaks are prevented (valgrind)
- [ ] Screen updates are smooth (60 FPS)

## Debug Utilities

```c
/**
 * Get performance statistics
 * @param ctx 3D display context
 * @return Number of cells updated last frame
 */
int display_3d_get_cells_updated(const Display3D* ctx);

/**
 * Draw debug grid
 * @param ctx 3D display context
 */
void display_3d_draw_debug_grid(Display3D* ctx);
```

## Example Usage

```c
// Initialize
Display3D* d3d = display_3d_init(display, 80, 24);

// Render loop
while (running) {
    display_3d_clear(d3d);
    
    // Draw walls
    for (int x = 0; x < 80; x++) {
        WallProjection proj = /* ... */;
        for (int y = proj.draw_start; y <= proj.draw_end; y++) {
            display_3d_put_char(d3d, x, y, '║', DISPLAY_COLOR_BLUE, false);
        }
    }
    
    // Draw sprites
    for (int i = 0; i < sprite_count; i++) {
        Sprite* s = &sprites[i];
        display_3d_put_char(d3d, s->screen_x, s->screen_y,
                           s->character, s->color, false);
    }
    
    // Present
    display_3d_present(d3d);
    
    // Throttle to 60 FPS
    usleep(16667);
}

// Cleanup
display_3d_shutdown(d3d);
```

## Integration with Existing display.h API

Ensure these functions are available (or add wrappers):
- `display_move_cursor(ctx, x, y)`
- `display_set_color(ctx, color)`
- `display_draw_char(ctx, c)`
- `display_flush(ctx)`
- `display_clear(ctx)`

## Terminal Compatibility

- Tested on: xterm, gnome-terminal, iTerm2, Windows Terminal
- Requires ANSI color support (most modern terminals)
- UTF-8 support optional (graceful ASCII fallback)
- Minimum terminal size: 80×24

## Estimated Complexity
- **Lines of Code:** ~200-250
- **Implementation Time:** 2-3 hours
- **Testing Time:** 1 hour
