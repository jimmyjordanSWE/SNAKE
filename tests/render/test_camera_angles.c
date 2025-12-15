#include <assert.h>
#include <math.h>
#include "snake/render_3d_camera.h"
#include "snake/types.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    Camera3D* cam = camera_create(90.0f, 64, 0.5f);
    if(!cam) return 2;
    for (int dir = SNAKE_DIR_UP; dir <= SNAKE_DIR_RIGHT; ++dir) {
        camera_set_from_player(cam, 5, 5, dir);
        float a = camera_get_interpolated_angle(cam);
        float expected = dir_to_angle(dir);
        
        float d = a - expected;
        while (d > (float)M_PI) d -= 2.0f * (float)M_PI;
        while (d < -(float)M_PI) d += 2.0f * (float)M_PI;
        if (fabsf(d) > 1e-6f) { camera_destroy(cam); return 1; }
        
        float dx = cosf(a);
        float dy = sinf(a);
        float dir_x, dir_y;
        camera_get_dir(cam, &dir_x, &dir_y);
        if (fabsf(dx - dir_x) > 1e-6f) { camera_destroy(cam); return 2; }
        if (fabsf(dy - dir_y) > 1e-6f) { camera_destroy(cam); return 3; }
    }
    camera_destroy(cam);
    return 0;
}
