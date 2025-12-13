#ifndef SNAKE_RENDER_3D_SPRITE_H
#define SNAKE_RENDER_3D_SPRITE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Sprite3D - Rendering entities (snakes, food) as camera-facing billboards.
 *
 * Manages:
 * - Transform world positions to screen coordinates
 * - Depth sorting (Z-buffering)
 * - Billboard size scaling based on distance
 * - Visibility testing
 */

typedef struct {
    float world_x;
    float world_y;
    float screen_x;      /* Computed screen X (-1.0 = left, 1.0 = right) */
    float screen_y;      /* Computed screen Y (-1.0 = top, 1.0 = bottom) */
    float screen_z;      /* Depth for sorting (distance from camera) */
    float screen_width;  /* Width as fraction of screen */
    float screen_height; /* Height as fraction of screen */
    uint8_t entity_type; /* 0=snake, 1=food, etc. */
    uint8_t entity_id;   /* ID within entity type */
    bool is_visible;     /* true if within screen bounds */
} SpriteProjection;

/**
 * Sprite renderer context.
 */
typedef struct {
    /* Projected sprite list (sorted by depth) */
    SpriteProjection* sprites;
    int max_sprites;
    int sprite_count;

    /* Camera reference for transforms (not owned) */
    const void* camera; /* Points to Camera3D, but avoid circular include */
} SpriteRenderer3D;

/**
 * Initialize sprite renderer.
 *
 * @param sr            SpriteRenderer3D context to initialize
 * @param max_sprites   Maximum number of sprites to track
 * @param camera        Pointer to Camera3D for world-to-screen transforms
 */
void sprite_init(SpriteRenderer3D* sr, int max_sprites, const void* camera);

/**
 * Clear sprite list for new frame.
 */
void sprite_clear(SpriteRenderer3D* sr);

/**
 * Add a sprite to render.
 *
 * @param sr            SpriteRenderer3D context
 * @param world_x, world_y  World position
 * @param entity_type   Entity type (0=snake segment, 1=food, etc.)
 * @param entity_id     Entity ID (for sorting/identification)
 * @return              true if added successfully, false if list full
 */
bool sprite_add(SpriteRenderer3D* sr, float world_x, float world_y, uint8_t entity_type, uint8_t entity_id);

/**
 * Sort sprites by depth (painter's algorithm: far to near).
 *
 * Call this after adding all sprites for frame, before rendering.
 */
void sprite_sort_by_depth(SpriteRenderer3D* sr);

/**
 * Get projected sprite by index (after sorting).
 *
 * @param sr    SpriteRenderer3D context
 * @param idx   Sprite index (0 to sprite_count-1)
 * @return      Pointer to SpriteProjection, or NULL if out of bounds
 */
const SpriteProjection* sprite_get(const SpriteRenderer3D* sr, int idx);

/**
 * Get number of visible sprites.
 */
int sprite_get_count(const SpriteRenderer3D* sr);

/**
 * Shutdown sprite renderer and free resources.
 */
void sprite_shutdown(SpriteRenderer3D* sr);

#endif
