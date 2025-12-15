#include <assert.h>
#include "snake/render_3d_camera.h"
#include "snake/types.h"

int main(void) {
    Camera3D cam;
    camera_init(&cam, 90.0f, 64, 0.25f);
    /* simulate initial state */
    cam.x = 1.5f; cam.y = 2.5f; cam.angle = 0.0f; cam.prev_x = cam.x; cam.prev_y = cam.y; cam.prev_angle = cam.angle;
    /* set camera from player to a new position+dir */
    camera_set_from_player(&cam, 2, 2, SNAKE_DIR_DOWN); /* sets angle to pi/2 and interp_time=0 */
    assert(cam.interp_time == 0.0f);
    /* advance half the update interval */
    camera_update_interpolation(&cam, 0.125f);
    float ia = camera_get_interpolated_angle(&cam);
    /* angle should be between 0 and pi/2 */
    assert(ia > 0.0f && ia < 1.7f);
    return 0;
}
