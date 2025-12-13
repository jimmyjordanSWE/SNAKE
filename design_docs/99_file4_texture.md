# File 4: Texture/Shading System Implementation

## Purpose
Provide ASCII/UTF-8 character patterns for walls, implement distance-based shading, and handle texture mapping. Creates visual depth and distinguishes surfaces.

## File Locations
- Header: `include/snake/texture.h`
- Source: `src/render3d/texture.c`

## Dependencies
- `snake/raycast.h` - For WallSide, WallType
- `snake/projection.h` - For WallProjection
- `snake/display.h` - For color definitions
- `<stdint.h>` - For uint16_t

## Data Structures

```c
typedef enum {
    SHADE_NEAR = 0,      // 0-3 units: brightest
    SHADE_MID,           // 4-7 units: medium
    SHADE_FAR,           // 8-12 units: dim
    SHADE_VERY_FAR,      // 13+ units: very dim/fade
    SHADE_COUNT
} ShadeLevel;

typedef struct {
    // Character patterns for different wall types and orientations
    const char* north_south_chars;  // E.g., "║│┃▐"
    const char* east_west_chars;    // E.g., "═─━▀"
    
    // Color per shade level (DISPLAY_COLOR_* values)
    uint16_t colors[SHADE_COUNT];
    
    // Brightness characters per shade level
    const char* shade_chars[SHADE_COUNT];
} WallTexture;

typedef struct {
    char character;      // ASCII/UTF-8 char to draw
    uint16_t color;      // Terminal color code
    bool bold;           // Use bold attribute
} TexelResult;
```

## Public API

### Initialization

```c
/**
 * Initialize texture system with character set preference
 * @param use_utf8 true for UTF-8 box drawing, false for ASCII
 */
void texture_init(bool use_utf8);

/**
 * Get the wall texture for a given wall type
 * @param type Wall type
 * @return Pointer to wall texture (static data)
 */
const WallTexture* texture_get_wall(WallType type);
```

### Shading

```c
/**
 * Determine shade level based on distance
 * @param distance Distance from camera
 * @return Shade level enum
 */
ShadeLevel texture_distance_to_shade(float distance);

/**
 * Get color for a distance
 * @param texture Wall texture
 * @param distance Distance from camera
 * @return Color code (DISPLAY_COLOR_*)
 */
uint16_t texture_shade_color(const WallTexture* texture, float distance);
```

### Texture Sampling

```c
/**
 * Sample a wall texture at a specific point
 * @param texture Wall texture
 * @param side Which side of wall was hit
 * @param wall_x Horizontal texture coordinate (0.0-1.0)
 * @param tex_v Vertical texture coordinate (0.0-1.0)
 * @param distance Distance for shading
 * @param result Output: character and color to draw
 */
void texture_sample_wall(const WallTexture* texture, WallSide side,
                        float wall_x, float tex_v, float distance,
                        TexelResult* result);
```

### Floor/Ceiling Textures

```c
/**
 * Sample floor or ceiling texture
 * @param world_x World X coordinate
 * @param world_y World Y coordinate
 * @param distance Distance for shading
 * @param is_ceiling true for ceiling, false for floor
 * @param result Output: character and color to draw
 */
void texture_sample_floor(float world_x, float world_y, float distance,
                         bool is_ceiling, TexelResult* result);
```

## Implementation Details

### Character Sets

```c
// UTF-8 box drawing (preferred)
static const char* UTF8_NS_WALL = "║│┃▐";  // North-South walls
static const char* UTF8_EW_WALL = "═─━▀";  // East-West walls

// ASCII fallback
static const char* ASCII_NS_WALL = "||I|";
static const char* ASCII_EW_WALL = "===#";

// Shading characters (distance-based)
static const char* UTF8_SHADE_CHARS[SHADE_COUNT] = {
    "█▓▒",      // SHADE_NEAR: bright, solid
    "▒░▒",      // SHADE_MID: medium density
    "░ .",      // SHADE_FAR: sparse
    " .",       // SHADE_VERY_FAR: almost empty
};

static const char* ASCII_SHADE_CHARS[SHADE_COUNT] = {
    "#@#",
    "=+:",
    ":. ",
    " . ",
};
```

### Distance to Shade Mapping

```c
ShadeLevel texture_distance_to_shade(float distance) {
    if (distance < 3.0f) return SHADE_NEAR;
    if (distance < 7.0f) return SHADE_MID;
    if (distance < 12.0f) return SHADE_FAR;
    return SHADE_VERY_FAR;
}
```

### Color Palettes

```c
// Border wall colors (by shade level)
static const uint16_t BORDER_COLORS[SHADE_COUNT] = {
    DISPLAY_COLOR_BRIGHT_BLUE,   // SHADE_NEAR
    DISPLAY_COLOR_BLUE,          // SHADE_MID
    DISPLAY_COLOR_CYAN,          // SHADE_FAR
    DISPLAY_COLOR_BLACK,         // SHADE_VERY_FAR
};

// Future: different colors for different wall types
```

### Wall Texture Sampling

