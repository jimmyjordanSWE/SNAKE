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
    Camera3D cam;
    camera_init(&cam, 90.0f, 64, 0.5f);
    for (int dir = SNAKE_DIR_UP; dir <= SNAKE_DIR_RIGHT; ++dir) {
        camera_set_from_player(&cam, 5, 5, dir);
        float a = camera_get_interpolated_angle(&cam);
        float expected = dir_to_angle(dir);
        /* angles normalized to [0,2pi) */
        float d = a - expected;
        while (d > (float)M_PI) d -= 2.0f * (float)M_PI;
        while (d < -(float)M_PI) d += 2.0f * (float)M_PI;
        if (fabsf(d) > 1e-6f) return 1;
        /* check direction vectors */
        float dx = cosf(a);
        float dy = sinf(a);
        /* compare to camera->dir_x/dir_y via update_vectors()
           those are set in camera_set_from_player */
        if (fabsf(dx - cam.dir_x) > 1e-6f) return 2;
        if (fabsf(dy - cam.dir_y) > 1e-6f) return 3;
    }
    return 0;
}
