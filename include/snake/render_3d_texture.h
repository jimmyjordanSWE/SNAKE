#pragma once
#include <stdbool.h>
#include <stdint.h>
#define TEXTURE_MAX_SHADES 6
typedef struct
{
    uint32_t color;
} Texel;
#define TEXTURE_SCALE 4
typedef struct
{
    uint32_t shade_colors[TEXTURE_MAX_SHADES];
    uint32_t side_colors[2][TEXTURE_MAX_SHADES];
    /* optional image data (RGBA, row-major) */
    uint32_t* pixels;
    int       img_w;
    int       img_h;
} Texture3D;
void    texture_init(Texture3D* tex);
uint8_t texture_shade_from_distance(float distance);
void    texture_get_texel(const Texture3D* tex,
                          float            distance,
                          bool             is_vertical,
                          float            tex_coord,
                          Texel*           texel_out);
void    texture_set_shade_chars(Texture3D* tex, const char* chars);
void    texture_set_shade_colors(Texture3D* tex, const uint32_t* colors);
/* Image loading */
bool texture_load_from_file(Texture3D* tex, const char* filename);
void texture_free_image(Texture3D* tex);
/* direct sampling */
uint32_t texture_sample(const Texture3D* tex, float u, float v, bool bilinear);