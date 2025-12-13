# File 5: Sprite Rendering Implementation

## Purpose
Render game entities (snakes, food, other players) as 2D billboards in 3D space. Handles transformation, scaling, depth sorting, and clipping.

## File Locations
- Header: `include/snake/sprite.h`
- Source: `src/render3d/sprite.c`

## Dependencies
- `snake/types.h` - For SnakePoint
- `snake/game.h` - For GameState, PlayerState
- `snake/camera.h` - For Camera3D
- `snake/projection.h` - For ProjectionParams
- `snake/display.h` - For color definitions
- `<stdlib.h>` - For qsort

## Data Structures

```c
typedef enum {
    SPRITE_TYPE_FOOD = 0,
    SPRITE_TYPE_SNAKE_HEAD,
    SPRITE_TYPE_SNAKE_BODY,
    SPRITE_TYPE_PLAYER_HEAD,  // Other player's head
    SPRITE_TYPE_PLAYER_BODY,  // Other player's body
} SpriteType;

typedef struct {
    // World position
    float world_x;
    float world_y;
    
    // Camera space (computed)
    float camera_x;    // Perpendicular to view (left/right)
    float camera_z;    // Depth (forward)
    
    // Screen space (computed)
    int screen_x;      // Center column
    int screen_y;      // Center row
    
    // Dimensions
    int sprite_width;  // Width in screen columns
    int sprite_height; // Height in screen rows
    
    // Type and appearance
    SpriteType type;
    uint16_t color;
    char character;
    
    // Distance (for sorting)
    float distance;
    
    // Visibility
    bool visible;      // Is on-screen and in front
} Sprite;

// Collection of all sprites to render this frame
typedef struct {
    Sprite* sprites;
    int count;
    int capacity;
} SpriteList;
```

## Public API

### Sprite List Management

```c
/**
 * Initialize sprite list with capacity
 * @param list Sprite list to initialize
 * @param capacity Maximum number of sprites
 * @return true on success, false on allocation failure
 */
bool sprite_list_init(SpriteList* list, int capacity);

/**
 * Free sprite list resources
 * @param list Sprite list to free
 */
void sprite_list_free(SpriteList* list);

/**
 * Clear sprite list (for next frame)
 * @param list Sprite list to clear
 */
void sprite_list_clear(SpriteList* list);

/**
 * Add a sprite to the list
 * @param list Sprite list
 * @param world_x World X position
 * @param world_y World Y position
 * @param type Sprite type
 * @return Pointer to added sprite, or NULL if list is full
 */
Sprite* sprite_list_add(SpriteList* list, float world_x, float world_y, SpriteType type);
```

### Sprite Collection (from Game State)

```c
/**
 * Collect all sprites from game state
 * @param list Output sprite list (will be cleared first)
 * @param game Game state
 * @param camera Camera (to exclude camera player)
 */
void sprite_collect_from_game(SpriteList* list, const GameState* game, const Camera3D* camera);
```

**Implementation:**

```c
void sprite_collect_from_game(SpriteList* list, const GameState* game, const Camera3D* camera) {
    sprite_list_clear(list);
    
    // Add food
    for (int i = 0; i < game->food_count; i++) {
        float fx = game->food[i].x + 0.5f;  // Center in cell
        float fy = game->food[i].y + 0.5f;
        sprite_list_add(list, fx, fy, SPRITE_TYPE_FOOD);
    }
    
    // Add snake segments for all players
    for (int p = 0; p < game->num_players; p++) {
        const PlayerState* player = &game->players[p];
        if (!player->active) continue;
        
        // Determine if this is the camera player
        bool is_camera_player = false;
        if (player->length > 0) {
            float head_x = player->body[0].x + 0.5f;
            float head_y = player->body[0].y + 0.5f;
            float dist = sqrtf((head_x - camera->x) * (head_x - camera->x) +
                              (head_y - camera->y) * (head_y - camera->y));
            if (dist < 0.1f) {
                is_camera_player = true;  // Camera is at this player's head
            }
        }
        
        // Skip camera player (first-person view)
        if (is_camera_player) continue;
        
        // Add head
        if (player->length > 0) {
            float hx = player->body[0].x + 0.5f;
            float hy = player->body[0].y + 0.5f;
            sprite_list_add(list, hx, hy, SPRITE_TYPE_PLAYER_HEAD);
        }
        
        // Add body segments (skip last few for performance)
        int step = (player->length > 50) ? 2 : 1;
        for (int i = 1; i < player->length; i += step) {
            float bx = player->body[i].x + 0.5f;
            float by = player->body[i].y + 0.5f;
            sprite_list_add(list, bx, by, SPRITE_TYPE_PLAYER_BODY);
        }
    }
}
```

