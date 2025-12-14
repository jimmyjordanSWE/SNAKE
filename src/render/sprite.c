#include "snake/render_3d_camera.h"
#include "snake/render_3d_sprite.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
void sprite_init(SpriteRenderer3D* sr, int max_sprites, const void* camera) {
if(!sr) return;
sr->max_sprites= max_sprites;
sr->camera= camera;
sr->sprite_count= 0;
sr->sprites= (SpriteProjection*)malloc(sizeof(SpriteProjection) * (long unsigned int)max_sprites);
}
void sprite_clear(SpriteRenderer3D* sr) {
if(!sr) return;
sr->sprite_count= 0;
}
bool sprite_add(SpriteRenderer3D* sr, float world_x, float world_y, uint8_t entity_type, uint8_t entity_id) {
if(!sr || sr->sprite_count >= sr->max_sprites || !sr->camera) return false;
const Camera3D* cam= (const Camera3D*)sr->camera;
SpriteProjection* spr= &sr->sprites[sr->sprite_count];
float dx= world_x - cam->x;
float dy= world_y - cam->y;
float cos_a= cosf(-cam->angle);
float sin_a= sinf(-cam->angle);
float cam_x= dx * cos_a - dy * sin_a;
float cam_y= dx * sin_a + dy * cos_a;
float distance= sqrtf(cam_x * cam_x + cam_y * cam_y);
if(distance < 0.1f) distance= 0.1f;
if(cam_y <= 0) return false;
float screen_x= cam_x / cam_y;
spr->screen_x= screen_x;
spr->screen_y= 0.0f;
spr->screen_z= distance;
float base_size= 0.1f;
float scale= base_size / distance;
spr->screen_width= scale;
spr->screen_height= scale;
float half_width= spr->screen_width / 2.0f;
spr->is_visible= (screen_x + half_width >= -1.0f && screen_x - half_width <= 1.0f);
spr->world_x= world_x;
spr->world_y= world_y;
spr->entity_type= entity_type;
spr->entity_id= entity_id;
sr->sprite_count++;
return true;
}
void sprite_sort_by_depth(SpriteRenderer3D* sr) {
if(!sr || sr->sprite_count <= 1) return;
for(int i= 0; i < sr->sprite_count - 1; i++) {
for(int j= 0; j < sr->sprite_count - i - 1; j++) {
if(sr->sprites[j].screen_z < sr->sprites[j + 1].screen_z) {
SpriteProjection temp= sr->sprites[j];
sr->sprites[j]= sr->sprites[j + 1];
sr->sprites[j + 1]= temp;
}
}
}
}
const SpriteProjection* sprite_get(const SpriteRenderer3D* sr, int idx) {
if(!sr || idx < 0 || idx >= sr->sprite_count) return NULL;
return &sr->sprites[idx];
}
int sprite_get_count(const SpriteRenderer3D* sr) {
if(!sr) return 0;
return sr->sprite_count;
}
void sprite_shutdown(SpriteRenderer3D* sr) {
if(!sr) return;
if(sr->sprites) {
free(sr->sprites);
sr->sprites= NULL;
}
sr->sprite_count= 0;
}
