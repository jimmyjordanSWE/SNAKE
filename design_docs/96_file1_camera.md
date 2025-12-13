# File 1: Camera System Implementation

## Purpose
Manage the 3D camera position, orientation, and view frustum calculations. The camera follows the active player and provides transformation utilities.

## File Locations
- Header: `include/snake/camera.h`
- Source: `src/render3d/camera.c`

## Dependencies
- `snake/types.h` - For SnakePoint
- `<math.h>` - For trigonometry

## Data Structures

```c
typedef struct {
    // Camera position in world space (matches player position)
    float x;
    float y;
    
    // Viewing angle in radians (0 = right, π/2 = down, π = left, 3π/2 = up)
    float angle;
    
    // Field of view in radians (typically π/3 for 60 degrees)
    float fov;
    
    // Direction vectors (computed from angle)
    float dir_x;  // cos(angle)
    float dir_y;  // sin(angle)
    
    // Camera plane vectors (perpendicular to direction, defines FOV)
    float plane_x;
    float plane_y;
    
    // Projection distance (computed from FOV)
    float projection_dist;
} Camera3D;
```

## Public API

### Initialization
```c
/**
 * Initialize camera with default settings
 * @param camera Camera to initialize
 * @param fov_degrees Field of view in degrees (typically 60-90)
 */
void camera_init(Camera3D* camera, float fov_degrees);
```

**Implementation Notes:**
- Convert FOV from degrees to radians
- Set default position (0, 0) and angle (0)
- Compute initial direction and plane vectors
- Calculate projection distance: `screen_width / (2 * tan(fov/2))`

### Position & Orientation
```c
/**
 * Set camera position and angle from player state
 * @param camera Camera to update
 * @param x World X coordinate (player position)
 * @param y World Y coordinate (player position)
 * @param dir Player's facing direction (SNAKE_DIR_UP/DOWN/LEFT/RIGHT)
 */
void camera_set_from_player(Camera3D* camera, int x, int y, SnakeDir dir);

/**
 * Update camera angle
 * @param camera Camera to update
 * @param angle_radians New angle in radians
 */
void camera_set_angle(Camera3D* camera, float angle_radians);

/**
 * Rotate camera by delta angle
 * @param camera Camera to update
 * @param delta_radians Rotation amount (positive = clockwise)
 */
void camera_rotate(Camera3D* camera, float delta_radians);
```

**Implementation Notes for `camera_set_from_player`:**
- Add 0.5 to x,y to center camera in grid cell
- Convert SnakeDir to angle:
  - `SNAKE_DIR_RIGHT` → 0.0
  - `SNAKE_DIR_DOWN` → π/2
  - `SNAKE_DIR_LEFT` → π
  - `SNAKE_DIR_UP` → 3π/2 or -π/2
- Call `camera_set_angle()` to update vectors

**Implementation Notes for `camera_set_angle`:**
- Normalize angle to [0, 2π)
- Compute direction: `dir_x = cos(angle)`, `dir_y = sin(angle)`
- Compute camera plane (perpendicular to direction):
  - `plane_x = -sin(angle) * tan(fov/2)`
  - `plane_y = cos(angle) * tan(fov/2)`

### Ray Generation
```c
/**
 * Generate a ray for a screen column
 * @param camera Camera parameters
 * @param screen_x Screen column (0 to screen_width-1)
 * @param screen_width Total screen width in columns
 * @param ray_dir_x Output: ray direction X component
 * @param ray_dir_y Output: ray direction Y component
 */
void camera_get_ray(const Camera3D* camera, int screen_x, int screen_width,
                    float* ray_dir_x, float* ray_dir_y);
```

**Implementation Notes:**
- Calculate camera space X: `camera_x = 2 * screen_x / (float)screen_width - 1`
  - This maps [0, screen_width-1] to [-1, 1]
- Ray direction:
  - `ray_dir_x = camera->dir_x + camera->plane_x * camera_x`
  - `ray_dir_y = camera->dir_y + camera->plane_y * camera_x`

### Utility Functions
```c
/**
 * Get distance from camera to a point (2D distance)
 * @param camera Camera position
 * @param x World X coordinate
 * @param y World Y coordinate
 * @return Euclidean distance
 */
float camera_distance_to_point(const Camera3D* camera, float x, float y);

/**
 * Check if a point is in front of the camera (positive Z in camera space)
 * @param camera Camera parameters
 * @param x World X coordinate
 * @param y World Y coordinate
 * @return true if point is in front
 */
bool camera_point_in_front(const Camera3D* camera, float x, float y);

/**
 * Transform world point to camera space
 * @param camera Camera parameters
 * @param world_x World X coordinate
 * @param world_y World Y coordinate
 * @param cam_x Output: camera space X (left/right)
 * @param cam_z Output: camera space Z (depth, forward)
 */
void camera_world_to_camera(const Camera3D* camera, float world_x, float world_y,
                            float* cam_x, float* cam_z);
```

