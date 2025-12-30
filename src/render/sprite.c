#include "render_3d_camera.h"
#include "render_3d_projection.h"
#include "render_3d_sdl.h"
#include "render_3d_sprite.h"
#include "render_3d_sprite_internal.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/* Small helper: clamp float to 0..255 and return uint8_t */
static inline uint8_t clamp_u8(float v) {
if(v <= 0.0f) return 0;
if(v >= 255.0f) return 255;
return (uint8_t)(v + 0.5f);
}
struct SpriteRenderer3D {
Sprite3D* sprites;
int max_sprites;
int count;
const Camera3D* camera;
const Projection3D* proj;
bool overlap_dirty;
};
#include "math_fast.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
void sprite_init(SpriteRenderer3D* sr, int max_sprites, const Camera3D* camera, const Projection3D* proj) {
if(!sr) return;
sr->sprites= calloc((size_t)max_sprites, sizeof(Sprite3D));
if(!sr->sprites) {
sr->max_sprites= 0;
sr->count= 0;
sr->camera= camera;
sr->proj= proj;
sr->overlap_dirty= true;
return;
}
sr->max_sprites= max_sprites;
sr->count= 0;
sr->camera= camera;
sr->proj= proj;
sr->overlap_dirty= true;
}
void sprite_clear(SpriteRenderer3D* sr) {
if(!sr) return;
sr->count= 0;
sr->overlap_dirty= true;
}
bool sprite_add(SpriteRenderer3D* sr, float world_x, float world_y, float world_height, float pivot, bool face_camera, int texture_id, int frame) {
if(!sr || sr->count >= sr->max_sprites) return false;
Sprite3D* s= &sr->sprites[sr->count++];
s->world_x= world_x;
s->world_y= world_y;
s->world_height= world_height;
s->pivot= pivot;
s->face_camera= face_camera;
s->texture_id= texture_id;
s->frame= frame;
s->color= render_3d_sdl_color(0, 128, 0, 255);
s->perp_distance= 0.0f;
s->screen_x= s->screen_w= s->screen_h= s->screen_y_top= 0;
s->visible= false;
sr->overlap_dirty= true;
return true;
}
bool sprite_add_color(SpriteRenderer3D* sr, float world_x, float world_y, float world_height, float pivot, bool face_camera, int texture_id, int frame, uint32_t color) {
if(!sr || sr->count >= sr->max_sprites) return false;
Sprite3D* s= &sr->sprites[sr->count++];
s->world_x= world_x;
s->world_y= world_y;
s->world_height= world_height;
s->pivot= pivot;
s->face_camera= face_camera;
s->texture_id= texture_id;
s->frame= frame;
s->color= color;
s->perp_distance= 0.0f;
s->screen_x= s->screen_w= s->screen_h= s->screen_y_top= 0;
s->visible= false;
s->is_rect= false;
sr->overlap_dirty= true;
return true;
}
bool sprite_add_rect_color(SpriteRenderer3D* sr, float world_x, float world_y, float world_height, float pivot, bool face_camera, int texture_id, int frame, uint32_t color) {
if(!sr || sr->count >= sr->max_sprites) return false;
Sprite3D* s= &sr->sprites[sr->count++];
s->world_x= world_x;
s->world_y= world_y;
s->world_height= world_height;
s->pivot= pivot;
s->face_camera= face_camera;
s->texture_id= texture_id;
s->frame= frame;
s->color= color;
s->perp_distance= 0.0f;
s->screen_x= s->screen_w= s->screen_h= s->screen_y_top= 0;
s->visible= false;
s->is_rect= true;
sr->overlap_dirty= true;
return true;
}
/* Shaded sprite helpers: reuse add_* then mark shaded */
bool sprite_add_color_shaded(SpriteRenderer3D* sr, float world_x, float world_y, float world_height, float pivot, bool face_camera, int texture_id, int frame, uint32_t color) {
if(!sr || sr->count >= sr->max_sprites) return false;
bool ok= sprite_add_color(sr, world_x, world_y, world_height, pivot, face_camera, texture_id, frame, color);
if(!ok) return false;
Sprite3D* s= &sr->sprites[sr->count - 1];
s->shaded= true;
return true;
}
bool sprite_add_rect_color_shaded(SpriteRenderer3D* sr, float world_x, float world_y, float world_height, float pivot, bool face_camera, int texture_id, int frame, uint32_t color) {
if(!sr || sr->count >= sr->max_sprites) return false;
bool ok= sprite_add_rect_color(sr, world_x, world_y, world_height, pivot, face_camera, texture_id, frame, color);
if(!ok) return false;
Sprite3D* s= &sr->sprites[sr->count - 1];
s->shaded= true;
return true;
}
/* Profiling helpers */
static inline uint64_t now_ns(void) {
struct timespec t;
clock_gettime(CLOCK_MONOTONIC, &t);
return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}
/* Pre-computed lighting table for shaded spheres (32x32) */
#define LIGHT_TABLE_SIZE 32
static float lighting_table[LIGHT_TABLE_SIZE][LIGHT_TABLE_SIZE];
static bool lighting_table_initialized= false;
static void init_lighting_table(void) {
if(lighting_table_initialized) return;
const float lnx= -0.40825f, lny= -0.40825f, lnz= 0.81650f;
const float hnx= -0.26726f, hny= -0.26726f, hnz= 0.92582f;
const float ambient= 0.25f;
const float spec_strength= 0.5f;
for(int yi= 0; yi < LIGHT_TABLE_SIZE; yi++) {
for(int xi= 0; xi < LIGHT_TABLE_SIZE; xi++) {
/* Map indices to -1..1 range */
float nx= ((float)xi / (float)(LIGHT_TABLE_SIZE - 1)) * 2.0f - 1.0f;
float ny= ((float)yi / (float)(LIGHT_TABLE_SIZE - 1)) * 2.0f - 1.0f;
float n2= nx * nx + ny * ny;
if(n2 > 1.0f) {
lighting_table[yi][xi]= 0.0f; /* Outside sphere */
continue;
}
float nz= (1.0f - n2) * fast_inv_sqrt(1.0f - n2);
float diffuse= nx * lnx + ny * lny + nz * lnz;
if(diffuse < 0.0f) diffuse= 0.0f;
float spec= nx * hnx + ny * hny + nz * hnz;
if(spec < 0.0f) spec= 0.0f;
float sp2= spec * spec, sp4= sp2 * sp2, sp8= sp4 * sp4;
spec= sp8 * sp8 * sp8 * spec_strength;
float intensity= ambient + (1.0f - ambient) * diffuse + spec;
if(intensity > 1.0f) intensity= 1.0f;
lighting_table[yi][xi]= intensity;
}
}
lighting_table_initialized= true;
}
/* Accumulators (nanoseconds) */
static uint64_t sprite_time_project_ns= 0;
static uint64_t sprite_time_sort_ns= 0;
static uint64_t sprite_time_draw_ns= 0;
void sprite_project_all(SpriteRenderer3D* sr) {
if(!sr || !sr->camera || !sr->proj) return;
uint64_t start= 0;
int do_profile= getenv("SNAKE_SPRITE_PROFILE") != NULL;
if(do_profile) start= now_ns();
float cam_x, cam_y;
camera_get_interpolated_position(sr->camera, &cam_x, &cam_y);
float cam_angle= camera_get_interpolated_angle(sr->camera);
float half_fov= projection_get_fov_radians(sr->proj) * 0.5f;
/* Precompute trig once per frame */
float cos_cam= cosf(cam_angle);
float sin_cam= sinf(cam_angle);
float sin_half= sinf(half_fov);
float sin_half_sq= sin_half * sin_half;
for(int i= 0; i < sr->count; ++i) {
Sprite3D* s= &sr->sprites[i];
float dx= s->world_x - cam_x;
float dy= s->world_y - cam_y;
float dist2= dx * dx + dy * dy;
if(dist2 <= 1e-8f) {
s->visible= false;
continue;
}
/* Camera-space components: forward (along cam dir) and right (lateral)
                           forward == perp (distance along camera forward vector) */
float forward= dx * cos_cam + dy * sin_cam;
if(forward <= 0.0f) {
s->visible= false;
continue;
}
float right= -dx * sin_cam + dy * cos_cam;
/* Check FOV without atan2 by comparing sin(delta) = right/dist <= sin(half_fov) */
if(right * right > dist2 * sin_half_sq) {
s->visible= false;
continue;
}
float perp= forward;
WallProjection wp;
/* Use direct wall projection with perp (already in camera axis) */
projection_project_wall(sr->proj, perp, &wp);
/* Use full unclamped wall_height for proper sprite scaling, not the clamped draw coordinates */
int full_screen_h= wp.wall_height;
int screen_h= (int)((float)full_screen_h * s->world_height + 0.5f);
if(screen_h <= 0) {
s->visible= false;
continue;
}
int screen_w= screen_h;
if(s->is_rect) { screen_w= (int)((float)screen_h * 1.5f + 0.5f); }
/* Use tangent-based projection matching camera ray calculation
         * Screen position: cameraX = (right/forward) / tan(half_fov)
         * This maintains constant screen position as you approach objects */
float tan_half= tanf(half_fov);
float cameraX= (right / forward) / tan_half;
/* Clamp to screen bounds */
if(cameraX < -1.0f) cameraX= -1.0f;
if(cameraX > 1.0f) cameraX= 1.0f;
int center_x= (int)((cameraX + 1.0f) * 0.5f * (float)projection_get_screen_width(sr->proj) + 0.5f);
/* Calculate vertical position from horizon using unclamped height
         * Sprite center is at horizon, then offset by pivot */
int horizon= projection_get_screen_height(sr->proj) / 2;
int sprite_center_y= horizon;
int top= sprite_center_y - (int)((float)screen_h * s->pivot);
s->perp_distance= perp;
s->screen_x= center_x;
s->screen_w= screen_w;
s->screen_h= screen_h;
s->screen_y_top= top;
s->visible= true;
}
if(do_profile) sprite_time_project_ns+= now_ns() - start;
/* Only run expensive overlap check when sprites have changed */
if(sr->overlap_dirty) {
for(int i= 0; i < sr->count; ++i) {
Sprite3D* a= &sr->sprites[i];
if(a->texture_id == -1) continue;
for(int j= 0; j < sr->count; ++j) {
if(i == j) continue;
Sprite3D* b= &sr->sprites[j];
if(b->texture_id != -1) continue;
if(fabsf(a->world_x - b->world_x) < 1e-3f && fabsf(a->world_y - b->world_y) < 1e-3f) {
a->perp_distance-= 1e-3f;
break;
}
}
}
sr->overlap_dirty= false;
}
}
#define SPRITE_SORT_INSERTION_THRESHOLD 32
/* Direct comparator implementing original tie-breaker and local micro-swap rule.
   Returns -1 if a should come before b, 1 if after, 0 if equal. */
