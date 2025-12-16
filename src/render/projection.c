#include "snake/render_3d_projection.h"
#include <math.h>
#include <stdlib.h>
struct Projection3D {
    int screen_width;
    int screen_height;
    float fov_radians;
    int horizon_y;
    float wall_scale;
};
Projection3D* projection_create(int screen_width, int screen_height, float fov_radians, float wall_scale) {
    Projection3D* p = calloc(1, sizeof(*p));
    if (!p)
        return NULL;
    projection_init(p, screen_width, screen_height, fov_radians, wall_scale);
    return p;
}
void projection_destroy(Projection3D* proj) {
    free(proj);
}
void projection_init(Projection3D* proj, int screen_width, int screen_height, float fov_radians, float wall_scale) {
    if (!proj)
        return;
    proj->screen_width = screen_width;
    proj->screen_height = screen_height;
    proj->fov_radians = fov_radians;
    proj->horizon_y = screen_height / 2;
    proj->wall_scale = wall_scale;
}
void projection_project_wall(const Projection3D* proj, float distance, WallProjection* result_out) {
    if (!proj || !result_out)
        return;
    if (distance <= 0.1f)
        distance = 0.1f;
    float wall_height = (float)proj->screen_height / (distance + 0.5f);
    wall_height *= (proj->wall_scale > 0.0f ? proj->wall_scale : 1.5f);
    if (wall_height > (float)proj->screen_height)
        wall_height = (float)proj->screen_height;
    result_out->wall_height = (int)wall_height;
    int center = proj->horizon_y;
    result_out->draw_start = center - (int)(wall_height / 2);
    result_out->draw_end = result_out->draw_start + (int)wall_height;
    if (result_out->draw_start < 0)
        result_out->draw_start = 0;
    if (result_out->draw_end >= proj->screen_height)
        result_out->draw_end = proj->screen_height - 1;
    result_out->texture_scale = 1.0f;
}
float projection_world_distance_per_pixel(const Projection3D* proj, float distance) {
    if (!proj || distance <= 0.1f)
        return 1.0f;
    return distance / ((float)proj->screen_height * 0.5f);
}
void projection_set_horizon(Projection3D* proj, int horizon_y) {
    if (!proj)
        return;
    if (horizon_y < 0)
        horizon_y = 0;
    if (horizon_y >= proj->screen_height)
        horizon_y = proj->screen_height - 1;
    proj->horizon_y = horizon_y;
}
void projection_project_wall_perp(const Projection3D* proj,
                                  float distance,
                                  float ray_angle,
                                  float cam_angle,
                                  WallProjection* out) {
    if (!proj || !out)
        return;
    float pd = distance * cosf(ray_angle - cam_angle);
    if (pd <= 0.1f)
        pd = 0.1f;
    projection_project_wall(proj, pd, out);
}
float projection_world_distance_per_pixel_perp(const Projection3D* proj,
                                               float distance,
                                               float ray_angle,
                                               float cam_angle) {
    if (!proj)
        return 1.0f;
    float pd = distance * cosf(ray_angle - cam_angle);
    if (pd <= 0.1f)
        pd = 0.1f;
    return projection_world_distance_per_pixel(proj, pd);
}
int projection_get_screen_width(const Projection3D* proj) {
    return proj ? proj->screen_width : 0;
}
int projection_get_screen_height(const Projection3D* proj) {
    return proj ? proj->screen_height : 0;
}
float projection_get_fov_radians(const Projection3D* proj) {
    return proj ? proj->fov_radians : 0.0f;
}
float projection_get_wall_scale(const Projection3D* proj) {
    return proj ? proj->wall_scale : 1.0f;
}
