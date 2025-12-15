#include "snake/render_3d_projection.h"
#include <math.h>
#include <stdlib.h>

int main(void) {
    Projection3D* p = projection_create(100, 50, 3.14159265359f / 2.0f, 1.2f);
    if(!p) return 1;
    if(projection_get_screen_width(p) != 100) return 2;
    if(projection_get_screen_height(p) != 50) return 3;
    if(fabsf(projection_get_fov_radians(p) - (3.14159265359f / 2.0f)) > 1e-6f) return 4;
    projection_set_horizon(p, 10);
    WallProjection wp;
    projection_project_wall(p, 1.0f, &wp);
    if(wp.wall_height <= 0) { projection_destroy(p); return 5; }
    projection_destroy(p);
    return 0;
}
