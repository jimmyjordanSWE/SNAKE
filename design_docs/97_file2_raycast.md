# File 2: Raycasting Core Implementation

## Purpose
Implement the DDA (Digital Differential Analyzer) raycasting algorithm to find wall intersections and compute distances. This is the heart of the pseudo-3D rendering system.

## File Locations
- Header: `include/snake/raycast.h`
- Source: `src/render3d/raycast.c`

## Dependencies
- `snake/types.h` - For SnakePoint
- `snake/game.h` - For GameState (world data)
- `snake/camera.h` - For Camera3D
- `<math.h>` - For sqrt, fabs

## Data Structures

```c
typedef enum {
    WALL_SIDE_NORTH = 0,  // Hit from south (wall faces north, dy < 0)
    WALL_SIDE_SOUTH,      // Hit from north (wall faces south, dy > 0)
    WALL_SIDE_EAST,       // Hit from west (wall faces east, dx > 0)
    WALL_SIDE_WEST,       // Hit from east (wall faces west, dx < 0)
} WallSide;

typedef enum {
    WALL_TYPE_BORDER = 0,  // Board boundary wall
    WALL_TYPE_NONE,        // No wall (empty space)
} WallType;

typedef struct {
    bool hit;              // Did ray hit a wall?
    
    // Hit position
    int map_x;             // Grid X coordinate of hit
    int map_y;             // Grid Y coordinate of hit
    
    // Distance (perpendicular to camera plane, fish-eye corrected)
    float distance;
    
    // Wall properties
    WallSide side;         // Which side of the wall was hit
    WallType type;         // Type of wall hit
    
    // Texture coordinate (0.0 to 1.0 along the wall)
    float wall_x;          // Where exactly the wall was hit
} RayHit;
```

## Public API

```c
/**
 * Cast a ray and find the first wall intersection
 * @param game Game state (provides world boundaries)
 * @param camera Camera position and direction (for fish-eye correction)
 * @param ray_dir_x Ray direction X component (normalized)
 * @param ray_dir_y Ray direction Y component (normalized)
 * @param max_distance Maximum ray distance (to prevent infinite loops)
 * @param hit Output: ray hit information
 */
void raycast(const GameState* game, const Camera3D* camera,
             float ray_dir_x, float ray_dir_y,
             float max_distance, RayHit* hit);

/**
 * Check if a grid cell is a wall
 * @param game Game state
 * @param map_x Grid X coordinate
 * @param map_y Grid Y coordinate
 * @return Wall type (WALL_TYPE_BORDER if wall, WALL_TYPE_NONE if empty)
 */
WallType check_wall(const GameState* game, int map_x, int map_y);
```

## Algorithm: DDA Raycasting

### Overview
DDA steps through the grid line-by-line, checking each cell for walls. It's efficient because it only checks cells the ray actually crosses.

### Mathematical Foundation

```
Ray equation: P(t) = origin + t * direction
            = (cam_x, cam_y) + t * (ray_dir_x, ray_dir_y)

Grid cell: floor(P(t))

DDA steps to each grid line intersection
```

### Step-by-Step Algorithm

```c
void raycast(const GameState* game, const Camera3D* camera,
             float ray_dir_x, float ray_dir_y,
             float max_distance, RayHit* hit) {
    
    // Initialize hit as miss
    hit->hit = false;
    hit->distance = max_distance;
    hit->type = WALL_TYPE_NONE;
    
    // Starting position
    float pos_x = camera->x;
    float pos_y = camera->y;
    
    // Current map cell
    int map_x = (int)pos_x;
    int map_y = (int)pos_y;
    
    // Length of ray from one grid line to next
    float delta_dist_x = (ray_dir_x == 0) ? 1e30f : fabsf(1.0f / ray_dir_x);
    float delta_dist_y = (ray_dir_y == 0) ? 1e30f : fabsf(1.0f / ray_dir_y);
    
    // Step direction (-1 or +1)
    int step_x, step_y;
    
    // Distance from start to first grid line
    float side_dist_x, side_dist_y;
    
    // Calculate step and initial side_dist
    if (ray_dir_x < 0) {
        step_x = -1;
        side_dist_x = (pos_x - map_x) * delta_dist_x;
    } else {
        step_x = 1;
        side_dist_x = (map_x + 1.0f - pos_x) * delta_dist_x;
    }
    
    if (ray_dir_y < 0) {
        step_y = -1;
        side_dist_y = (pos_y - map_y) * delta_dist_y;
    } else {
        step_y = 1;
        side_dist_y = (map_y + 1.0f - pos_y) * delta_dist_y;
    }
    
    // DDA loop
    int max_steps = (int)(max_distance * 2) + 10;  // Safety limit
    WallSide side = WALL_SIDE_NORTH;
    
    for (int i = 0; i < max_steps; i++) {
        // Check if current cell is a wall
        WallType wall = check_wall(game, map_x, map_y);
        if (wall != WALL_TYPE_NONE) {
            // Hit a wall!
            hit->hit = true;
            hit->map_x = map_x;
            hit->map_y = map_y;
            hit->type = wall;
            hit->side = side;
            
            // Calculate distance (perpendicular to camera plane)
            float perp_wall_dist;
            if (side == WALL_SIDE_EAST || side == WALL_SIDE_WEST) {
                perp_wall_dist = side_dist_x - delta_dist_x;
            } else {
                perp_wall_dist = side_dist_y - delta_dist_y;
            }
            
            hit->distance = perp_wall_dist;
            
            // Calculate wall_x (where on the wall we hit, 0-1)
            float wall_hit_x, wall_hit_y;
            if (side == WALL_SIDE_EAST || side == WALL_SIDE_WEST) {
                wall_hit_y = pos_y + perp_wall_dist * ray_dir_y;
                hit->wall_x = wall_hit_y - floorf(wall_hit_y);
            } else {
                wall_hit_x = pos_x + perp_wall_dist * ray_dir_x;
                hit->wall_x = wall_hit_x - floorf(wall_hit_x);
            }
            
            return;
        }
        
        // Step to next grid cell
        if (side_dist_x < side_dist_y) {
            side_dist_x += delta_dist_x;
            map_x += step_x;
            side = (step_x > 0) ? WALL_SIDE_EAST : WALL_SIDE_WEST;
        } else {
            side_dist_y += delta_dist_y;
            map_y += step_y;
            side = (step_y > 0) ? WALL_SIDE_SOUTH : WALL_SIDE_NORTH;
        }
        
        // Safety check: ray went too far
        float traveled = (side_dist_x < side_dist_y) 
                        ? side_dist_x - delta_dist_x 
                        : side_dist_y - delta_dist_y;
        if (traveled > max_distance) {
            break;
        }
    }
    
    // No hit found
    hit->hit = false;
}
```