**Implementation Notes for `camera_world_to_camera`:**
- Translate to camera origin:
  - `dx = world_x - camera->x`
  - `dy = world_y - camera->y`
- Rotate into camera space:
  - `*cam_x = -dy * camera->dir_x + dx * camera->dir_y`
  - `*cam_z = dx * camera->dir_x + dy * camera->dir_y`

## Implementation Example

```c
// camera.c skeleton
#include "snake/camera.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void camera_init(Camera3D* camera, float fov_degrees) {
    camera->x = 0.0f;
    camera->y = 0.0f;
    camera->fov = fov_degrees * M_PI / 180.0f;
    camera_set_angle(camera, 0.0f);
}

void camera_set_angle(Camera3D* camera, float angle_radians) {
    // Normalize to [0, 2π)
    camera->angle = fmod(angle_radians, 2.0f * M_PI);
    if (camera->angle < 0) camera->angle += 2.0f * M_PI;
    
    // Direction vector
    camera->dir_x = cosf(camera->angle);
    camera->dir_y = sinf(camera->angle);
    
    // Camera plane (perpendicular, scaled by FOV)
    float plane_len = tanf(camera->fov * 0.5f);
    camera->plane_x = -sinf(camera->angle) * plane_len;
    camera->plane_y = cosf(camera->angle) * plane_len;
    
    // Projection distance (for reference, not always needed)
    camera->projection_dist = 1.0f / tanf(camera->fov * 0.5f);
}

void camera_set_from_player(Camera3D* camera, int x, int y, SnakeDir dir) {
    // Center camera in grid cell
    camera->x = x + 0.5f;
    camera->y = y + 0.5f;
    
    // Convert direction to angle
    float angle;
    switch (dir) {
        case SNAKE_DIR_RIGHT: angle = 0.0f; break;
        case SNAKE_DIR_DOWN:  angle = M_PI * 0.5f; break;
        case SNAKE_DIR_LEFT:  angle = M_PI; break;
        case SNAKE_DIR_UP:    angle = M_PI * 1.5f; break;
        default: angle = 0.0f; break;
    }
    
    camera_set_angle(camera, angle);
}

void camera_rotate(Camera3D* camera, float delta_radians) {
    camera_set_angle(camera, camera->angle + delta_radians);
}

void camera_get_ray(const Camera3D* camera, int screen_x, int screen_width,
                   float* ray_dir_x, float* ray_dir_y) {
    // Map screen_x to [-1, 1]
    float camera_x = 2.0f * screen_x / (float)screen_width - 1.0f;
    
    // Ray direction
    *ray_dir_x = camera->dir_x + camera->plane_x * camera_x;
    *ray_dir_y = camera->dir_y + camera->plane_y * camera_x;
}

float camera_distance_to_point(const Camera3D* camera, float x, float y) {
    float dx = x - camera->x;
    float dy = y - camera->y;
    return sqrtf(dx * dx + dy * dy);
}

bool camera_point_in_front(const Camera3D* camera, float x, float y) {
    float dx = x - camera->x;
    float dy = y - camera->y;
    float dot = dx * camera->dir_x + dy * camera->dir_y;
    return dot > 0.0f;
}

void camera_world_to_camera(const Camera3D* camera, float world_x, float world_y,
                           float* cam_x, float* cam_z) {
    float dx = world_x - camera->x;
    float dy = world_y - camera->y;
    
    // Rotate into camera space (camera looks down +Z axis)
    *cam_x = -dy * camera->dir_x + dx * camera->dir_y;
    *cam_z = dx * camera->dir_x + dy * camera->dir_y;
}
```

## Testing Checklist

- [ ] Camera initializes with correct FOV and vectors
- [ ] Direction vectors are unit length
- [ ] Plane vectors are perpendicular to direction
- [ ] `camera_set_from_player` correctly maps SNAKE_DIR to angles
- [ ] Ray generation produces correct spread for FOV
- [ ] World-to-camera transform is correct (test with known points)
- [ ] Rotation wraps angle correctly to [0, 2π)

## Integration Notes

- Camera should be updated once per frame before rendering
- For smooth rotation (future enhancement), interpolate angle
- Consider adding camera offset for third-person view
- FOV typically set to 60-70 degrees for comfortable viewing

## Estimated Complexity
- **Lines of Code:** ~150-200
- **Implementation Time:** 1-2 hours
- **Testing Time:** 30 minutes
