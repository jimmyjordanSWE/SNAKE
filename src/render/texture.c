#include "snake/render_3d_texture.h"
#include <stdbool.h>
#include <string.h>
void texture_init(Texture3D* tex) {
if(!tex) return;
const uint32_t default_shades[TEXTURE_MAX_SHADES]= {0xFFEEEEEE, 0xFFCCCCCC, 0xFF999999, 0xFF666666, 0xFF333333, 0xFF000000};
memcpy(tex->shade_colors, default_shades, sizeof(tex->shade_colors));
/* Provide side colors for vertical and horizontal faces to avoid single-column
   bright seams when rays align with grid axes.
*/
const uint32_t side_v[TEXTURE_MAX_SHADES] = {0xFFDDDDDD, 0xFFBFBFBF, 0xFF8F8F8F, 0xFF606060, 0xFF303030, 0xFF000000};
const uint32_t side_h[TEXTURE_MAX_SHADES] = {0xFFCCCCCC, 0xFFAAAAAA, 0xFF777777, 0xFF505050, 0xFF282828, 0xFF000000};
memcpy(tex->side_colors[0], side_v, sizeof(side_v));
memcpy(tex->side_colors[1], side_h, sizeof(side_h));
}
uint8_t texture_shade_from_distance(float distance) {
if(distance < 3.0f) return 0;
if(distance < 6.0f) return 1;
if(distance < 9.0f) return 2;
if(distance < 12.0f) return 3;
if(distance < 15.0f) return 4;
return TEXTURE_MAX_SHADES - 1;
}
void texture_get_texel(const Texture3D* tex, float distance, bool is_vertical, float tex_coord, Texel* texel_out) {
(void)tex_coord;
if(!tex || !texel_out) return;
uint8_t shade= texture_shade_from_distance(distance);
if(shade >= TEXTURE_MAX_SHADES) shade= TEXTURE_MAX_SHADES - 1;
if(is_vertical && tex->side_colors[0][shade])
texel_out->color= tex->side_colors[0][shade];
else if(!is_vertical && tex->side_colors[1][shade])
texel_out->color= tex->side_colors[1][shade];
else
texel_out->color= tex->shade_colors[shade];
}
void texture_set_shade_colors(Texture3D* tex, const uint32_t* colors) {
if(!tex || !colors) return;
memcpy(tex->shade_colors, colors, TEXTURE_MAX_SHADES * sizeof(uint32_t));
}