### Sprite Transformation

```c
/**
 * Transform sprites from world space to screen space
 * @param list Sprite list
 * @param camera Camera for transformation
 * @param params Projection parameters
 */
void sprite_transform(SpriteList* list, const Camera3D* camera, const ProjectionParams* params);

/**
 * Sort sprites by distance (far to near, for painter's algorithm)
 * @param list Sprite list
 */
void sprite_sort_by_distance(SpriteList* list);
```

**Implementation:**

```c
void sprite_transform(SpriteList* list, const Camera3D* camera, const ProjectionParams* params) {
    for (int i = 0; i < list->count; i++) {
        Sprite* s = &list->sprites[i];
        
        // Transform to camera space
        camera_world_to_camera(camera, s->world_x, s->world_y,
                              &s->camera_x, &s->camera_z);
        
        // Calculate distance
        s->distance = s->camera_z;
        
        // Check if in front of camera
        if (s->camera_z <= 0.0f) {
            s->visible = false;
            continue;
        }
        
        // Project to screen space
        float screen_center_x = params->screen_width / 2.0f;
        float inv_z = 1.0f / s->camera_z;
        
        s->screen_x = (int)(screen_center_x + s->camera_x * inv_z * params->screen_width * 0.5f);
        
        // Vertical position (at ground level)
        s->screen_y = params->horizon_y + (int)(params->wall_height_scale * inv_z * 0.5f);
        
        // Sprite size based on distance
        float size_scale = params->wall_height_scale * inv_z;
        s->sprite_width = (int)(size_scale * 0.5f);  // Half-size sprites
        s->sprite_height = (int)(size_scale * 0.5f);
        if (s->sprite_width < 1) s->sprite_width = 1;
        if (s->sprite_height < 1) s->sprite_height = 1;
        
        // Set appearance based on type
        sprite_set_appearance(s);
        
        // Check if on screen
        if (s->screen_x < -s->sprite_width || 
            s->screen_x >= params->screen_width + s->sprite_width ||
            s->screen_y < -s->sprite_height ||
            s->screen_y >= params->screen_height + s->sprite_height) {
            s->visible = false;
            continue;
        }
        
        s->visible = true;
    }
}

static int sprite_compare_distance(const void* a, const void* b) {
    const Sprite* sa = (const Sprite*)a;
    const Sprite* sb = (const Sprite*)b;
    // Sort far to near (descending distance)
    if (sa->distance > sb->distance) return -1;
    if (sa->distance < sb->distance) return 1;
    return 0;
}

void sprite_sort_by_distance(SpriteList* list) {
    qsort(list->sprites, list->count, sizeof(Sprite), sprite_compare_distance);
}
```

### Sprite Appearance

```c
/**
 * Set sprite appearance based on type and distance
 * @param sprite Sprite to configure (modifies character and color)
 */
void sprite_set_appearance(Sprite* sprite);
```

**Implementation:**

```c
void sprite_set_appearance(Sprite* sprite) {
    switch (sprite->type) {
    case SPRITE_TYPE_FOOD:
        sprite->character = '@';  // or '◉' in UTF-8
        sprite->color = DISPLAY_COLOR_BRIGHT_RED;
        break;
        
    case SPRITE_TYPE_SNAKE_HEAD:
        sprite->character = 'O';
        sprite->color = DISPLAY_COLOR_BRIGHT_GREEN;
        break;
        
    case SPRITE_TYPE_SNAKE_BODY:
        sprite->character = 'o';
        sprite->color = DISPLAY_COLOR_GREEN;
        break;
        
    case SPRITE_TYPE_PLAYER_HEAD:
        sprite->character = 'Θ';  // or 'X'
        sprite->color = DISPLAY_COLOR_BRIGHT_MAGENTA;
        break;
        
    case SPRITE_TYPE_PLAYER_BODY:
        sprite->character = '●';  // or 'x'
        sprite->color = DISPLAY_COLOR_MAGENTA;
        break;
        
    default:
        sprite->character = '?';
        sprite->color = DISPLAY_COLOR_WHITE;
        break;
    }
    
    // Fade color based on distance
    if (sprite->distance > 10.0f) {
        sprite->color = DISPLAY_COLOR_BLACK;
    } else if (sprite->distance > 7.0f) {
        // Dim colors for far objects (use non-bright versions)
        if (sprite->color == DISPLAY_COLOR_BRIGHT_RED) sprite->color = DISPLAY_COLOR_RED;
        if (sprite->color == DISPLAY_COLOR_BRIGHT_GREEN) sprite->color = DISPLAY_COLOR_GREEN;
        if (sprite->color == DISPLAY_COLOR_BRIGHT_MAGENTA) sprite->color = DISPLAY_COLOR_MAGENTA;
    }
}
```