static inline int sprite_cmp_direct(const Sprite3D* a, const Sprite3D* b) {
if(a->perp_distance > b->perp_distance) return -1;
if(a->perp_distance < b->perp_distance) return 1;
bool a_text= a->texture_id != -1;
bool b_text= b->texture_id != -1;
/* If sprites are near-overlapping in world coords, prefer untextured before textured */
if(fabsf(a->world_x - b->world_x) < 0.6f && fabsf(a->world_y - b->world_y) < 0.6f) {
if(a_text && !b_text) return 1;
if(!a_text && b_text) return -1;
}
/* Default tie-breaker: prefer textured before untextured */
if(a_text && !b_text) return -1;
if(!a_text && b_text) return 1;
return 0;
}
static int sprite_cmp(const void* pa, const void* pb) {
const Sprite3D* a= pa;
const Sprite3D* b= pb;
return sprite_cmp_direct(a, b);
}
static void sprite_insertion_sort(Sprite3D* arr, int n) {
for(int i= 1; i < n; ++i) {
Sprite3D key= arr[i];
int j= i - 1;
while(j >= 0 && sprite_cmp_direct(&arr[j], &key) > 0) {
arr[j + 1]= arr[j];
--j;
}
arr[j + 1]= key;
}
}
void sprite_sort_by_depth(SpriteRenderer3D* sr) {
if(!sr) return;
uint64_t start= 0;
int do_profile= getenv("SNAKE_SPRITE_PROFILE") != NULL;
if(do_profile) start= now_ns();
if(sr->count <= SPRITE_SORT_INSERTION_THRESHOLD) {
sprite_insertion_sort(sr->sprites, sr->count);
} else {
qsort(sr->sprites, (size_t)sr->count, sizeof(Sprite3D), sprite_cmp);
}
if(do_profile) sprite_time_sort_ns+= now_ns() - start;
}
void sprite_draw(SpriteRenderer3D* sr, SDL3DContext* ctx, const float* column_depths) {
if(!sr || !ctx || !column_depths) return;
uint64_t start= 0;
int do_profile= getenv("SNAKE_SPRITE_PROFILE") != NULL;
if(do_profile) start= now_ns();
const int scr_w= render_3d_sdl_get_width(ctx);
const int scr_h= render_3d_sdl_get_height(ctx);
for(int i= 0; i < sr->count; ++i) {
Sprite3D* s= &sr->sprites[i];
if(!s->visible) continue;
int half_w= s->screen_w / 2;
int x1= s->screen_x - half_w;
int x2= s->screen_x + half_w;
if(x1 < 0) x1= 0;
if(x2 >= scr_w) x2= scr_w - 1;
int y0= s->screen_y_top;
int y1= y0 + s->screen_h - 1;
if(y0 < 0) y0= 0;
if(y1 >= scr_h) y1= scr_h - 1;
uint32_t col= s->color ? s->color : render_3d_sdl_color(0, 128, 0, 255);
if(s->texture_id == -1) {
/* Check if sprite is fully occluded before rendering */
bool any_visible= false;
for(int xx= x1; xx <= x2; ++xx) {
if(s->perp_distance < column_depths[xx]) {
any_visible= true;
break;
}
}
if(!any_visible) continue; /* Skip fully occluded sprite */
int center_x= s->screen_x;
int center_y= s->screen_y_top + s->screen_h / 2;
int radius= s->screen_w / 2;
if(radius <= 0) radius= 1;
if(s->is_rect) {
int rw= s->screen_w;
int rh= s->screen_h;
int rx0= center_x - rw / 2;
int ry0= center_y - rh / 2;
int rx1= rx0 + rw - 1;
int ry1= ry0 + rh - 1;
if(rx0 < 0) rx0= 0;
if(ry0 < 0) ry0= 0;
if(rx1 >= scr_w) rx1= scr_w - 1;
if(ry1 >= scr_h) ry1= scr_h - 1;
for(int yy= ry0; yy <= ry1; ++yy) {
for(int xx= rx0; xx <= rx1; ++xx) {
if(s->perp_distance < column_depths[xx]) { render_3d_sdl_blend_pixel(ctx, xx, yy, col); }
}
}
} else {
int bx0= center_x - radius;
int bx1= center_x + radius;
int by0= center_y - radius;
int by1= center_y + radius;
if(bx0 < 0) bx0= 0;
if(by0 < 0) by0= 0;
if(bx1 >= scr_w) bx1= scr_w - 1;
if(by1 >= scr_h) by1= scr_h - 1;
const int r2= radius * radius;
for(int yy= by0; yy <= by1; ++yy) {
int dy= yy - center_y;
int dy2= dy * dy;
for(int xx= bx0; xx <= bx1; ++xx) {
int dx= xx - center_x;
if(dx > radius || dx < -radius) continue;
if(dx * dx + dy2 <= r2) {
if(s->perp_distance < column_depths[xx]) {
if(s->shaded) {
/* Use pre-computed lighting table */
if(!lighting_table_initialized) init_lighting_table();
float nx= (float)dx / (float)radius;
float ny= (float)dy / (float)radius;
/* Map to table indices [0, LIGHT_TABLE_SIZE-1] */
int xi= (int)((nx + 1.0f) * 0.5f * (float)(LIGHT_TABLE_SIZE - 1) + 0.5f);
int yi= (int)((ny + 1.0f) * 0.5f * (float)(LIGHT_TABLE_SIZE - 1) + 0.5f);
if(xi < 0) xi= 0;
if(xi >= LIGHT_TABLE_SIZE) xi= LIGHT_TABLE_SIZE - 1;
if(yi < 0) yi= 0;
if(yi >= LIGHT_TABLE_SIZE) yi= LIGHT_TABLE_SIZE - 1;
float intensity= lighting_table[yi][xi];
if(intensity <= 0.0f) continue; /* Outside sphere in table */
uint8_t a= (uint8_t)((col >> 24) & 0xFFu);
uint8_t br= (uint8_t)((col >> 16) & 0xFFu);
uint8_t bg= (uint8_t)((col >> 8) & 0xFFu);
uint8_t bb= (uint8_t)(col & 0xFFu);
uint8_t rr= clamp_u8((float)br * intensity);
uint8_t rg= clamp_u8((float)bg * intensity);
uint8_t rb= clamp_u8((float)bb * intensity);
uint32_t shaded_col= ((uint32_t)a << 24) | ((uint32_t)rr << 16) | ((uint32_t)rg << 8) | (uint32_t)rb;
render_3d_sdl_blend_pixel(ctx, xx, yy, shaded_col);
} else {
render_3d_sdl_blend_pixel(ctx, xx, yy, col);
}
}
}
}
}
}
} else {
for(int x= x1; x <= x2; ++x) {
if(s->perp_distance < column_depths[x]) { render_3d_sdl_draw_column(ctx, x, y0, y1, col); }
}
}
}
if(do_profile) {
sprite_time_draw_ns+= now_ns() - start;
/* Append raw profile to a log file and also print to stderr for quick inspection */
FILE* f= fopen("logs/sprite_profile.log", "a");
if(f) {
fprintf(f, "sprite_profile: project_ns=%llu sort_ns=%llu draw_ns=%llu count=%d\n", (unsigned long long)sprite_time_project_ns, (unsigned long long)sprite_time_sort_ns, (unsigned long long)sprite_time_draw_ns, sr->count);
fclose(f);
}
fprintf(stderr, "sprite_profile: project_ns=%llu sort_ns=%llu draw_ns=%llu count=%d\n", (unsigned long long)sprite_time_project_ns, (unsigned long long)sprite_time_sort_ns, (unsigned long long)sprite_time_draw_ns, sr->count);
/* Reset accumulators for next frame */
sprite_time_project_ns= sprite_time_sort_ns= sprite_time_draw_ns= 0;
}
}
void sprite_shutdown(SpriteRenderer3D* sr) {
if(!sr) return;
free(sr->sprites);
sr->sprites= NULL;
sr->max_sprites= 0;
sr->count= 0;
sr->camera= NULL;
sr->proj= NULL;
}
SpriteRenderer3D* sprite_create(int max_sprites, const Camera3D* camera, const Projection3D* proj) {
SpriteRenderer3D* sr= calloc(1, sizeof *sr);
if(!sr) return NULL;
sprite_init(sr, max_sprites, camera, proj);
return sr;
}
void sprite_destroy(SpriteRenderer3D* sr) {
if(!sr) return;
sprite_shutdown(sr);
free(sr);
}
int sprite_get_count(const SpriteRenderer3D* sr) { return sr ? sr->count : 0; }
bool sprite_get_screen_info(const SpriteRenderer3D* sr, int idx, int* screen_x_out, int* screen_h_out, bool* visible_out) {
if(!sr || idx < 0 || idx >= sr->count) return false;
const Sprite3D* s= &sr->sprites[idx];
if(visible_out) *visible_out= s->visible;
if(screen_x_out) *screen_x_out= s->screen_x;
if(screen_h_out) *screen_h_out= s->screen_h;
return true;
}
int sprite_get_texture_id(const SpriteRenderer3D* sr, int idx, int* texture_id_out) {
if(!sr || idx < 0 || idx >= sr->count) return 0;
if(texture_id_out) *texture_id_out= sr->sprites[idx].texture_id;
return 1;
}
