#include "render_3d_raycast.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#define PI 3.14159265359f
struct Raycaster3D {
    int board_width;
    int board_height;
    const uint8_t* board;
};
Raycaster3D* raycaster_create(int width, int height, const uint8_t* board) {
    Raycaster3D* r = calloc(1, sizeof *r);
    if (!r)
        return NULL;
    raycast_init(r, width, height, board);
    return r;
}
void raycaster_destroy(Raycaster3D* rc) {
    if (!rc)
        return;
    free(rc);
}
void raycast_init(Raycaster3D* rc, int width, int height, const uint8_t* board) {
    if (!rc)
        return;
    rc->board_width = width;
    rc->board_height = height;
    rc->board = board;
}
bool raycast_cast_ray(const Raycaster3D* rc, float camera_x, float camera_y, float ray_angle, RayHit* hit_out) {
    if (!rc || !hit_out)
        return false;
    while (ray_angle < 0)
        ray_angle += 2 * PI;
    while (ray_angle >= 2 * PI)
        ray_angle -= 2 * PI;
    float dist_right = (float)rc->board_width - camera_x;
    float dist_left = camera_x;
    float dist_up = camera_y;
    float dist_down = (float)rc->board_height - camera_y;
    float cos_a = cosf(ray_angle);
    float sin_a = sinf(ray_angle);
    const float eps = 1e-6f;
    float dist_x;
    if (fabsf(cos_a) < eps) {
        dist_x = FLT_MAX;
    } else {
        dist_x = (cos_a > 0) ? dist_right / cos_a : dist_left / -cos_a;
        if (!isfinite(dist_x) || dist_x < 0.0f)
            dist_x = FLT_MAX;
    }
    float dist_y;
    if (fabsf(sin_a) < eps) {
        dist_y = FLT_MAX;
    } else {
        dist_y = (sin_a > 0) ? dist_down / sin_a : dist_up / -sin_a;
        if (!isfinite(dist_y) || dist_y < 0.0f)
            dist_y = FLT_MAX;
    }
    float distance = fminf(dist_x, dist_y);
    if (!isfinite(distance) || distance <= 0.0f) {
        return false;
    }
    hit_out->distance = distance;
    hit_out->hit_x = camera_x + cos_a * distance;
    hit_out->hit_y = camera_y + sin_a * distance;
    hit_out->is_vertical = (dist_x < dist_y);
    hit_out->shade_level = 0;
    return true;
}
bool raycast_is_wall(const Raycaster3D* rc, float x, float y) {
    if (!rc || !rc->board)
        return false;
    int gx = (int)x;
    int gy = (int)y;
    if (gx < 0 || gx >= rc->board_width || gy < 0 || gy >= rc->board_height)
        return true;
    return false;
}
float raycast_get_texture_coord(const RayHit* hit, bool is_vertical) {
    if (!hit)
        return 0.0f;
    float coord = is_vertical ? hit->hit_y : hit->hit_x;
    float frac = coord - floorf(coord);
    if (frac < 0.0f)
        frac += 1.0f;
    return frac;
}
