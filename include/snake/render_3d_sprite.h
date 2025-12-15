
#pragma once
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_sdl.h"
#include "snake/types.h"
#include <stdbool.h>
typedef struct Sprite3D Sprite3D;
typedef struct SpriteRenderer3D SpriteRenderer3D;
/* Lifecycle */
SpriteRenderer3D* sprite_create(int max_sprites, const Camera3D* camera, const Projection3D* proj);
void sprite_destroy(SpriteRenderer3D* sr);
void sprite_init(SpriteRenderer3D* sr, int max_sprites, const Camera3D* camera, const Projection3D* proj);
void sprite_clear(SpriteRenderer3D* sr);
bool sprite_add(SpriteRenderer3D* sr, float world_x, float world_y, float world_height, float pivot, bool face_camera, int texture_id, int frame);
bool sprite_add_color(SpriteRenderer3D* sr, float world_x, float world_y, float world_height, float pivot, bool face_camera, int texture_id, int frame, uint32_t color);
void sprite_project_all(SpriteRenderer3D* sr);
void sprite_sort_by_depth(SpriteRenderer3D* sr);
void sprite_draw(SpriteRenderer3D* sr, SDL3DContext* ctx, const float* column_depths);
void sprite_shutdown(SpriteRenderer3D* sr);
bool sprite_get_screen_info(const SpriteRenderer3D* sr, int idx, int* screen_x_out, int* screen_h_out, bool* visible_out);
/* Accessors */
int sprite_get_count(const SpriteRenderer3D* sr);