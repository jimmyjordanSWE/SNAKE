/* Sprite rendering API for 3D billboards */
#pragma once
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_sdl.h"
#include "snake/types.h"
#include <stdbool.h>

/* Forward-declare Sprite3D to keep its derived/internal fields private.
 * Use the sprite_* API to interact with sprites; callers should not rely
 * on the internal layout of Sprite3D. */
typedef struct Sprite3D Sprite3D;

typedef struct SpriteRenderer3D
{
    Sprite3D*           sprites;
    int                 max_sprites;
    int                 count;
    const Camera3D*     camera;
    const Projection3D* proj;
} SpriteRenderer3D;

void sprite_init(SpriteRenderer3D* sr,
                 int                 max_sprites,
                 const Camera3D*     camera,
                 const Projection3D* proj);
void sprite_clear(SpriteRenderer3D* sr);
bool sprite_add(SpriteRenderer3D* sr,
                float             world_x,
                float             world_y,
                float             world_height,
                float             pivot,
                bool              face_camera,
                int               texture_id,
                int               frame);
bool sprite_add_color(SpriteRenderer3D* sr,
                      float             world_x,
                      float             world_y,
                      float             world_height,
                      float             pivot,
                      bool              face_camera,
                      int               texture_id,
                      int               frame,
                      uint32_t          color);
void sprite_project_all(SpriteRenderer3D* sr);
void sprite_sort_by_depth(SpriteRenderer3D* sr);
void sprite_draw(SpriteRenderer3D* sr,
                 SDL3DContext*     ctx,
                 const float*      column_depths);
void sprite_shutdown(SpriteRenderer3D* sr);