#ifndef SNAKE_RENDER_3D_CAMERA_H
#define SNAKE_RENDER_3D_CAMERA_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Camera3D - Advanced first-person camera with interpolation support.
 *
 * Supports smooth rendering at high framerates (60 Hz) while the game updates
 * at low framerates (2 Hz) by interpolating between game state updates.
 *
 * Manages:
 * - Current and previous camera position in world space
 * - Current and previous viewing angle in radians
 * - Direction and plane vectors for ray casting
 * - Field of view and screen dimensions
 * - Interpolation timing for smooth animation
 */

typedef struct {
    /* Current camera state (world space) */
    float x, y;  /* Camera position X, Y */
    float angle; /* View angle in radians (0 = right, π/2 = down) */

    /* Previous camera state (for interpolation) */
    float prev_x, prev_y;
    float prev_angle;

    /* Viewing parameters */
    float fov_radians; /* Field of view in radians (computed from degrees) */
    int screen_width;  /* Number of screen columns (rays to cast) */

    /* Precomputed direction vectors (from angle) */
    float dir_x; /* cos(angle) - direction looking right */
    float dir_y; /* sin(angle) - direction looking down */

    /* Precomputed camera plane vectors (perpendicular to direction) */
    float plane_x; /* Left edge of view frustum */
    float plane_y; /* Right edge of view frustum */

    /* Interpolation state */
    float interp_time;     /* Current time in update interval [0, 1] */
    float update_interval; /* Time between game updates (seconds) */
} Camera3D;

/**
 * Initialize camera with default settings.
 *
 * @param camera          Camera to initialize
 * @param fov_degrees     Field of view in degrees (typically 60-90)
 * @param screen_width    Number of screen columns
 * @param update_interval Time between game updates in seconds (e.g., 0.5 for 2 Hz)
 */
void camera_init(Camera3D* camera, float fov_degrees, int screen_width, float update_interval);

/**
 * Set camera position and angle from player state (game update).
 *
 * This is called when the game updates. The previous state is automatically
 * saved for interpolation purposes.
 *
 * @param camera Camera to update
 * @param x      World X coordinate (grid cell) - will be centered at +0.5
 * @param y      World Y coordinate (grid cell) - will be centered at +0.5
 * @param dir    Player's facing direction (SNAKE_DIR_UP/DOWN/LEFT/RIGHT)
 */
void camera_set_from_player(Camera3D* camera, int x, int y, int dir);

/**
 * Update interpolation time (called once per render frame).
 *
 * Should be called with delta_time from the render loop.
 * Accumulates time and wraps around at update_interval.
 *
 * @param camera      Camera to update
 * @param delta_time  Time since last frame in seconds
 */
void camera_update_interpolation(Camera3D* camera, float delta_time);

/**
 * Set interpolation time explicitly.
 *
 * Useful for testing or syncing with external timing systems.
 *
 * @param camera     Camera to update
 * @param time       Time value in [0, update_interval]
 */
void camera_set_interpolation_time(Camera3D* camera, float time);

/**
 * Get the interpolated ray direction for a screen column.
 *
 * Computes ray angle using interpolated camera state.
 *
 * @param camera         Camera object
 * @param col            Screen column (0 to screen_width-1)
 * @param ray_angle_out  Output: ray angle in radians
 */
void camera_get_ray_angle(const Camera3D* camera, int col, float* ray_angle_out);

/**
 * Transform a world coordinate to interpolated camera space.
 *
 * Camera space: origin at interpolated camera, +X right, +Y down (in screen 2D)
 *
 * @param camera         Camera object
 * @param world_x, world_y  World coordinate to transform
 * @param cam_x_out      Output: camera-space X (screen left/right)
 * @param cam_y_out      Output: camera-space Z (depth, forward)
 */
void camera_world_to_camera(const Camera3D* camera, float world_x, float world_y, float* cam_x_out, float* cam_y_out);

/**
 * Transform camera-space coordinate back to world space.
 *
 * @param camera         Camera object
 * @param cam_x, cam_y   Camera-space coordinate
 * @param world_x_out    Output: world-space X
 * @param world_y_out    Output: world-space Y
 */
void camera_camera_to_world(const Camera3D* camera, float cam_x, float cam_y, float* world_x_out, float* world_y_out);

/**
 * Get distance from interpolated camera to a point.
 *
 * @param camera Camera object
 * @param x      World X coordinate
 * @param y      World Y coordinate
 * @return       Euclidean distance
 */
float camera_distance_to_point(const Camera3D* camera, float x, float y);

/**
 * Check if a point is in front of interpolated camera.
 *
 * @param camera Camera object
 * @param x      World X coordinate
 * @param y      World Y coordinate
 * @return       true if point is in front (positive Z in camera space)
 */
bool camera_point_in_front(const Camera3D* camera, float x, float y);

/**
 * Get current interpolated camera position.
 *
 * @param camera    Camera object
 * @param x_out     Output: current X position
 * @param y_out     Output: current Y position
 */
void camera_get_interpolated_position(const Camera3D* camera, float* x_out, float* y_out);

/**
 * Get current interpolated camera angle.
 *
 * @param camera Camera object
 * @return       Interpolated angle in radians
 */
float camera_get_interpolated_angle(const Camera3D* camera);

#endif /* SNAKE_RENDER_3D_CAMERA_H */
