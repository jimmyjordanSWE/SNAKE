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
    Camera3D* cam = camera_create(90.0f, 64, 1.0f);
    if(!cam) return 2;
    
    for (int from = SNAKE_DIR_UP; from <= SNAKE_DIR_RIGHT; ++from) {
        for (int to = SNAKE_DIR_UP; to <= SNAKE_DIR_RIGHT; ++to) {
            camera_set_prev_angle(cam, dir_to_angle(from));
            camera_set_angle(cam, dir_to_angle(to));
            camera_set_update_interval(cam, 1.0f);
            camera_set_interpolation_time(cam, 0.5f);
            float ia = camera_get_interpolated_angle(cam);
            
            float prev_angle = dir_to_angle(from);
            float angle = dir_to_angle(to);
            float delta = angle - prev_angle;
            if (delta > (float)M_PI) delta -= 2.0f * (float)M_PI;
            if (delta < -(float)M_PI) delta += 2.0f * (float)M_PI;
            float expected = prev_angle + 0.5f * delta;
            expected = fmodf(expected, 2.0f * (float)M_PI);
            if (expected < 0) expected += 2.0f * (float)M_PI;
            float d = angle_diff_abs(ia, expected);
            assert(d < 1e-3f);
        }
    }
    camera_destroy(cam);
    return 0;
}
