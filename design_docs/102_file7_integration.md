# File 7: Mode Integration & Controller Implementation

## Purpose
Integrate the 3D renderer with the existing game loop, provide view mode switching, and implement the main 3D rendering pipeline. This ties all previous components together.

## File Locations
- Header: `include/snake/render_3d.h`
- Source: `src/render/render_3d.c`
- Modified: `src/render/render.c` (add mode switching)
- Modified: `src/input/input.c` (add view toggle key)
- Modified: `main.c` (initialization and configuration)

## Dependencies
All previous components:
- `snake/camera.h`
- `snake/raycast.h`
- `snake/projection.h`
- `snake/texture.h`
- `snake/sprite.h`
- `snake/display_3d.h`
- Plus existing: `snake/game.h`, `snake/render.h`, `snake/input.h`

## Data Structures

```c
typedef enum {
    RENDER_MODE_2D = 0,    // Original top-down view
    RENDER_MODE_3D,        // Pseudo-3D first-person view
    RENDER_MODE_COUNT
} RenderMode;

typedef struct {
    // Current mode
    RenderMode mode;
    
    // 3D rendering components
    Camera3D camera;
    ProjectionParams projection;
    Display3D* display_3d;
    SpriteList sprites;
    
    // Z-buffer for sprite rendering
    float* z_buffer;
    int z_buffer_size;
    
    // Configuration
    float fov;               // Field of view (degrees)
    float max_distance;      // Maximum render distance
    bool render_floor;       // Enable floor rendering
    bool render_ceiling;     // Enable ceiling rendering
    
    // Active player (whose view we're rendering)
    int camera_player;       // 0 or 1
} Render3DContext;
```

## Public API

### Initialization

```c
/**
 * Initialize 3D rendering system
 * @param display Existing display context
 * @param width Screen width
 * @param height Screen height
 * @param use_utf8 Use UTF-8 characters
 * @return Initialized 3D context, or NULL on failure
 */
Render3DContext* render_3d_init(DisplayContext* display, int width, int height, bool use_utf8);

/**
 * Shutdown 3D rendering system
 * @param ctx 3D render context
 */
void render_3d_shutdown(Render3DContext* ctx);
```

**Implementation:**

```c
Render3DContext* render_3d_init(DisplayContext* display, int width, int height, bool use_utf8) {
    Render3DContext* ctx = malloc(sizeof(Render3DContext));
    if (!ctx) return NULL;
    
    // Default to 2D mode
    ctx->mode = RENDER_MODE_2D;
    
    // Initialize camera
    camera_init(&ctx->camera, 70.0f);  // 70° FOV
    
    // Initialize projection
    projection_init(&ctx->projection, width, height);
    
    // Initialize 3D display
    ctx->display_3d = display_3d_init(display, width, height);
    if (!ctx->display_3d) {
        free(ctx);
        return NULL;
    }
    
    // Initialize texture system
    texture_init(use_utf8);
    
    // Initialize sprite list
    if (!sprite_list_init(&ctx->sprites, 256)) {
        display_3d_shutdown(ctx->display_3d);
        free(ctx);
        return NULL;
    }
    
    // Allocate Z-buffer
    ctx->z_buffer_size = width;
    ctx->z_buffer = malloc(width * sizeof(float));
    if (!ctx->z_buffer) {
        sprite_list_free(&ctx->sprites);
        display_3d_shutdown(ctx->display_3d);
        free(ctx);
        return NULL;
    }
    
    // Configuration
    ctx->fov = 70.0f;
    ctx->max_distance = 50.0f;
    ctx->render_floor = true;
    ctx->render_ceiling = false;  // Ceiling is typically dark/empty
    ctx->camera_player = 0;
    
    return ctx;
}

void render_3d_shutdown(Render3DContext* ctx) {
    if (!ctx) return;
    
    free(ctx->z_buffer);
    sprite_list_free(&ctx->sprites);
    display_3d_shutdown(ctx->display_3d);
    free(ctx);
}
```

### Mode Control

```c
/**
 * Toggle between 2D and 3D rendering modes
 * @param ctx 3D render context
 */
void render_3d_toggle_mode(Render3DContext* ctx);

/**
 * Set rendering mode explicitly
 * @param ctx 3D render context
 * @param mode Desired render mode
 */
void render_3d_set_mode(Render3DContext* ctx, RenderMode mode);

/**
 * Get current rendering mode
 * @param ctx 3D render context
 * @return Current mode
 */
RenderMode render_3d_get_mode(const Render3DContext* ctx);
```

