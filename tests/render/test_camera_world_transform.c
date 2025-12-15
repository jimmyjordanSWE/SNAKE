#include <assert.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "snake/render_3d_camera.h"
#include "snake/types.h"

static float dir_to_angle(int dir) {
    switch(dir) {
        case SNAKE_DIR_UP: return (float)M_PI * 1.5f;
        case SNAKE_DIR_DOWN: return (float)M_PI * 0.5f;
        case SNAKE_DIR_LEFT: return (float)M_PI;
        case SNAKE_DIR_RIGHT: return 0.0f;
        default: return 0.0f;
    }
}


int main(void) {
    Camera3D* cam = camera_create(90.0f, 64, 1.0f);
    if(!cam) return 2;
    
    camera_set_prev_angle(cam, dir_to_angle(SNAKE_DIR_RIGHT));
    camera_set_angle(cam, dir_to_angle(SNAKE_DIR_LEFT));
    camera_set_update_interval(cam, 1.0f);
    camera_set_interpolation_time(cam, 0.5f);

    float prev_angle = dir_to_angle(SNAKE_DIR_RIGHT);
    float angle = dir_to_angle(SNAKE_DIR_LEFT);
    float diff = angle - prev_angle;
    if (diff > (float)M_PI) diff -= 2.0f * (float)M_PI;
    if (diff < -(float)M_PI) diff += 2.0f * (float)M_PI;
    float expected = prev_angle + 0.5f * diff;

    float cam_x, cam_y;
    camera_get_position(cam, &cam_x, &cam_y);
    float world_x = cam_x + cosf(expected) * 1.0f;
    float world_y = cam_y + sinf(expected) * 1.0f;

    float cx, cy;
    camera_world_to_camera(cam, world_x, world_y, &cx, &cy);
    
    assert(cx > 0.0f);
    assert(fabsf(cy) < 0.01f);
    
    assert(camera_point_in_front(cam, world_x, world_y));
    camera_destroy(cam);
    return 0;
}
