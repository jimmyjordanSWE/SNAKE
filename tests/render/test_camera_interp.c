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

static float angle_diff_abs(float a, float b) {
    float d = a - b;
    while (d > (float)M_PI) d -= 2.0f * (float)M_PI;
    while (d < -(float)M_PI) d += 2.0f * (float)M_PI;
    if (d < 0) d = -d;
    return d;
}

int main(void) {
    Camera3D cam;
    camera_init(&cam, 90.0f, 64, 1.0f);
    /* Test mapping and interpolation for all direction transitions */
    for (int from = SNAKE_DIR_UP; from <= SNAKE_DIR_RIGHT; ++from) {
        for (int to = SNAKE_DIR_UP; to <= SNAKE_DIR_RIGHT; ++to) {
            cam.prev_angle = dir_to_angle(from);
            cam.angle = dir_to_angle(to);
            cam.update_interval = 1.0f;
            cam.interp_time = 0.5f; /* halfway */
            float ia = camera_get_interpolated_angle(&cam);
            /* expected halfway between shortest arc */
            float delta = cam.angle - cam.prev_angle;
            if (delta > (float)M_PI) delta -= 2.0f * (float)M_PI;
            if (delta < -(float)M_PI) delta += 2.0f * (float)M_PI;
            float expected = cam.prev_angle + 0.5f * delta;
            expected = fmodf(expected, 2.0f * (float)M_PI);
            if (expected < 0) expected += 2.0f * (float)M_PI;
            float d = angle_diff_abs(ia, expected);
            assert(d < 1e-3f);
        }
    }
    return 0;
}
