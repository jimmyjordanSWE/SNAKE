# File 3: 3D Projection Math Implementation

## Purpose
Convert raycasting distances into wall heights and screen coordinates. Handles vertical projection, clipping, and provides utilities for rendering.

## File Locations
- Header: `include/snake/projection.h`
- Source: `src/render3d/projection.c`

## Dependencies
- `snake/raycast.h` - For RayHit
- `<stdint.h>` - For int types
- `<stdbool.h>` - For bool

## Data Structures

```c
typedef struct {
    // Screen space
    int screen_x;          // Column on screen
    
    // Vertical span
    int draw_start;        // Top pixel of wall
    int draw_end;          // Bottom pixel of wall
    
    // Wall height in pixels
    int wall_height;
    
    // Clipping flags
    bool clipped_top;
    bool clipped_bottom;
    
    // Texture mapping
    float tex_step;        // Step size for texture coordinate per pixel
    float tex_pos;         // Starting texture position
} WallProjection;

typedef struct {
    // Screen dimensions
    int screen_width;      // Columns (terminal width)
    int screen_height;     // Rows (terminal height)
    
    // Projection parameters
    float wall_height_scale; // Scale factor for wall heights
    
    // Horizon position (vertical center)
    int horizon_y;
} ProjectionParams;
```

## Public API

### Initialization

```c
/**
 * Initialize projection parameters for a given screen size
 * @param params Output: projection parameters
 * @param screen_width Screen width in columns
 * @param screen_height Screen height in rows
 */
void projection_init(ProjectionParams* params, int screen_width, int screen_height);
```

**Implementation Notes:**
- `horizon_y = screen_height / 2` (vertical center)
- `wall_height_scale = screen_height` (walls at distance 1.0 fill screen)
- Can adjust scale for aesthetic preference (0.8 - 1.5 range)

### Wall Projection

```c
/**
 * Project a ray hit into screen space coordinates
 * @param params Projection parameters
 * @param hit Ray hit information (must have hit->hit == true)
 * @param screen_x Column on screen
 * @param projection Output: wall projection data
 */
void projection_wall(const ProjectionParams* params, const RayHit* hit,
                    int screen_x, WallProjection* projection);
```

**Implementation:**

```c
void projection_wall(const ProjectionParams* params, const RayHit* hit,
                    int screen_x, WallProjection* projection) {
    projection->screen_x = screen_x;
    
    // Calculate wall height
    // wall_height = screen_height / distance
    // Clamp distance to prevent division by zero
    float distance = hit->distance;
    if (distance < 0.01f) distance = 0.01f;
    
    int wall_height = (int)(params->wall_height_scale / distance);
    projection->wall_height = wall_height;
    
    // Calculate draw start and end (centered on horizon)
    int draw_start = params->horizon_y - wall_height / 2;
    int draw_end = params->horizon_y + wall_height / 2;
    
    // Clipping
    projection->clipped_top = false;
    projection->clipped_bottom = false;
    
    if (draw_start < 0) {
        projection->clipped_top = true;
        draw_start = 0;
    }
    
    if (draw_end >= params->screen_height) {
        projection->clipped_bottom = true;
        draw_end = params->screen_height - 1;
    }
    
    projection->draw_start = draw_start;
    projection->draw_end = draw_end;
    
    // Texture mapping parameters (for texture system to use)
    projection->tex_step = 1.0f / (float)wall_height;
    projection->tex_pos = 0.0f;
    
    // Adjust tex_pos if top is clipped
    if (projection->clipped_top) {
        projection->tex_pos = (0 - (params->horizon_y - wall_height / 2)) 
                             * projection->tex_step;
    }
}
```

### Floor/Ceiling Projection

```c
/**
 * Calculate floor/ceiling parameters for a horizontal row
 * @param params Projection parameters
 * @param screen_y Row on screen
 * @param camera_height Camera height above floor (typically 0.5)
 * @param row_distance Output: distance to floor at this row
 * @param floor_step_x Output: floor X step per column
 * @param floor_step_y Output: floor Y step per column
 */
void projection_floor_row(const ProjectionParams* params, int screen_y,
                         float camera_height,
                         float* row_distance,
                         float* floor_step_x, float* floor_step_y);
```

**Implementation:**

```c
void projection_floor_row(const ProjectionParams* params, int screen_y,
                         float camera_height,
                         float* row_distance,
                         float* floor_step_x, float* floor_step_y) {
    // Distance from camera to floor at this screen row
    // row_distance = camera_height / (screen_y - horizon_y)
    
    int p = screen_y - params->horizon_y;
    
    if (p == 0) {
        // Horizon line - infinite distance
        *row_distance = 1e30f;
        *floor_step_x = 0.0f;
        *floor_step_y = 0.0f;
        return;
    }
    
    float pos_z = params->screen_height * camera_height;
    *row_distance = pos_z / (float)p;
    
    // Step vectors will be computed by renderer using camera plane
    // These are just placeholders for now
    *floor_step_x = *row_distance / (float)params->screen_width;
    *floor_step_y = *row_distance / (float)params->screen_width;
}
```

### Distance-to-Screen Utilities