### Main Rendering Function

```c
/**
 * Render the game in 3D mode
 * @param ctx 3D render context
 * @param game Game state
 * @param player_name Player name for HUD
 * @param scores High scores for HUD
 * @param score_count Number of high scores
 */
void render_3d_draw(Render3DContext* ctx, const GameState* game,
                   const char* player_name,
                   const HighScore* scores, int score_count);
```

## Main Rendering Pipeline Implementation

```c
void render_3d_draw(Render3DContext* ctx, const GameState* game,
                   const char* player_name,
                   const HighScore* scores, int score_count) {
    if (!ctx || !game) return;
    
    // Clear display
    display_3d_clear(ctx->display_3d);
    
    // Clear Z-buffer
    for (int i = 0; i < ctx->z_buffer_size; i++) {
        ctx->z_buffer[i] = ctx->max_distance;
    }
    
    // Update camera from active player
    const PlayerState* player = &game->players[ctx->camera_player];
    if (player->active && player->length > 0) {
        SnakePoint head = player->body[0];
        camera_set_from_player(&ctx->camera, head.x, head.y, player->current_dir);
    }
    
    // === STEP 1: Render Walls (Column-by-Column) ===
    int screen_width, screen_height;
    display_3d_get_size(ctx->display_3d, &screen_width, &screen_height);
    
    for (int x = 0; x < screen_width; x++) {
        // Generate ray for this column
        float ray_dir_x, ray_dir_y;
        camera_get_ray(&ctx->camera, x, screen_width, &ray_dir_x, &ray_dir_y);
        
        // Cast ray
        RayHit hit;
        raycast(game, &ctx->camera, ray_dir_x, ray_dir_y, ctx->max_distance, &hit);
        
        // Store distance in Z-buffer
        ctx->z_buffer[x] = hit.hit ? hit.distance : ctx->max_distance;
        
        if (!hit.hit) {
            // No wall hit, draw empty column
            continue;
        }
        
        // Project wall to screen
        WallProjection proj;
        projection_wall(&ctx->projection, &hit, x, &proj);
        
        // Get wall texture
        const WallTexture* wall_tex = texture_get_wall(hit.type);
        
        // === Draw ceiling (optional) ===
        if (ctx->render_ceiling && proj.draw_start > 0) {
            for (int y = 0; y < proj.draw_start; y++) {
                TexelResult texel;
                texture_sample_floor(0, 0, hit.distance, true, &texel);
                display_3d_put_char(ctx->display_3d, x, y,
                                   texel.character, texel.color, texel.bold);
            }
        }
        
        // === Draw wall ===
        float tex_v = proj.tex_pos;
        for (int y = proj.draw_start; y <= proj.draw_end; y++) {
            // Sample texture
            TexelResult texel;
            texture_sample_wall(wall_tex, hit.side, hit.wall_x, tex_v,
                              hit.distance, &texel);
            
            display_3d_put_char(ctx->display_3d, x, y,
                               texel.character, texel.color, texel.bold);
            
            tex_v += proj.tex_step;
        }
        
        // === Draw floor (optional) ===
        if (ctx->render_floor && proj.draw_end < screen_height - 1) {
            for (int y = proj.draw_end + 1; y < screen_height; y++) {
                // Calculate floor position for this row
                float row_distance, step_x, step_y;
                projection_floor_row(&ctx->projection, y, 0.5f,
                                   &row_distance, &step_x, &step_y);
                
                // Calculate floor world position
                float floor_x = ctx->camera.x + ray_dir_x * row_distance;
                float floor_y = ctx->camera.y + ray_dir_y * row_distance;
                
                TexelResult texel;
                texture_sample_floor(floor_x, floor_y, row_distance, false, &texel);
                display_3d_put_char(ctx->display_3d, x, y,
                                   texel.character, texel.color, texel.bold);
            }
        }
    }
    
    // === STEP 2: Render Sprites ===
    sprite_collect_from_game(&ctx->sprites, game, &ctx->camera);
    sprite_transform(&ctx->sprites, &ctx->camera, &ctx->projection);
    sprite_sort_by_distance(&ctx->sprites);
    
    for (int i = 0; i < ctx->sprites.count; i++) {
        sprite_render(&ctx->sprites.sprites[i], ctx->z_buffer,
                     &ctx->projection,
                     (void (*)(int, int, char, uint16_t))display_3d_put_char_wrapper);
    }
    
    // === STEP 3: Draw HUD ===
    render_3d_draw_hud(ctx, game, player_name, scores, score_count);
    
    // === STEP 4: Present frame ===
    display_3d_present(ctx->display_3d);
}

// Wrapper for sprite rendering callback
static Display3D* g_display_3d = NULL;  // Set before sprite rendering

void display_3d_put_char_wrapper(int x, int y, char c, uint16_t color) {
    display_3d_put_char(g_display_3d, x, y, c, color, false);
}
```

