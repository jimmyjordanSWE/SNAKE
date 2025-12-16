#include <assert.h>
#include "snake/render_3d_camera.h"

int main(void) {
    Camera3D* cam = camera_create(90.0f, 64, 0.5f);
    if(!cam) return 2;
    camera_tick(cam, 1.0f);
    camera_destroy(cam);
    return 0;
}
