#include <assert.h>
#include <math.h>
#include "snake/render_3d_camera.h"
#include "snake/types.h"

int main(void) {
    Camera3D* cam = camera_create(90.0f, 64, 0.5f);
    if(!cam) return 2;
    camera_set_from_player(cam, 0, 0, SNAKE_DIR_RIGHT);
    camera_update_interpolation(cam, 0.5f);
    float a1 = camera_get_interpolated_angle(cam);
    camera_update_interpolation(cam, 0.0f);
    float a2 = camera_get_interpolated_angle(cam);
    
    if(isnan(a1) || isnan(a2)) { camera_destroy(cam); return 1; }
    camera_destroy(cam);
    return 0;
}
