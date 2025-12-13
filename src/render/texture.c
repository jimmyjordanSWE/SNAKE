#include "snake/render_3d_texture.h"

#include <stdbool.h>
#include <string.h>

void texture_init(Texture3D* tex) {
    if (!tex) return;

    /* Default shade colors: from bright to dim (ARGB8888) */
    const uint32_t default_shades[TEXTURE_MAX_SHADES] = {
        0xFFFFFFFF, /* White (near) */
        0xFFCCCCCC, /* Light gray */
        0xFF999999, /* Gray */
        0xFF666666, /* Dark gray */
        0xFF333333, /* Very dark gray */
        0xFF000000  /* Black (far) */
    };
    memcpy(tex->shade_colors, default_shades, sizeof(tex->shade_colors));

    /* TODO: Initialize side colors if needed */
}

uint8_t texture_shade_from_distance(float distance) {
    /* Map continuous distance to shade level */
    if (distance < 3.0f) return 0;
    if (distance < 6.0f) return 1;
    if (distance < 9.0f) return 2;
    if (distance < 12.0f) return 3;
    if (distance < 15.0f) return 4;
    return TEXTURE_MAX_SHADES - 1;
}
void texture_get_texel(const Texture3D* tex, float distance, bool is_vertical, float tex_coord, Texel* texel_out) {
    (void)tex_coord; /* Currently unused, may be needed for future texture mapping */
    if (!tex || !texel_out) return;

    uint8_t shade = texture_shade_from_distance(distance);

    if (shade >= TEXTURE_MAX_SHADES) { shade = TEXTURE_MAX_SHADES - 1; }

    /* Use wall-side variation if available */
    if (is_vertical && tex->side_colors[0][shade]) {
        texel_out->color = tex->side_colors[0][shade];
    } else if (!is_vertical && tex->side_colors[1][shade]) {
        texel_out->color = tex->side_colors[1][shade];
    } else {
        texel_out->color = tex->shade_colors[shade];
    }
}

void texture_set_shade_colors(Texture3D* tex, const uint32_t* colors) {
    if (!tex || !colors) return;
    memcpy(tex->shade_colors, colors, TEXTURE_MAX_SHADES * sizeof(uint32_t));
}
