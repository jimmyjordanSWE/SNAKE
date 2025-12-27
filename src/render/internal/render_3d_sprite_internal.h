
#pragma once
#include "render_3d_sprite.h"
#include <stdint.h>
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
bool is_rect;
};