### Sprite Rendering

```c
/**
 * Render a sprite to the screen (checks Z-buffer)
 * @param sprite Sprite to render
 * @param z_buffer Depth buffer (array of screen_width floats)
 * @param params Projection parameters
 * @param draw_func Callback to draw character at (x, y, char, color)
 */
void sprite_render(const Sprite* sprite, const float* z_buffer,
                  const ProjectionParams* params,
                  void (*draw_func)(int x, int y, char c, uint16_t color));
```

**Implementation:**

```c
void sprite_render(const Sprite* sprite, const float* z_buffer,
                  const ProjectionParams* params,
                  void (*draw_func)(int x, int y, char c, uint16_t color)) {
    if (!sprite->visible) return;
    
    // Calculate sprite bounds
    int start_x = sprite->screen_x - sprite->sprite_width / 2;
    int end_x = sprite->screen_x + sprite->sprite_width / 2;
    int start_y = sprite->screen_y - sprite->sprite_height / 2;
    int end_y = sprite->screen_y + sprite->sprite_height / 2;
    
    // Clip to screen
    if (start_x < 0) start_x = 0;
    if (end_x >= params->screen_width) end_x = params->screen_width - 1;
    if (start_y < 0) start_y = 0;
    if (end_y >= params->screen_height) end_y = params->screen_height - 1;
    
    // Draw sprite pixels (with Z-buffer check)
    for (int x = start_x; x <= end_x; x++) {
        // Check Z-buffer: only draw if sprite is closer than wall
        if (z_buffer[x] < sprite->distance) {
            continue;  // Wall is in front, skip
        }
        
        for (int y = start_y; y <= end_y; y++) {
            // Could add shape masking here (circle, etc.)
            draw_func(x, y, sprite->character, sprite->color);
        }
    }
}
```

## Z-Buffer Integration

The sprite renderer needs access to the Z-buffer (distance buffer) from wall rendering to avoid drawing sprites behind walls.

```c
// In main 3D renderer:
float z_buffer[SCREEN_WIDTH];

// After wall rendering:
for (int x = 0; x < screen_width; x++) {
    z_buffer[x] = ray_hits[x].distance;
}

// During sprite rendering:
sprite_render(&sprites[i], z_buffer, ...);
```

## Sprite Shapes (Optional Enhancement)

```c
// Define sprite masks for non-rectangular shapes
static const char* FOOD_SHAPE_3x3[3] = {
    " @ ",
    "@@@",
    " @ "
};

static const char* SNAKE_HEAD_5x5[5] = {
    "  O  ",
    " OOO ",
    "OOOOO",
    " OOO ",
    "  O  "
};
```

## Testing Checklist

- [ ] Sprites collected correctly from game state
- [ ] Camera player excluded from sprite list
- [ ] Sprites transform to correct screen positions
- [ ] Distance sorting works (far to near)
- [ ] Sprites behind walls don't show (Z-buffer)
- [ ] Sprites behind camera are invisible
- [ ] Off-screen sprites marked invisible
- [ ] Sprite colors fade with distance
- [ ] No crashes with empty sprite list

## Visual Example

```
View from player 1 looking at player 2:

┌────────────────────────────────────┐
│          ...ceiling...             │
│ ║  ║     Θ (player 2 head)    ║  ║ │ ← Player 2 visible
│ ║  ║     ● ● ● (body)         ║  ║ │
│ ║  ║  @  ● ●                  ║  ║ │ ← Food (@) visible
│ ║  ║                          ║  ║ │
│          ...floor...               │
└────────────────────────────────────┘

Sprites sorted by distance:
1. Player 2 head (distance 8.0)
2. Player 2 body segments (distance 8.5-10.0)
3. Food (distance 5.0) ← Drawn on top (closer)
```

## Performance Notes

- Sprite count typically low (< 100)
- Sorting is O(n log n) but n is small
- Z-buffer check per pixel is O(1)
- Can optimize by culling off-screen sprites early

## Integration Notes

1. Call `sprite_collect_from_game()` each frame
2. Call `sprite_transform()` after camera update
3. Call `sprite_sort_by_distance()` before rendering
4. Render walls first (fills Z-buffer)
5. Render sprites in sorted order

## Future Enhancements

- Animated sprites (cycle characters)
- Sprite shadows on floor
- Billboard rotation (face camera)
- Multi-character sprites (large objects)
- Sprite lighting (based on wall shading)

## Estimated Complexity
- **Lines of Code:** ~250-300
- **Implementation Time:** 2-3 hours
- **Testing Time:** 1 hour
