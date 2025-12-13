#ifndef SNAKE_RENDER_3D_PROJECTION_H
#define SNAKE_RENDER_3D_PROJECTION_H

#include <stdint.h>

/**
 * Projection3D - Screen-space projection and geometry calculations.
 *
 * Converts raycasting results (distance) into screen-space geometry.
 *
 * Manages:
 * - Distance to wall height conversion
 * - Screen clipping (culling invisible portions)
 * - Vertical centering (horizon line)
 * - Texture coordinate mapping
 */

typedef struct {
    int draw_start;      /* Screen Y where wall starts to draw */
    int draw_end;        /* Screen Y where wall ends to draw */
    int wall_height;     /* Height in screen pixels */
    float texture_scale; /* Texture coordinate scale factor */
} WallProjection;

/**
 * Projection context (holds screen dimensions).
 */
typedef struct {
    int screen_width;
    int screen_height;
    float fov_radians;
} Projection3D;

/**
 * Initialize projection system.
 *
 * @param proj          Projection context to initialize
 * @param screen_width  Screen width in columns
 * @param screen_height Screen height in rows
 * @param fov_radians   Field of view in radians
 */
void projection_init(Projection3D* proj, int screen_width, int screen_height, float fov_radians);

/**
 * Project a wall hit result to screen-space geometry.
 *
 * Converts perpendicular distance to wall heights on screen,
 * with vertical centering around horizon line.
 *
 * @param proj          Projection context
 * @param distance      Perpendicular distance from camera to wall
 * @param result_out    WallProjection result
 */
void projection_project_wall(const Projection3D* proj, float distance, WallProjection* result_out);

/**
 * Get the world distance corresponding to a single screen pixel height.
 *
 * Used for scaling sprites and other entities.
 *
 * @param proj      Projection context
 * @param distance  Distance at which to measure (affects scale)
 * @return          Approximate world distance per pixel
 */
float projection_world_distance_per_pixel(const Projection3D* proj, float distance);

/**
 * Set the horizon line position (vertical center offset).
 *
 * Allows tilting camera up/down. Default is screen_height / 2.
 *
 * @param proj          Projection context
 * @param horizon_y     Screen Y coordinate for horizon line
 */
void projection_set_horizon(Projection3D* proj, int horizon_y);

#endif // SNAKE_RENDER_3D_PROJECTION_H
