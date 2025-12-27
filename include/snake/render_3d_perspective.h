#pragma once
#include <stdbool.h>
float render_3d_compute_wall_u(float interp_cam_x, float interp_cam_y, float ray_angle, int yy, int horizon, int screen_height, float pd, bool is_vertical, float wall_world_height, float* out_rowDist);