```c
void texture_sample_wall(const WallTexture* texture, WallSide side,
                        float wall_x, float tex_v, float distance,
                        TexelResult* result) {
    // Determine shade level
    ShadeLevel shade = texture_distance_to_shade(distance);
    
    // Select character set based on wall orientation
    const char* char_set;
    if (side == WALL_SIDE_NORTH || side == WALL_SIDE_SOUTH) {
        char_set = texture->north_south_chars;
    } else {
        char_set = texture->east_west_chars;
    }
    
    // Select character based on texture coordinate
    // Use wall_x to create horizontal variation
    int char_count = strlen(char_set);
    int char_index = ((int)(wall_x * char_count * 4.0f)) % char_count;
    result->character = char_set[char_index];
    
    // Apply shading overlay (use shade chars for variety)
    const char* shade_chars = texture->shade_chars[shade];
    int shade_char_count = strlen(shade_chars);
    int shade_index = ((int)(tex_v * shade_char_count * 8.0f)) % shade_char_count;
    
    // Blend: prefer detail character at near distances
    if (shade == SHADE_NEAR || shade == SHADE_MID) {
        result->character = char_set[char_index];
    } else {
        result->character = shade_chars[shade_index];
    }
    
    // Set color
    result->color = texture->colors[shade];
    
    // Bold for near objects
    result->bold = (shade == SHADE_NEAR);
}
```

### Floor Texture Sampling

```c
void texture_sample_floor(float world_x, float world_y, float distance,
                         bool is_ceiling, TexelResult* result) {
    ShadeLevel shade = texture_distance_to_shade(distance);
    
    // Checkerboard pattern
    int cell_x = (int)world_x;
    int cell_y = (int)world_y;
    bool checker = ((cell_x + cell_y) % 2) == 0;
    
    // Select character based on distance and pattern
    const char* chars = is_ceiling ? " .·˙" : " ·.▪";
    int char_index = (int)shade;
    if (char_index >= strlen(chars)) char_index = strlen(chars) - 1;
    
    result->character = chars[char_index];
    
    // Color gradient based on distance
    if (is_ceiling) {
        // Ceiling: dark to darker
        static const uint16_t CEIL_COLORS[SHADE_COUNT] = {
            DISPLAY_COLOR_BLACK,
            DISPLAY_COLOR_BLACK,
            DISPLAY_COLOR_BLACK,
            DISPLAY_COLOR_BLACK,
        };
        result->color = CEIL_COLORS[shade];
    } else {
        // Floor: gradient with checkerboard
        static const uint16_t FLOOR_COLORS[SHADE_COUNT] = {
            DISPLAY_COLOR_BRIGHT_GREEN,
            DISPLAY_COLOR_GREEN,
            DISPLAY_COLOR_BLACK,
            DISPLAY_COLOR_BLACK,
        };
        static const uint16_t FLOOR_COLORS_ALT[SHADE_COUNT] = {
            DISPLAY_COLOR_YELLOW,
            DISPLAY_COLOR_BLACK,
            DISPLAY_COLOR_BLACK,
            DISPLAY_COLOR_BLACK,
        };
        result->color = checker ? FLOOR_COLORS[shade] : FLOOR_COLORS_ALT[shade];
    }
    
    result->bold = false;
}
```

## Texture Patterns

### Wall Variations

```
North/South Wall (vertical):
  Near: ║║║│┃▐║║║    (alternating heavy/light)
  Mid:  │││││││││    (consistent light)
  Far:  ░ ░ ░ ░ ░    (sparse dots)
  
East/West Wall (horizontal):
  Near: ═══─━▀═══    (alternating styles)
  Mid:  ────────      (consistent)
  Far:  . . . . .     (sparse)
```

### Floor/Ceiling Patterns

```
Ceiling:
  All distances: sparse dots fading to black
  
Floor (checkerboard):
  Near:  ▪ ▪ ▪ (solid green/yellow cells)
  Mid:   · · · (dots)
  Far:   .   . (very sparse)
```

## Advanced Shading Techniques

### Distance Fog

```c
// Optional: progressive fade beyond max distance
float fog_factor(float distance, float max_distance) {
    if (distance >= max_distance) return 0.0f;  // Fully fogged
    if (distance <= max_distance * 0.7f) return 1.0f;  // Clear
    
    // Linear fade in last 30%
    return (max_distance - distance) / (max_distance * 0.3f);
}
```

### Dithering

```c
// Use alternating patterns to simulate shading
// E.g., "▓▒▓▒" creates visual gradient effect
// Already implemented via character selection using tex_v
```

## Testing Checklist

- [ ] UTF-8 and ASCII modes both work
- [ ] North/South walls look different from East/West walls
- [ ] Distance shading creates clear depth perception
- [ ] Floor checkerboard pattern is visible
- [ ] Very far objects fade appropriately
- [ ] Character selection is deterministic (same texture coords → same char)
- [ ] No buffer overruns when indexing character arrays

## Visual Example

```
Close wall (distance 2.0):
  ║▓║▓║▓║▓║  (bright blue, bold)

Mid wall (distance 5.0):
  │▒│▒│▒│▒│  (medium blue)

Far wall (distance 10.0):
  ░ ░ ░ ░ ░  (cyan, sparse)

Floor at feet (distance 0.5):
  ▪ ▪ ▪ ▪ ▪  (bright green, clear pattern)

Floor mid-distance (distance 5.0):
  · . · . ·  (green, dots)

Floor far (distance 15.0):
  .     .    (black, very sparse)
```

## Performance Notes

- Texture sampling is O(1)
- Character sets are static (no allocation)
- String indexing with modulo (fast)
- Could cache shade level per column if needed

## Integration Notes

- Call `texture_init()` at startup
- Call `texture_sample_wall()` for each vertical wall pixel
- Call `texture_sample_floor()` for floor/ceiling spans (optional)
- Pass `TexelResult` to display system for rendering

## Future Enhancements

- Animated textures (cycle characters)
- Normal mapping (fake lighting)
- Different textures per wall type
- Parallax floor textures

## Estimated Complexity
- **Lines of Code:** ~200-250
- **Implementation Time:** 2-3 hours
- **Testing Time:** 1 hour
