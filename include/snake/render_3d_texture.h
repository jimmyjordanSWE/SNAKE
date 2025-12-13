#ifndef SNAKE_RENDER_3D_TEXTURE_H
#define SNAKE_RENDER_3D_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Texture3D - Shading, coloring, and character selection for walls.
 *
 * Manages:
 * - Distance-based shade level selection
 * - Wall side variation (vertical vs horizontal)
 * - Character pattern selection per shade
 * - Color mapping per shade
 */

/* Shade level: 0 = nearest, higher = farther */
#define TEXTURE_MAX_SHADES 6

typedef struct {
    uint32_t color; /* ARGB8888 color for pixel rendering */
} Texel;

/**
 * Texture context (holds shade patterns and colors).
 */
typedef struct {
    /* ARGB8888 colors per shade level [0=near, MAX_SHADES-1=far] */
    uint32_t shade_colors[TEXTURE_MAX_SHADES];

    /* Alternative colors for wall sides (texture variation) */
    uint32_t side_colors[2][TEXTURE_MAX_SHADES]; /* [0]=vertical, [1]=horizontal */
} Texture3D;

/**
 * Initialize texture system with default shade patterns and colors.
 *
 * @param tex   Texture context to initialize
 */
void texture_init(Texture3D* tex);

/**
 * Determine shade level based on distance.
 *
 * Maps continuous distance to discrete shade level [0, TEXTURE_MAX_SHADES).
 *
 * @param distance  Perpendicular distance from camera
 * @return          Shade level [0, TEXTURE_MAX_SHADES)
 */
uint8_t texture_shade_from_distance(float distance);

/**
 * Get texel (character + color) for given wall parameters.
 *
 * @param tex           Texture context
 * @param distance      Perpendicular distance from camera
 * @param is_vertical   true = vertical wall, false = horizontal
 * @param tex_coord     Texture coordinate (0.0-1.0) along wall surface
 * @param texel_out     Output texel (character + color)
 */
void texture_get_texel(const Texture3D* tex, float distance, bool is_vertical, float tex_coord, Texel* texel_out);

/**
 * Customize shade character set.
 *
 * Default is: █ ▓ ▒ ░ · (space)
 *
 * @param tex           Texture context
 * @param chars         Array of TEXTURE_MAX_SHADES characters
 */
void texture_set_shade_chars(Texture3D* tex, const char* chars);

/**
 * Customize shade color set.
 *
 * @param tex           Texture context
 * @param colors        Array of TEXTURE_MAX_SHADES color codes
 */
void texture_set_shade_colors(Texture3D* tex, const uint32_t* colors);

#endif
