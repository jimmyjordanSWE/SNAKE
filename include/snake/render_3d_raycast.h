#ifndef SNAKE_RENDER_3D_RAYCAST_H
#define SNAKE_RENDER_3D_RAYCAST_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Raycaster3D - DDA-based raycasting for wall detection.
 *
 * Casts rays from camera position and detects intersections with game world walls.
 * Uses Digital Differential Analyzer (DDA) algorithm for efficient line traversal.
 *
 * Manages:
 * - Ray-wall intersection testing
 * - Perpendicular distance calculation (fish-eye correction)
 * - Wall side determination (vertical or horizontal)
 * - Texture coordinate mapping
 */

typedef struct {
    float distance;      /* Perpendicular distance from camera to wall */
    float hit_x;         /* World X coordinate of wall hit point */
    float hit_y;         /* World Y coordinate of wall hit point */
    bool is_vertical;    /* true = vertical wall, false = horizontal wall */
    uint8_t shade_level; /* Computed shade level based on distance */
} RayHit;

/**
 * Raycaster context (holds world geometry reference).
 * Initialized with game board dimensions.
 */
typedef struct {
    int board_width;
    int board_height;
    /* Pointer to board grid (wall data) - set by caller */
    const uint8_t* board;
} Raycaster3D;

/**
 * Initialize raycaster with board dimensions.
 *
 * @param rc            Raycaster context to initialize
 * @param width, height Board dimensions in cells
 * @param board         Pointer to board grid (width * height bytes)
 */
void raycast_init(Raycaster3D* rc, int width, int height, const uint8_t* board);

/**
 * Cast a ray from camera position at given angle and detect wall intersection.
 *
 * Uses DDA algorithm to efficiently traverse grid cells until a wall is hit.
 * Computes perpendicular distance (corrected for fish-eye effect).
 *
 * @param rc            Raycaster context
 * @param camera_x, camera_y  Camera position in world space
 * @param ray_angle     Ray direction in radians
 * @param hit_out       Pointer to RayHit struct to fill with result
 * @return              true if wall hit, false if ray escapes world
 */
bool raycast_cast_ray(const Raycaster3D* rc, float camera_x, float camera_y, float ray_angle, RayHit* hit_out);

/**
 * Check if a world coordinate is a wall cell.
 *
 * @param rc    Raycaster context
 * @param x, y  World coordinate (will be truncated to grid)
 * @return      true if coordinate is a wall
 */
bool raycast_is_wall(const Raycaster3D* rc, float x, float y);

/**
 * Get the texture coordinate (0.0-1.0) on the hit wall surface.
 *
 * Used to determine which character to draw on vertical spans.
 *
 * @param hit           Result from raycast_cast_ray()
 * @param is_vertical   Whether the hit is on a vertical or horizontal wall
 * @return              Normalized texture coordinate (0.0 = left/top, 1.0 = right/bottom)
 */
float raycast_get_texture_coord(const RayHit* hit, bool is_vertical);

#endif
