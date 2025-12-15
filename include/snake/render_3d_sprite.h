/* Sprite rendering API for 3D billboards */
#ifndef SNAKE_RENDER_3D_SPRITE_H
#define SNAKE_RENDER_3D_SPRITE_H

#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_sdl.h"
#include "snake/types.h"
#include <stdbool.h>

typedef struct Sprite3D
{
    float    world_x, world_y;
    float    world_height;
    float    pivot; /* 0.0 bottom .. 1.0 top */
    bool     face_camera;
    int      texture_id; /* -1 => solid color */
    int      frame;
    uint32_t color;
    /* derived */
    float perp_distance;
    int   screen_x;
    int   screen_w;
    int   screen_h;
    int   screen_y_top;
    bool  visible;
} Sprite3D;

typedef struct SpriteRenderer3D
{
    Sprite3D*           sprites;
    int                 max_sprites;
    int                 count;
    const Camera3D*     camera;
    const Projection3D* proj;
} SpriteRenderer3D;

void sprite_init(SpriteRenderer3D*   sr,
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

#endif /* SNAKE_RENDER_3D_SPRITE_H */