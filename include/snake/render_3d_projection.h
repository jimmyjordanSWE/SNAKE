#pragma once
#include <stdint.h>
typedef struct {
int draw_start;
int draw_end;
int wall_height;
float texture_scale;
} WallProjection;
typedef struct Projection3D Projection3D;
// Returns a newly allocated Projection3D; caller must call projection_destroy()
Projection3D* projection_create(int screen_width, int screen_height, float fov_radians, float wall_scale);
void projection_destroy(Projection3D* proj);
void projection_init(Projection3D* proj, int screen_width, int screen_height, float fov_radians, float wall_scale);
void projection_project_wall(const Projection3D* proj, float distance, WallProjection* result_out);
float projection_world_distance_per_pixel(const Projection3D* proj, float distance);
void projection_project_wall_perp(const Projection3D* proj, float distance, float ray_angle, float cam_angle, WallProjection* result_out);
float projection_world_distance_per_pixel_perp(const Projection3D* proj, float distance, float ray_angle, float cam_angle);
void projection_set_horizon(Projection3D* proj, int horizon_y);
int projection_get_screen_width(const Projection3D* proj);
int projection_get_screen_height(const Projection3D* proj);
float projection_get_fov_radians(const Projection3D* proj);
float projection_get_wall_scale(const Projection3D* proj);