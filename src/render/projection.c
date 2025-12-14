#include "snake/render_3d_projection.h"
#include <math.h>
void projection_init(Projection3D* proj, int screen_width, int screen_height, float fov_radians) {
if(!proj) return;
proj->screen_width= screen_width;
proj->screen_height= screen_height;
proj->fov_radians= fov_radians;
}
void projection_project_wall(const Projection3D* proj, float distance, WallProjection* result_out) {
if(!proj || !result_out) return;
if(distance <= 0.1f) distance= 0.1f;
float wall_height= (float)proj->screen_height / (distance + 0.5f);
if(wall_height > (float)proj->screen_height) wall_height= (float)proj->screen_height;
result_out->wall_height= (int)wall_height;
int center= proj->screen_height / 2;
result_out->draw_start= center - (int)(wall_height / 2);
result_out->draw_end= result_out->draw_start + (int)wall_height;
if(result_out->draw_start < 0) result_out->draw_start= 0;
if(result_out->draw_end >= proj->screen_height) result_out->draw_end= proj->screen_height - 1;
result_out->texture_scale= 1.0f;
}
float projection_world_distance_per_pixel(const Projection3D* proj, float distance) {
if(!proj || distance <= 0.1f) return 1.0f;
return distance / ((float)proj->screen_height * 0.5f);
}
void projection_set_horizon(Projection3D* proj, int horizon_y) {
if(!proj) return;
(void)horizon_y;
}