### Wall Detection

```c
WallType check_wall(const GameState* game, int map_x, int map_y) {
    // Check if outside board boundaries
    if (map_x < 0 || map_x >= game->width ||
        map_y < 0 || map_y >= game->height) {
        return WALL_TYPE_BORDER;
    }
    
    // Check if on board edge (border walls)
    if (map_x == 0 || map_x == game->width - 1 ||
        map_y == 0 || map_y == game->height - 1) {
        return WALL_TYPE_BORDER;
    }
    
    // Interior cells are empty (no walls in Snake game)
    return WALL_TYPE_NONE;
}
```

## Fish-Eye Correction

**Problem:** Using Euclidean distance causes barrel distortion (fish-eye effect).

**Solution:** Use perpendicular distance to camera plane.

```
Euclidean distance: sqrt((hit_x - cam_x)² + (hit_y - cam_y)²)
Perpendicular distance: (hit_point - cam_pos) · ray_dir / |ray_dir|

The DDA algorithm already computes perpendicular distance via side_dist!
```

This is why we use:
```c
perp_wall_dist = side_dist_x - delta_dist_x;  // or side_dist_y - delta_dist_y
```

## Optimization Notes

1. **Early Exit:** Return immediately when wall is found
2. **Integer Grid Stepping:** DDA only checks integer grid cells
3. **Avoid sqrt:** Perpendicular distance calculation avoids expensive sqrt
4. **Max Distance:** Prevents infinite loops and improves performance

## Edge Cases to Handle

- [ ] Ray direction is exactly 0 in X or Y (handled by 1e30f)
- [ ] Camera starts exactly on grid line
- [ ] Ray shoots parallel to walls
- [ ] Maximum distance reached without hit
- [ ] Negative coordinates (outside board)

## Testing Checklist

- [ ] Ray hitting north-facing wall sets WALL_SIDE_NORTH
- [ ] Ray hitting east-facing wall sets WALL_SIDE_EAST
- [ ] Distance is perpendicular (no fish-eye)
- [ ] wall_x ranges from 0.0 to 1.0
- [ ] Rays at 45° work correctly
- [ ] Camera at edge of board works
- [ ] Max distance prevents infinite loops

## Example Test Cases

```c
// Test 1: Ray straight ahead hits wall
Camera: (5.5, 5.5), angle=0°, FOV=60°
Ray: dir=(1, 0)
Expected: hit at map_x=39 (right border), side=WALL_SIDE_EAST

// Test 2: Ray at 45° to corner
Camera: (5.5, 5.5), angle=45°
Ray: dir=(0.707, 0.707)
Expected: hit at corner, consistent distance

// Test 3: Ray backwards
Camera: (5.5, 5.5), angle=0°
Ray: dir=(-1, 0)
Expected: hit at map_x=0 (left border), side=WALL_SIDE_WEST
```

## Performance Characteristics

- **Best Case:** O(1) - wall is immediately adjacent
- **Average Case:** O(n) where n = distance to wall in grid cells
- **Worst Case:** O(max_distance * 2) - ray travels maximum distance

For typical Snake board (40x20), average ray traverses 5-10 cells.

## Integration Notes

- Call `raycast()` once per screen column
- Store RayHit results in array for later sprite rendering (Z-buffer)
- Consider caching rays for static geometry (not needed for Snake)

## Estimated Complexity
- **Lines of Code:** ~120-150
- **Implementation Time:** 2-3 hours
- **Testing Time:** 1 hour
