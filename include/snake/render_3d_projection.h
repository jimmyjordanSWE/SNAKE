#pragma once
#include <stdint.h>
typedef struct
{
    int   draw_start;
    int   draw_end;
    int   wall_height;
    float texture_scale;
} WallProjection;
typedef struct
{
    int   screen_width;
    int   screen_height;
    float fov_radians;
    /* vertical horizon line (pixels from top) used for camera elevation */
    int horizon_y;
    /* scale applied to computed wall heights */
    float wall_scale;
} Projection3D;
void  projection_init(Projection3D* proj,
                      int           screen_width,
                      int           screen_height,
                      float         fov_radians,
                      float         wall_scale);
void  projection_project_wall(const Projection3D* proj,
                              float               distance,
                              WallProjection*     result_out);
float projection_world_distance_per_pixel(const Projection3D* proj,
                                          float               distance);
void  projection_project_wall_perp(const Projection3D* proj,
                                   float               distance,
                                   float               ray_angle,
                                   float               cam_angle,
                                   WallProjection*     result_out);
float projection_world_distance_per_pixel_perp(const Projection3D* proj,
                                               float               distance,
                                               float               ray_angle,
                                               float               cam_angle);
void  projection_set_horizon(Projection3D* proj, int horizon_y);