## HUD Rendering

```c
/**
 * Draw heads-up display (score, minimap, controls)
 * @param ctx 3D render context
 * @param game Game state
 * @param player_name Player name
 * @param scores High scores
 * @param score_count Number of high scores
 */
void render_3d_draw_hud(Render3DContext* ctx, const GameState* game,
                       const char* player_name,
                       const HighScore* scores, int score_count);
```

**Implementation:**

```c
void render_3d_draw_hud(Render3DContext* ctx, const GameState* game,
                       const char* player_name,
                       const HighScore* scores, int score_count) {
    int width, height;
    display_3d_get_size(ctx->display_3d, &width, &height);
    
    // Score at top-left
    const PlayerState* player = &game->players[ctx->camera_player];
    char score_text[64];
    snprintf(score_text, sizeof(score_text), "Score: %d", player->score);
    
    for (int i = 0; i < strlen(score_text) && i < width; i++) {
        display_3d_put_char(ctx->display_3d, i, 0,
                           score_text[i], DISPLAY_COLOR_BRIGHT_YELLOW, true);
    }
    
    // Player name at top-right
    int name_x = width - strlen(player_name) - 1;
    if (name_x < 0) name_x = 0;
    for (int i = 0; i < strlen(player_name) && name_x + i < width; i++) {
        display_3d_put_char(ctx->display_3d, name_x + i, 0,
                           player_name[i], DISPLAY_COLOR_BRIGHT_CYAN, true);
    }
    
    // Minimap at bottom-right (optional, simple version)
    int minimap_size = 10;
    int minimap_x = width - minimap_size - 1;
    int minimap_y = height - minimap_size - 1;
    
    if (minimap_x > 0 && minimap_y > 0) {
        render_3d_draw_minimap(ctx, game, minimap_x, minimap_y, minimap_size);
    }
    
    // Controls hint at bottom-left
    const char* hint = "[V] Toggle View  [Arrows] Move  [P] Pause  [Q] Quit";
    int hint_len = strlen(hint);
    if (hint_len > width) hint_len = width;
    for (int i = 0; i < hint_len; i++) {
        display_3d_put_char(ctx->display_3d, i, height - 1,
                           hint[i], DISPLAY_COLOR_WHITE, false);
    }
}

void render_3d_draw_minimap(Render3DContext* ctx, const GameState* game,
                           int offset_x, int offset_y, int size) {
    // Simple top-down minimap
    // Scale game board to fit minimap size
    float scale_x = (float)size / game->width;
    float scale_y = (float)size / game->height;
    
    // Draw border
    for (int i = 0; i < size; i++) {
        display_3d_put_char(ctx->display_3d, offset_x + i, offset_y, '-', DISPLAY_COLOR_WHITE, false);
        display_3d_put_char(ctx->display_3d, offset_x + i, offset_y + size - 1, '-', DISPLAY_COLOR_WHITE, false);
        display_3d_put_char(ctx->display_3d, offset_x, offset_y + i, '|', DISPLAY_COLOR_WHITE, false);
        display_3d_put_char(ctx->display_3d, offset_x + size - 1, offset_y + i, '|', DISPLAY_COLOR_WHITE, false);
    }
    
    // Draw player
    const PlayerState* player = &game->players[ctx->camera_player];
    if (player->active && player->length > 0) {
        int px = (int)(player->body[0].x * scale_x);
        int py = (int)(player->body[0].y * scale_y);
        display_3d_put_char(ctx->display_3d, offset_x + px, offset_y + py,
                           '@', DISPLAY_COLOR_BRIGHT_GREEN, true);
    }
    
    // Draw food
    for (int i = 0; i < game->food_count; i++) {
        int fx = (int)(game->food[i].x * scale_x);
        int fy = (int)(game->food[i].y * scale_y);
        display_3d_put_char(ctx->display_3d, offset_x + fx, offset_y + fy,
                           '*', DISPLAY_COLOR_BRIGHT_RED, false);
    }
}
```

## Input Handling Integration

### Modified input.c

Add view toggle key:

