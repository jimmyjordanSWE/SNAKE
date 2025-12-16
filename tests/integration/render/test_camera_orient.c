#include <assert.h>
#include "snake/render_3d_camera.h"
#include "snake/types.h"

int main(void) {
    Camera3D* cam = camera_create(90.0f, 64, 0.5f);
    if(!cam) return 2;
    camera_set_from_player(cam, 1, 1, SNAKE_DIR_LEFT);
    camera_destroy(cam);
    return 0;
}
