#pragma once
#include <stdbool.h>
#include <stdint.h>
#define TEXTURE_MAX_SHADES 6
typedef struct {
uint32_t color;
} Texel;
#define TEXTURE_SCALE 4
typedef struct Texture3D Texture3D;
// Returns a newly allocated Texture3D; caller must call texture_destroy()
Texture3D* texture_create(void);
void texture_destroy(Texture3D* tex);
void texture_init(Texture3D* tex);
uint8_t texture_shade_from_distance(float distance);
void texture_get_texel(const Texture3D* tex, float distance, bool is_vertical, float tex_coord, Texel* texel_out);
void texture_set_shade_chars(Texture3D* tex, const char* chars);
void texture_set_shade_colors(Texture3D* tex, const uint32_t* colors);
bool texture_load_from_file(Texture3D* tex, const char* filename);
void texture_free_image(Texture3D* tex);
uint32_t texture_sample(const Texture3D* tex, float u, float v, bool bilinear);
const uint32_t* texture_get_pixels(const Texture3D* tex);
int texture_get_img_w(const Texture3D* tex);
int texture_get_img_h(const Texture3D* tex);
bool texture_has_image(const Texture3D* tex);