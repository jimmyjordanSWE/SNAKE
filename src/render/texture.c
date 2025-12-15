#include "../vendor/stb_image.h"
#include "snake/render_3d_raycast.h"
#include "snake/render_3d_texture.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void texture_init(Texture3D* tex) {
if(!tex) return;
const uint32_t default_shades[TEXTURE_MAX_SHADES]= {0xFFEEEEEE, 0xFFCCCCCC, 0xFF999999, 0xFF666666, 0xFF333333, 0xFF000000};
memcpy(tex->shade_colors, default_shades, sizeof(tex->shade_colors));
/* Provide side colors for vertical and horizontal faces to avoid
           single-column bright seams when rays align with grid axes.
        */
const uint32_t side_v[TEXTURE_MAX_SHADES]= {0xFFDDDDDD, 0xFFBFBFBF, 0xFF8F8F8F, 0xFF606060, 0xFF303030, 0xFF000000};
const uint32_t side_h[TEXTURE_MAX_SHADES]= {0xFFCCCCCC, 0xFFAAAAAA, 0xFF777777, 0xFF505050, 0xFF282828, 0xFF000000};
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
if(!tex || !texel_out) return;
/* normalize tex_coord to [0,1) */
float frac= tex_coord - floorf(tex_coord);
if(frac < 0.0f) frac+= 1.0f;
/* If an image is present, sample it (bilinear) and return that color. */
if(tex->pixels && tex->img_w > 0 && tex->img_h > 0) {
uint32_t col= texture_sample(tex, frac, 1.0f - frac, true);
texel_out->color= col;
return;
}
/* simple stripe pattern: alternate shade based on fractional coordinate */
const int pattern_resolution= 8;
int pattern_pos= (int)(frac * (float)pattern_resolution);
uint8_t base_shade= texture_shade_from_distance(distance);
/* add a small pattern-dependent offset to the shade to make textures
     * visible */
uint8_t shade= base_shade + (uint8_t)(pattern_pos % 3);
if(shade >= TEXTURE_MAX_SHADES) shade= TEXTURE_MAX_SHADES - 1;
/* prefer side color (vertical/horizontal), fall back to base shade colors
     */
