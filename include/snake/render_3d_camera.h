#pragma once
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
typedef struct {
float x, y;
float angle;
float prev_x, prev_y, prev_angle;
float fov_radians;
int screen_width;
float dir_x, dir_y;
float plane_x, plane_y;
float interp_time;
float update_interval;
	/* camera mode: 0 = first-person, 1 = third-person */
	int mode;
	/* third-person distance behind the player (world units) */
	float third_person_distance;
	/* vertical elevation as pixel offset for projection horizon (positive moves horizon down)
	   used to simulate camera above the player */
	int elevation_pixels;
} Camera3D;
typedef enum { CAMERA_MODE_FIRST_PERSON = 0, CAMERA_MODE_THIRD_PERSON = 1 } CameraMode;
void camera_init(Camera3D* camera, float fov_degrees, int screen_width, float update_interval);
void camera_set_from_player(Camera3D* camera, int x, int y, int dir);
void camera_set_mode(Camera3D* camera, CameraMode mode);
void camera_set_third_person_params(Camera3D* camera, float distance, int elevation_pixels);
void camera_update_interpolation(Camera3D* camera, float delta_time);
void camera_set_interpolation_time(Camera3D* camera, float time);
void camera_get_ray_angle(const Camera3D* camera, int col, float* ray_angle_out);
void camera_world_to_camera(const Camera3D* camera, float world_x, float world_y, float* cam_x_out, float* cam_y_out);
void camera_camera_to_world(const Camera3D* camera, float cam_x, float cam_y, float* world_x_out, float* world_y_out);
float camera_distance_to_point(const Camera3D* camera, float x, float y);
bool camera_point_in_front(const Camera3D* camera, float x, float y);
void camera_get_interpolated_position(const Camera3D* camera, float* x_out, float* y_out);
float camera_get_interpolated_angle(const Camera3D* camera);