```c
/**
 * Get the screen Y coordinate for a given height at a distance
 * @param params Projection parameters
 * @param distance Distance from camera
 * @param world_height Height in world units (0 = floor, 1 = ceiling)
 * @return Screen Y coordinate (row)
 */
int projection_height_to_screen(const ProjectionParams* params,
                                float distance, float world_height);

/**
 * Get the screen X coordinate for a sprite at camera-space position
 * @param params Projection parameters
 * @param camera_x Sprite X in camera space (perpendicular to view)
 * @param camera_z Sprite Z in camera space (depth)
 * @return Screen X coordinate (column), or -1 if off-screen
 */
int projection_sprite_x_to_screen(const ProjectionParams* params,
                                  float camera_x, float camera_z);
```

**Implementation:**

```c
int projection_height_to_screen(const ProjectionParams* params,
                                float distance, float world_height) {
    if (distance < 0.01f) distance = 0.01f;
    
    // Calculate screen offset from horizon
    int screen_offset = (int)((world_height - 0.5f) * params->wall_height_scale / distance);
    
    return params->horizon_y - screen_offset;
}

int projection_sprite_x_to_screen(const ProjectionParams* params,
                                  float camera_x, float camera_z) {
    if (camera_z <= 0.0f) return -1;  // Behind camera
    
    // Project to screen
    // screen_x = (camera_x / camera_z) * scale + screen_center
    float screen_center = params->screen_width / 2.0f;
    float scale = params->screen_width / 2.0f;
    
    int screen_x = (int)(screen_center + (camera_x / camera_z) * scale);
    
    // Check bounds
    if (screen_x < 0 || screen_x >= params->screen_width) {
        return -1;  // Off-screen
    }
    
    return screen_x;
}
```

### Depth/Distance Utilities

```c
/**
 * Calculate size factor for an object at a given distance
 * @param params Projection parameters
 * @param distance Distance from camera
 * @return Scale factor (1.0 at distance 1.0, decreases with distance)
 */
float projection_size_factor(const ProjectionParams* params, float distance);

/**
 * Check if a distance is within render range
 * @param params Projection parameters
 * @param distance Distance to check
 * @param max_distance Maximum visible distance
 * @return true if should be rendered
 */
bool projection_in_range(const ProjectionParams* params,
                        float distance, float max_distance);
```

**Implementation:**

```c
float projection_size_factor(const ProjectionParams* params, float distance) {
    if (distance < 0.01f) distance = 0.01f;
    return params->wall_height_scale / distance;
}

bool projection_in_range(const ProjectionParams* params,
                        float distance, float max_distance) {
    (void)params;  // Unused
    return distance >= 0.01f && distance <= max_distance;
}
```

## Coordinate Systems

```
Screen Space:           World Space:           Camera Space:
                        
y=0  ┌──────┐           y=0  ┌──────┐               z+
     │      │                 │ ##   │               ↑
     │      │  x+             │      │               │
     │      │ →               │      │  x+           └──→ x+
y=H  └──────┘                 └──────┘ →            (y+ into screen)
                              y=H

Screen: top-left origin   World: top-left origin   Camera: forward is +z
```

## Mathematical Relationships

### Wall Height
```
h = screen_height / distance

Example:
- distance = 1.0, screen_height = 24 → h = 24 pixels (fills screen)
- distance = 2.0, screen_height = 24 → h = 12 pixels (half screen)
- distance = 0.5, screen_height = 24 → h = 48 pixels (clipped)
```

### Vertical Position
```
screen_y = horizon_y - (wall_height / 2) + pixel_offset

Where:
- horizon_y = vertical center of screen
- wall_height / 2 = offset to center wall
- pixel_offset = 0 to wall_height (for texturing)
```

### Floor Distance
```
row_distance = (screen_height / 2) / (screen_y - horizon_y)

This creates perspective: rows near horizon have large distance,
rows near bottom have small distance.
```

## Testing Checklist

- [ ] Wall at distance 1.0 has correct height
- [ ] Walls are centered on horizon
- [ ] Clipping works for very close walls
- [ ] Floor distance increases towards horizon
- [ ] Sprite projection places objects correctly
- [ ] Negative camera_z returns -1 (behind camera)
- [ ] Division by zero is prevented (distance, p)

## Example Calculations

```c
// Screen: 80 columns × 24 rows
ProjectionParams params;
projection_init(&params, 80, 24);
// horizon_y = 12

// Ray hits wall at distance 3.0
RayHit hit = {.hit = true, .distance = 3.0f, /* ... */};
WallProjection proj;
projection_wall(&params, &hit, 40, &proj);

// Expected:
// wall_height = 24 / 3.0 = 8 pixels
// draw_start = 12 - 4 = 8
// draw_end = 12 + 4 = 16
// (wall spans rows 8-16, centered on row 12)
```

## Performance Notes

- All calculations are simple arithmetic (no sqrt, no trig)
- Can be done per-column without caching
- Integer calculations where possible
- Floating-point only for precision in distance calculations

## Integration Notes

- `projection_init()` called once when screen size changes
- `projection_wall()` called once per ray hit
- `projection_floor_row()` called per horizontal span (optional, for floor rendering)
- Results passed to texture system for actual drawing

## Future Enhancements

- Adjustable horizon (for ducking/jumping)
- Variable wall heights
- Sloped floors/ceilings
- Vertical looking (pitch angle)

## Estimated Complexity
- **Lines of Code:** ~150-180
- **Implementation Time:** 1-2 hours
- **Testing Time:** 30 minutes
