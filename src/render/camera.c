#include "snake/render_3d_camera.h"
#include "snake/types.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static float normalize_angle(float angle) {
float result= fmodf(angle, 2.0f * (float)M_PI);
if(result < 0.0f) result+= 2.0f * (float)M_PI;
return result;
}
static float angle_difference(float from, float to) {
float diff= to - from;
while(diff > (float)M_PI) diff-= 2.0f * (float)M_PI;
while(diff < -(float)M_PI) diff+= 2.0f * (float)M_PI;
return diff;
}
static float interpolate_angle(float from, float to, float t) {
float diff= angle_difference(from, to);
return normalize_angle(from + diff * t);
}
static void update_vectors(Camera3D* camera) {
if(!camera) return;
camera->dir_x= cosf(camera->angle);
camera->dir_y= sinf(camera->angle);
float half_fov= camera->fov_radians * 0.5f;
float plane_len= tanf(half_fov);
camera->plane_x= -sinf(camera->angle) * plane_len;
camera->plane_y= cosf(camera->angle) * plane_len;
}
void camera_init(Camera3D* camera, float fov_degrees, int screen_width, float update_interval) {
if(!camera) return;
memset(camera, 0, sizeof(Camera3D));
camera->x= 0.5f;
camera->y= 0.5f;
camera->prev_x= 0.5f;
camera->prev_y= 0.5f;
camera->angle= 0.0f;
camera->prev_angle= 0.0f;
camera->fov_radians= fov_degrees * (float)M_PI / 180.0f;
camera->screen_width= screen_width;
camera->update_interval= update_interval > 0.0f ? update_interval : 0.5f;
	camera->interp_time= 0.0f;
update_vectors(camera);
}
void camera_set_from_player(Camera3D* camera, int x, int y, int dir) {
if(!camera) return;
	camera->prev_x= camera->x;
	camera->prev_y= camera->y;
	camera->prev_angle= camera->angle;
	camera->x= (float)x + 0.5f;
	camera->y= (float)y + 0.5f;
	/* Map `SnakeDir` enum to world angles. Coordinate system:
	 *  - angle==0 -> +X (right)
	 *  - angle==pi/2 -> +Y (down)
	 *  - angle==pi -> -X (left)
	 *  - angle==3*pi/2 -> -Y (up)
	 * SnakeDir values: UP=0, DOWN=1, LEFT=2, RIGHT=3
	 */
	float angle;
	switch((int)dir) {
	case SNAKE_DIR_UP: angle = (float)M_PI * 1.5f; break;   /* -Y */
	case SNAKE_DIR_DOWN: angle = (float)M_PI * 0.5f; break;  /* +Y */
	case SNAKE_DIR_LEFT: angle = (float)M_PI; break;         /* -X */
	case SNAKE_DIR_RIGHT: angle = 0.0f; break;               /* +X */
	default: angle = 0.0f; break;
	}
	camera->angle= normalize_angle(angle);
	/* start interpolation timer at 0 (will be advanced per-frame) */
	camera->interp_time= 0.0f;
	update_vectors(camera);
}

void camera_update_vectors(Camera3D* camera) {
	update_vectors(camera);
}
void camera_update_interpolation(Camera3D* camera, float delta_time) {
if(!camera || camera->update_interval <= 0.0f) return;
camera->interp_time+= delta_time;
if(camera->interp_time > camera->update_interval) camera->interp_time= camera->update_interval;
}
void camera_set_interpolation_time(Camera3D* camera, float time) {
if(!camera) return;
if(time < 0.0f)
camera->interp_time= 0.0f;
else if(time > camera->update_interval)
camera->interp_time= camera->update_interval;
else
camera->interp_time= time;
}
void camera_get_ray_angle(const Camera3D* camera, int col, float* ray_angle_out) {
if(!camera || !ray_angle_out) return;
float col_normalized= ((float)col - (float)camera->screen_width / 2.0f) / (float)camera->screen_width;
float angle_offset= col_normalized * camera->fov_radians;
float interp_angle= camera_get_interpolated_angle(camera);
*ray_angle_out= interp_angle + angle_offset;
}
void camera_world_to_camera(const Camera3D* camera, float world_x, float world_y, float* cam_x_out, float* cam_y_out) {
if(!camera || !cam_x_out || !cam_y_out) return;
float cam_x, cam_y;
float interp_angle;
camera_get_interpolated_position(camera, &cam_x, &cam_y);
interp_angle = camera_get_interpolated_angle(camera);
float dx = world_x - cam_x;
float dy = world_y - cam_y;
float cos_a = cosf(-interp_angle);
float sin_a = sinf(-interp_angle);
*cam_x_out = dx * cos_a - dy * sin_a;
*cam_y_out = dx * sin_a + dy * cos_a;
}
void camera_camera_to_world(const Camera3D* camera, float cam_x, float cam_y, float* world_x_out, float* world_y_out) {
if(!camera || !world_x_out || !world_y_out) return;
float cam_pos_x, cam_pos_y;
float interp_angle;
camera_get_interpolated_position(camera, &cam_pos_x, &cam_pos_y);
interp_angle = camera_get_interpolated_angle(camera);
float cos_a = cosf(interp_angle);
float sin_a = sinf(interp_angle);
float dx = cam_x * cos_a - cam_y * sin_a;
float dy = cam_x * sin_a + cam_y * cos_a;
*world_x_out = dx + cam_pos_x;
*world_y_out = dy + cam_pos_y;
}
float camera_distance_to_point(const Camera3D* camera, float x, float y) {
if(!camera) return 0.0f;
float cam_x, cam_y;
camera_get_interpolated_position(camera, &cam_x, &cam_y);
float dx= x - cam_x;
float dy= y - cam_y;
return sqrtf(dx * dx + dy * dy);
}
bool camera_point_in_front(const Camera3D* camera, float x, float y) {
if(!camera) return false;
float cam_x, cam_y;
float interp_angle;
camera_get_interpolated_position(camera, &cam_x, &cam_y);
interp_angle = camera_get_interpolated_angle(camera);
float dx = x - cam_x;
float dy = y - cam_y;
float dir_x = cosf(interp_angle);
float dir_y = sinf(interp_angle);
float dot = dx * dir_x + dy * dir_y;
return dot > 0.0f;
}
void camera_get_interpolated_position(const Camera3D* camera, float* x_out, float* y_out) {
if(!camera || !x_out || !y_out) return;
float t= 0.0f;
if(camera->update_interval > 0.0f) {
t= camera->interp_time / camera->update_interval;
if(t < 0.0f) t= 0.0f;
if(t > 1.0f) t= 1.0f;
}
*x_out= camera->prev_x + (camera->x - camera->prev_x) * t;
*y_out= camera->prev_y + (camera->y - camera->prev_y) * t;
}
float camera_get_interpolated_angle(const Camera3D* camera) {
if(!camera) return 0.0f;
float t= 0.0f;
if(camera->update_interval > 0.0f) {
t= camera->interp_time / camera->update_interval;
if(t < 0.0f) t= 0.0f;
if(t > 1.0f) t= 1.0f;
}
return interpolate_angle(camera->prev_angle, camera->angle, t);
}
