#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    float distance;
    float hit_x;
    float hit_y;
    bool is_vertical;
    uint8_t shade_level;
} RayHit;
typedef struct Raycaster3D Raycaster3D;
Raycaster3D* raycaster_create(int width, int height, const uint8_t* board);
void raycaster_destroy(Raycaster3D* rc);
void raycast_init(Raycaster3D* rc, int width, int height, const uint8_t* board);
bool raycast_cast_ray(const Raycaster3D* rc, float camera_x, float camera_y, float ray_angle, RayHit* hit_out);
bool raycast_is_wall(const Raycaster3D* rc, float x, float y);
float raycast_get_texture_coord(const RayHit* hit, bool is_vertical);