if(is_vertical && tex->side_colors[0][shade])
texel_out->color= tex->side_colors[0][shade];
else if(!is_vertical && tex->side_colors[1][shade])
texel_out->color= tex->side_colors[1][shade];
else
texel_out->color= tex->shade_colors[shade];
}
void texture_set_shade_chars(Texture3D* tex, const char* chars) {
(void)tex;
(void)chars;
/* Stub: no-op for now; kept for API compatibility. */
}
void texture_set_shade_colors(Texture3D* tex, const uint32_t* colors) {
if(!tex || !colors) return;
memcpy(tex->shade_colors, colors, TEXTURE_MAX_SHADES * sizeof(uint32_t));
}
bool texture_load_from_file(Texture3D* tex, const char* filename) {
if(!tex || !filename) return false;
int w= 0, h= 0, channels= 0;
const char* try_path= NULL;
unsigned char* data= NULL;
/* Try a few candidate paths to be tolerant to different working
     * directories. */
const int max_parent_depth= 6;
char buf[1024];
/* 1) direct filename as given */
data= stbi_load(filename, &w, &h, &channels, 4);
if(data) { try_path= filename; }
/* 2) ./filename */
if(!data) {
if((size_t)snprintf(buf, sizeof(buf), "./%s", filename) < sizeof(buf)) {
data= stbi_load(buf, &w, &h, &channels, 4);
if(data) try_path= buf;
}
}
/* 2b) try relative to the executable directory (helpful when the working
     * dir is different) */
if(!data) {
char exe_path[1024];
exe_path[0]= '\0';
ssize_t len= readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
if(len > 0) {
exe_path[len]= '\0';
char* last= strrchr(exe_path, '/');
if(last) *last= '\0'; /* strip binary name */
if((size_t)snprintf(buf, sizeof(buf), "%s/%s", exe_path, filename) < sizeof(buf)) {
data= stbi_load(buf, &w, &h, &channels, 4);
if(data) try_path= buf;
}
}
}
/* 3) walk up the directory tree trying ../, ../../, etc */
for(int depth= 1; depth <= max_parent_depth && !data; depth++) {
size_t used= 0;
for(int i= 0; i < depth; i++) {
if(used + 3 >= sizeof(buf)) break;
buf[used++]= '.';
buf[used++]= '.';
buf[used++]= '/';
}
buf[used]= '\0';
/* append filename if space allows */
if(used + strlen(filename) + 1 >= sizeof(buf)) break;
strcat(buf, filename);
data= stbi_load(buf, &w, &h, &channels, 4);
if(data) try_path= buf;
}
if(!data) {
/* final: try searching for filename without any directory (basename) */
const char* base= strrchr(filename, '/');
if(base)
base++;
else
base= filename;
data= stbi_load(base, &w, &h, &channels, 4);
if(data) try_path= base;
}
if(!data) {
/* Helpful diagnostic when assets fail to load: check whether candidate
                   filenames exist at the filesystem level (fopen). */
fprintf(stderr,
    "texture_load_from_file: failed to load '%s' (stb_image "
    "returned NULL). Existence check:\n",
    filename);
char check[1024];
FILE* f= fopen(filename, "rb");
fprintf(stderr, "  '%s' -> %s\n", filename, f ? "exists" : "missing");
if(f) fclose(f);
if((size_t)snprintf(check, sizeof(check), "./%s", filename) < sizeof(check)) {
f= fopen(check, "rb");
fprintf(stderr, "  '%s' -> %s\n", check, f ? "exists" : "missing");
if(f) fclose(f);
}
/* check parent directories */
for(int depth= 1; depth <= 6; depth++) {
size_t used= 0;
for(int i= 0; i < depth && used + 3 < sizeof(check); i++) {
check[used++]= '.';
check[used++]= '.';
check[used++]= '/';
}
check[used]= '\0';
if(used + strlen(filename) + 1 >= sizeof(check)) break;
strcat(check, filename);
f= fopen(check, "rb");
fprintf(stderr, "  '%s' -> %s\n", check, f ? "exists" : "missing");
if(f) fclose(f);
}
/* check basename */
const char* base= strrchr(filename, '/');
if(base)
base++;
else
base= filename;
f= fopen(base, "rb");
fprintf(stderr, "  '%s' -> %s\n", base, f ? "exists" : "missing");
if(f) fclose(f);
return false;
}
if(try_path) { fprintf(stderr, "texture_load_from_file: loaded '%s' (resolved '%s')\n", filename, try_path); }
/* free any existing image */
if(tex->pixels) free(tex->pixels);
tex->img_w= w;
tex->img_h= h;
tex->pixels= (uint32_t*)malloc((size_t)w * (size_t)h * sizeof(uint32_t));
if(!tex->pixels) {
stbi_image_free(data);
return false;
}
/* data is in R G B A order per pixel */
/* SDL_PIXELFORMAT_ARGB8888 expects bytes [B, G, R, A] in little-endian
           memory, but a hex value 0xAARRGGBB becomes [BB, GG, RR, AA]. This
       swaps R and B. To fix this, we store as 0xAABBGGRR which becomes [RR, GG,
       BB, AA] in memory. */
for(int y= 0; y < h; y++) {
for(int x= 0; x < w; x++) {
int i= (y * w + x) * 4;
unsigned char r= data[i + 0];
unsigned char g= data[i + 1];
unsigned char b= data[i + 2];
unsigned char a= data[i + 3];
/* Store as ABGR hex to get [RGB A] in memory */
tex->pixels[y * w + x]= ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
}
}
stbi_image_free(data);
return true;
}
void texture_free_image(Texture3D* tex) {
if(!tex) return;
if(tex->pixels) {
free(tex->pixels);
tex->pixels= NULL;
}
tex->img_w= tex->img_h= 0;
}
static uint32_t sample_nearest(const Texture3D* tex, float u, float v) {
if(!tex || !tex->pixels || tex->img_w <= 0 || tex->img_h <= 0) return 0;
/* wrap */
while(u < 0.0f) u+= 1.0f;
while(v < 0.0f) v+= 1.0f;
u= u - floorf(u);
v= v - floorf(v);
int x= (int)(u * (float)tex->img_w);
int y= (int)(v * (float)tex->img_h);
if(x < 0) { x= 0; }
if(x >= tex->img_w) { x= tex->img_w - 1; }
if(y < 0) { y= 0; }
if(y >= tex->img_h) { y= tex->img_h - 1; }
return tex->pixels[y * tex->img_w + x];
}
static uint32_t sample_bilinear(const Texture3D* tex, float u, float v) {
if(!tex || !tex->pixels || tex->img_w <= 0 || tex->img_h <= 0) return 0;
while(u < 0.0f) u+= 1.0f;
while(v < 0.0f) v+= 1.0f;
u= u - floorf(u);
v= v - floorf(v);
float x= u * (float)(tex->img_w - 1);
float y= v * (float)(tex->img_h - 1);
int x0= (int)floorf(x);
int x1= x0 + 1;
if(x1 >= tex->img_w) x1= tex->img_w - 1;
int y0= (int)floorf(y);
int y1= y0 + 1;
if(y1 >= tex->img_h) y1= tex->img_h - 1;
float sx= x - (float)x0;
float sy= y - (float)y0;
uint32_t c00= tex->pixels[y0 * tex->img_w + x0];
uint32_t c10= tex->pixels[y0 * tex->img_w + x1];
uint32_t c01= tex->pixels[y1 * tex->img_w + x0];
uint32_t c11= tex->pixels[y1 * tex->img_w + x1];
/* interpolate channels - stored as ABGR so extract B and R in swapped order
     */
float a00= (float)((c00 >> 24) & 0xFF);
float b00= (float)((c00 >> 16) & 0xFF); /* B is at bits 16-23 */
float g00= (float)((c00 >> 8) & 0xFF);
float r00= (float)(c00 & 0xFF); /* R is at bits 0-7 */
float a10= (float)((c10 >> 24) & 0xFF);
float b10= (float)((c10 >> 16) & 0xFF);
float g10= (float)((c10 >> 8) & 0xFF);
float r10= (float)(c10 & 0xFF);
float a01= (float)((c01 >> 24) & 0xFF);
float b01= (float)((c01 >> 16) & 0xFF);
float g01= (float)((c01 >> 8) & 0xFF);
float r01= (float)(c01 & 0xFF);
float a11= (float)((c11 >> 24) & 0xFF);
float b11= (float)((c11 >> 16) & 0xFF);
float g11= (float)((c11 >> 8) & 0xFF);
float r11= (float)(c11 & 0xFF);
float a0= a00 * (1 - sx) + a10 * sx;
float r0= r00 * (1 - sx) + r10 * sx;
float g0= g00 * (1 - sx) + g10 * sx;
float b0= b00 * (1 - sx) + b10 * sx;
float a1= a01 * (1 - sx) + a11 * sx;
float r1= r01 * (1 - sx) + r11 * sx;
float g1= g01 * (1 - sx) + g11 * sx;
float b1= b01 * (1 - sx) + b11 * sx;
float a= a0 * (1 - sy) + a1 * sy;
float r= r0 * (1 - sy) + r1 * sy;
float g= g0 * (1 - sy) + g1 * sy;
float b= b0 * (1 - sy) + b1 * sy;
uint32_t ia= (uint32_t)(a + 0.5f);
uint32_t ir= (uint32_t)(r + 0.5f);
uint32_t ig= (uint32_t)(g + 0.5f);
uint32_t ib= (uint32_t)(b + 0.5f);
/* Reconstruct as ABGR */
return (ia << 24) | (ib << 16) | (ig << 8) | ir;
}
uint32_t texture_sample(const Texture3D* tex, float u, float v, bool bilinear) {
if(!tex || !tex->pixels) return 0;
if(!bilinear) return sample_nearest(tex, u, v);
return sample_bilinear(tex, u, v);
}
