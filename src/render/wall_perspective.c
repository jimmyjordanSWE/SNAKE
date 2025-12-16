#include "snake/render_3d_perspective.h"
#include <math.h>
#include <stdbool.h>
float render_3d_compute_wall_u(float interp_cam_x,
                               float interp_cam_y,
                               float ray_angle,
                               int yy,
                               int horizon,
                               int screen_height,
                               float pd,
                               bool is_vertical,
                               float wall_world_height,
                               float* out_rowDist) {
    const float camera_height = 0.5f;
    float p = (float)(yy - horizon) / ((float)screen_height * 0.5f);
    float rowDist = pd;
    if (fabsf(p) > 1e-6f) {
        float v = 0.5f;
        float z = (1.0f - v) * wall_world_height;
        rowDist = (camera_height - z) / p;
        if (!isfinite(rowDist) || rowDist <= 0.0f)
            rowDist = pd;
    }
    if (out_rowDist)
        *out_rowDist = rowDist;
    float world_x = interp_cam_x + cosf(ray_angle) * rowDist;
    float world_y = interp_cam_y + sinf(ray_angle) * rowDist;
    float u_frac;
    if (is_vertical)
        u_frac = world_y - floorf(world_y);
    else
        u_frac = world_x - floorf(world_x);
    if (u_frac < 0.0f)
        u_frac += 1.0f;
    return u_frac;
}
