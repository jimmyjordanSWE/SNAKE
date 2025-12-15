#include <assert.h>
#include "snake/render_3d_camera.h"
#include "snake/types.h"

int main(void) {
    Camera3D* cam = camera_create(90.0f, 64, 0.25f);
    if(!cam) return 2;
    
    camera_set_position(cam, 1.5f, 2.5f);
    camera_set_prev_position(cam, 1.5f, 2.5f);
    camera_set_angle(cam, 0.0f);
    camera_set_prev_angle(cam, 0.0f);
    
    camera_set_from_player(cam, 2, 2, SNAKE_DIR_DOWN);
    assert(camera_get_interp_time(cam) == 0.0f);
    
    camera_update_interpolation(cam, 0.125f);
    float ia = camera_get_interpolated_angle(cam);
    
    assert(ia > 0.0f && ia < 1.7f);
    camera_destroy(cam);
    return 0;
}