```c
// In input.c
typedef enum {
    INPUT_KEY_NONE = 0,
    // ... existing keys ...
    INPUT_KEY_VIEW_TOGGLE = 'v',  // or 'V'
} InputKey;

// In input processing:
if (key == 'v' || key == 'V') {
    return INPUT_KEY_VIEW_TOGGLE;
}
```

### Modified main.c

```c
// In main game loop:
if (input == INPUT_KEY_VIEW_TOGGLE) {
    render_3d_toggle_mode(render_3d_ctx);
}

// In render selection:
if (render_3d_get_mode(render_3d_ctx) == RENDER_MODE_3D) {
    render_3d_draw(render_3d_ctx, &game, player_name, scores, score_count);
} else {
    render_draw(&game, player_name, scores, score_count);  // Original 2D
}
```

## Configuration Options

```c
/**
 * Configure 3D rendering parameters
 * @param ctx 3D render context
 * @param fov Field of view in degrees (45-120)
 * @param max_distance Maximum render distance
 * @param render_floor Enable floor rendering
 * @param render_ceiling Enable ceiling rendering
 */
void render_3d_configure(Render3DContext* ctx,
                        float fov,
                        float max_distance,
                        bool render_floor,
                        bool render_ceiling);

/**
 * Set which player's view to render
 * @param ctx 3D render context
 * @param player_index Player index (0 or 1)
 */
void render_3d_set_camera_player(Render3DContext* ctx, int player_index);
```

## Testing Checklist

- [ ] Toggle between 2D and 3D works seamlessly
- [ ] 3D view shows correct perspective
- [ ] HUD displays score and player name
- [ ] Minimap shows player position
- [ ] No crashes when switching modes
- [ ] Performance is acceptable (>30 FPS)
- [ ] Memory is properly freed on shutdown
- [ ] Works with both UTF-8 and ASCII modes
- [ ] Multiplayer: can switch camera between players

## Integration Steps

1. **Add header includes** to main.c and render.c
2. **Initialize 3D context** in main.c startup
3. **Add view toggle handler** in input processing
4. **Modify render dispatch** to check mode
5. **Add cleanup** in shutdown sequence
6. **Update Makefile** to include new source files

### Makefile Changes

```makefile
# Add to SOURCES
SOURCES += src/render3d/camera.c \
           src/render3d/raycast.c \
           src/render3d/projection.c \
           src/render3d/texture.c \
           src/render3d/sprite.c \
           src/render/display_3d.c \
           src/render/render_3d.c

# Add to HEADERS
HEADERS += include/snake/camera.h \
           include/snake/raycast.h \
           include/snake/projection.h \
           include/snake/texture.h \
           include/snake/sprite.h \
           include/snake/display_3d.h \
           include/snake/render_3d.h
```

## Performance Considerations

- **Target:** 60 FPS on modern terminals
- **Bottlenecks:** 
  - Terminal I/O (use double buffering)
  - Raycasting loop (optimize with SIMD if needed)
  - Sprite sorting (use fast sort, small n)
- **Optimizations:**
  - Skip columns for lower resolution
  - Limit sprite count
  - Cache texture lookups
  - Use dirty rectangle updates

## Debug Mode

```c
/**
 * Enable debug rendering (wireframe, stats)
 * @param ctx 3D render context
 * @param enable Enable debug mode
 */
void render_3d_set_debug(Render3DContext* ctx, bool enable);
```

Draw debug info:
- FPS counter
- Ray count
- Sprite count
- Camera position/angle
- Z-buffer visualization

## Future Enhancements

- Multiple camera modes (first-person, third-person, chase cam)
- Smooth camera transitions
- Screen-space effects (scanlines, CRT effect)
- Spectator mode (free camera)
- Stereoscopic 3D (anaglyph)

## Estimated Complexity
- **Lines of Code:** ~400-500 (including HUD and integration)
- **Implementation Time:** 3-4 hours
- **Testing Time:** 2 hours
- **Total Project Time:** 15-20 hours for all 7 files

## Final Integration Test Plan

1. Build system compiles all files
2. Game starts in 2D mode
3. Press 'V' to switch to 3D
4. Walls render correctly with perspective
5. Food items appear as sprites
6. Other player visible (in 2-player mode)
7. HUD shows score and controls
8. Press 'V' to return to 2D
9. No memory leaks (run with valgrind)
10. Performance >30 FPS

## Completion Criteria

✅ All 7 implementation files created  
✅ Code compiles without errors  
✅ 3D rendering displays correctly  
✅ Mode switching works reliably  
✅ No memory leaks or crashes  
✅ Documentation complete  
✅ Tests pass
