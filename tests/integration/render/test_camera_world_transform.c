#include <assert.h>
#include "snake/render_3d_camera.h"

int main(void) {
    Camera3D* cam = camera_create(90.0f, 64, 0.5f);
    if(!cam) return 2;
    float wx, wy;
    camera_world_to_camera(cam, 0, 0, &wx, &wy);
    camera_destroy(cam);
    return 0;
}
