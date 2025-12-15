#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_sdl.h"
#include "snake/render_3d_sprite.h"
#include <stdlib.h>
#include <stddef.h>
struct Sprite3D {
float world_x, world_y;
float world_height;
float pivot;
bool face_camera;
int texture_id;
int frame;
uint32_t color;
float perp_distance;
int screen_x;
int screen_w;
int screen_h;
int screen_y_top;
bool visible;
};

/* Internal definition of SpriteRenderer3D (opaque to public headers) */
struct SpriteRenderer3D {
    Sprite3D* sprites;
    int max_sprites;
    int count;
    const Camera3D* camera;
    const Projection3D* proj;
};

#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef TEST_SPRITE
float camera_get_interpolated_angle(const Camera3D* camera) {
(void)camera;
return 0.0f;
}
void camera_get_interpolated_position(const Camera3D* camera, float* x_out, float* y_out) {
(void)camera;
if(x_out) *x_out= 0.0f;
if(y_out) *y_out= 0.0f;
}
void projection_project_wall_perp(const Projection3D* proj, float distance, float ray_angle, float cam_angle, WallProjection* result_out) {
(void)distance;
(void)ray_angle;
(void)cam_angle;
if(result_out) {
result_out->wall_height= 1;
result_out->draw_end= proj ? (projection_get_screen_height(proj) / 2) : 0;
result_out->draw_start= result_out->draw_end - 1;
result_out->texture_scale= 1.0f;
}
}
void render_3d_sdl_draw_column(SDL3DContext* ctx, int x, int y0, int y1, uint32_t col) {
(void)ctx;
(void)x;
(void)y0;
(void)y1;
(void)col;
}
#endif
void sprite_init(SpriteRenderer3D* sr, int max_sprites, const Camera3D* camera, const Projection3D* proj) {
if(!sr) return;
sr->sprites= calloc((size_t)max_sprites, sizeof(Sprite3D));
sr->max_sprites= max_sprites;
sr->count= 0;
sr->camera= camera;
sr->proj= proj;
}
void sprite_clear(SpriteRenderer3D* sr) {
if(!sr) return;
sr->count= 0;
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
s->color= render_3d_sdl_color(0, 255, 0, 255);
s->perp_distance= 0.0f;
s->screen_x= s->screen_w= s->screen_h= s->screen_y_top= 0;
s->visible= false;
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
return true;
}
void sprite_project_all(SpriteRenderer3D* sr) {
if(!sr || !sr->camera || !sr->proj) return;
float cam_x, cam_y;
camera_get_interpolated_position(sr->camera, &cam_x, &cam_y);
float cam_angle= camera_get_interpolated_angle(sr->camera);
float half_fov= projection_get_fov_radians(sr->proj) * 0.5f;
for(int i= 0; i < sr->count; ++i) {
Sprite3D* s= &sr->sprites[i];
float dx= s->world_x - cam_x;
float dy= s->world_y - cam_y;
float dist= sqrtf(dx * dx + dy * dy);
float angle_to_sprite= atan2f(dy, dx);
float delta= angle_to_sprite - cam_angle;
while(delta > 3.14159265358979323846f) delta-= 2.0f * 3.14159265358979323846f;
while(delta < -3.14159265358979323846f) delta+= 2.0f * 3.14159265358979323846f;
if(cosf(delta) <= 0.0f || fabsf(delta) > half_fov) {
s->visible= false;
continue;
}
float perp= dist * cosf(delta);
if(perp <= 0.0f) {
s->visible= false;
continue;
}
WallProjection wp;
projection_project_wall_perp(sr->proj, perp, angle_to_sprite, cam_angle, &wp);
int screen_h= (int)((float)wp.wall_height * s->world_height + 0.5f);
if(screen_h <= 0) {
s->visible= false;
continue;
}
int screen_w= screen_h;
float nx= delta / half_fov;
if(nx < -1.0f) nx= -1.0f;
if(nx > 1.0f) nx= 1.0f;
int center_x= (int)((nx + 1.0f) * 0.5f * (float)projection_get_screen_width(sr->proj) + 0.5f);
int top= wp.draw_end - (int)((float)screen_h * (1.0f - s->pivot));
s->perp_distance= perp;
s->screen_x= center_x;
s->screen_w= screen_w;
s->screen_h= screen_h;
s->screen_y_top= top;
s->visible= true;
}
}
void sprite_sort_by_depth(SpriteRenderer3D* sr) {
if(!sr) return;
for(int i= 1; i < sr->count; ++i) {
Sprite3D key= sr->sprites[i];
int j= i - 1;
while(j >= 0 && sr->sprites[j].perp_distance < key.perp_distance) {
sr->sprites[j + 1]= sr->sprites[j];
--j;
}
sr->sprites[j + 1]= key;
}
}
void sprite_draw(SpriteRenderer3D* sr, SDL3DContext* ctx, const float* column_depths) {
if(!sr || !ctx || !column_depths) return;
for(int i= 0; i < sr->count; ++i) {
Sprite3D* s= &sr->sprites[i];
if(!s->visible) continue;
int half_w= s->screen_w / 2;
int x1= s->screen_x - half_w;
int x2= s->screen_x + half_w;
if(x1 < 0) x1= 0;
if(x2 >= render_3d_sdl_get_width(ctx)) x2= render_3d_sdl_get_width(ctx) - 1;
int y0= s->screen_y_top;
int y1= y0 + s->screen_h - 1;
if(y0 < 0) y0= 0;
if(y1 >= render_3d_sdl_get_height(ctx)) y1= render_3d_sdl_get_height(ctx) - 1;
uint32_t col= s->color ? s->color : render_3d_sdl_color(0, 255, 0, 255);
if(s->texture_id == -1) {
int center_x= s->screen_x;
int center_y= s->screen_y_top + s->screen_h / 2;
int radius= s->screen_w / 2;
if(radius <= 0) radius= 1;
int bx0= center_x - radius;
if(bx0 < 0) bx0= 0;
int bx1= center_x + radius;
if(bx1 >= render_3d_sdl_get_width(ctx)) bx1= render_3d_sdl_get_width(ctx) - 1;
int by0= center_y - radius;
if(by0 < 0) by0= 0;
int by1= center_y + radius;
if(by1 >= render_3d_sdl_get_height(ctx)) by1= render_3d_sdl_get_height(ctx) - 1;
for(int yy= by0; yy <= by1; ++yy) {
for(int xx= bx0; xx <= bx1; ++xx) {
int dx= xx - center_x;
int dy= yy - center_y;
if(dx * dx + dy * dy <= radius * radius) {
if(s->perp_distance < column_depths[xx]) { render_3d_sdl_set_pixel(ctx, xx, yy, col); }
}
}
}
} else {
for(int x= x1; x <= x2; ++x) {
if(s->perp_distance < column_depths[x]) { render_3d_sdl_draw_column(ctx, x, y0, y1, col); }
}
}
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

/* public lifecycle functions for opaque SpriteRenderer3D */
SpriteRenderer3D* sprite_create(int max_sprites, const Camera3D* camera, const Projection3D* proj) {
    SpriteRenderer3D* sr = (SpriteRenderer3D*)calloc(1, sizeof(*sr));
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
