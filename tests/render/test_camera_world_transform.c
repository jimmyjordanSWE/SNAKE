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

/* Ensure world->camera transform uses interpolated angle/position */
int main(void) {
    Camera3D cam;
    camera_init(&cam, 90.0f, 64, 1.0f);
    /* simulate a 180-degree turn from RIGHT -> LEFT */
    cam.prev_angle = dir_to_angle(SNAKE_DIR_RIGHT);
    cam.angle = dir_to_angle(SNAKE_DIR_LEFT);
    cam.update_interval = 1.0f;
    cam.interp_time = 0.5f; /* halfway */
    /* expected halfway angle (shortest arc) */
    float diff = cam.angle - cam.prev_angle;
    if (diff > (float)M_PI) diff -= 2.0f * (float)M_PI;
    if (diff < -(float)M_PI) diff += 2.0f * (float)M_PI;
    float expected = cam.prev_angle + 0.5f * diff;

    /* place a point one unit in front along expected angle */
    float world_x = cam.x + cosf(expected) * 1.0f;
    float world_y = cam.y + sinf(expected) * 1.0f;

    float cx, cy;
    camera_world_to_camera(&cam, world_x, world_y, &cx, &cy);
    /* point should be in front (positive cam-x) and near the center (cam-y approx 0) */
    assert(cx > 0.0f);
    assert(fabsf(cy) < 0.01f);
    /* camera_point_in_front should agree */
    assert(camera_point_in_front(&cam, world_x, world_y));
    return 0;
}
