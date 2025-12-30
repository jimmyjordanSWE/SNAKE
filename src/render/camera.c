#include "math_fast.h"
#include "render_3d_camera.h"
#include "types.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
struct Camera3D {
float x, y;
float angle;
float prev_x, prev_y, prev_angle;
float fov_radians;
int screen_width;
float dir_x, dir_y;
float plane_x, plane_y;
float interp_time;
float update_interval;
/* Cached per-column angle offsets (length = cached_width) */
float* cached_angle_offsets;
int cached_width;
float cached_fov_radians;
};
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
Camera3D* camera_create(float fov_degrees, int screen_width, float update_interval) {
Camera3D* c= calloc(1, sizeof(*c));
if(!c) return NULL;
camera_init(c, fov_degrees, screen_width, update_interval);
return c;
}
void camera_destroy(Camera3D* camera) {
if(!camera) return;
free(camera->cached_angle_offsets);
camera->cached_angle_offsets= NULL;
free(camera);
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
/* Init cached angle offsets state */
camera->cached_angle_offsets= NULL;
camera->cached_width= 0;
camera->cached_fov_radians= -1.0f;
update_vectors(camera);
}
void camera_set_from_player(Camera3D* camera, int x, int y, int dir) {
if(!camera) return;
camera->prev_x= camera->x;
camera->prev_y= camera->y;
camera->prev_angle= camera->angle;
camera->x= (float)x + 0.5f;
camera->y= (float)y + 0.5f;
float angle;
switch((int)dir) {
case SNAKE_DIR_UP: angle= (float)M_PI * 1.5f; break;
case SNAKE_DIR_DOWN: angle= (float)M_PI * 0.5f; break;
case SNAKE_DIR_LEFT: angle= (float)M_PI; break;
case SNAKE_DIR_RIGHT: angle= 0.0f; break;
default: angle= 0.0f; break;
}
camera->angle= normalize_angle(angle);
camera->interp_time= 0.0f;
update_vectors(camera);
}
void camera_update_vectors(Camera3D* camera) { update_vectors(camera); }
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
void camera_set_position(Camera3D* cam, float x, float y) {
if(!cam) return;
cam->x= x;
cam->y= y;
}
void camera_set_angle(Camera3D* cam, float angle) {
if(!cam) return;
cam->angle= normalize_angle(angle);
update_vectors(cam);
}
void camera_set_prev_position(Camera3D* cam, float prev_x, float prev_y) {
if(!cam) return;
cam->prev_x= prev_x;
cam->prev_y= prev_y;
}
void camera_set_prev_angle(Camera3D* cam, float prev_angle) {
if(!cam) return;
cam->prev_angle= normalize_angle(prev_angle);
}
void camera_set_update_interval(Camera3D* cam, float update_interval) {
if(!cam) return;
cam->update_interval= update_interval > 0.0f ? update_interval : 0.0f;
}
float camera_get_fov_radians(const Camera3D* cam) { return cam ? cam->fov_radians : 0.0f; }
void camera_get_dir(const Camera3D* cam, float* dx_out, float* dy_out) {
if(!cam) return;
if(dx_out) *dx_out= cam->dir_x;
if(dy_out) *dy_out= cam->dir_y;
}
float camera_get_update_interval(const Camera3D* cam) { return cam ? cam->update_interval : 0.0f; }
float camera_get_interpolation_fraction(const Camera3D* cam) {
if(!cam) return 0.0f;
float t= 0.0f;
if(cam->update_interval > 0.0f) {
t= cam->interp_time / cam->update_interval;
if(t < 0.0f) t= 0.0f;
if(t > 1.0f) t= 1.0f;
}
return t;
}
float camera_get_interp_time(const Camera3D* cam) { return cam ? cam->interp_time : 0.0f; }
void camera_get_position(const Camera3D* cam, float* x_out, float* y_out) {
if(!cam || !x_out || !y_out) return;
*x_out= cam->x;
*y_out= cam->y;
}
void camera_get_ray_angle(const Camera3D* camera, int col, float* ray_angle_out) {
if(!camera || !ray_angle_out) return;
float interp_angle= camera_get_interpolated_angle(camera);
float half_fov= camera->fov_radians * 0.5f;
float tan_half= tanf(half_fov);
float cameraX= (2.0f * ((float)col + 0.5f) / (float)camera->screen_width) - 1.0f;
float angle_offset= atanf(cameraX * tan_half);
*ray_angle_out= interp_angle + angle_offset;
}
void camera_fill_ray_angle_offsets(const Camera3D* camera, float* out_array) {
if(!camera || !out_array) return;
float half_fov= camera->fov_radians * 0.5f;
float tan_half= tanf(half_fov);
for(int col= 0; col < camera->screen_width; col++) {
float cameraX= (2.0f * ((float)col + 0.5f) / (float)camera->screen_width) - 1.0f;
out_array[col]= atanf(cameraX * tan_half);
}
}
void camera_prepare_angle_offsets(Camera3D* camera, int screen_width) {
if(!camera || screen_width <= 0) return;
/* If width and fov unchanged, keep cached buffer as-is */
if(camera->cached_angle_offsets && camera->cached_width == screen_width && camera->cached_fov_radians == camera->fov_radians) return;
float* new_buf= (float*)realloc(camera->cached_angle_offsets, (size_t)screen_width * sizeof(float));
if(!new_buf) {
/* Allocation failed: leave previous buffer intact and return */
return;
}
camera->cached_angle_offsets= new_buf;
camera->cached_width= screen_width;
camera->cached_fov_radians= camera->fov_radians;
float half_fov= camera->fov_radians * 0.5f;
float tan_half= tanf(half_fov);
for(int col= 0; col < screen_width; col++) {
float cameraX= (2.0f * ((float)col + 0.5f) / (float)screen_width) - 1.0f;
camera->cached_angle_offsets[col]= atanf(cameraX * tan_half);
}
}
const float* camera_get_cached_angle_offsets(const Camera3D* camera) { return camera ? camera->cached_angle_offsets : NULL; }
int camera_get_cached_angle_offsets_width(const Camera3D* camera) { return camera ? camera->cached_width : 0; }
void camera_world_to_camera(const Camera3D* camera, float world_x, float world_y, float* cam_x_out, float* cam_y_out) {
if(!camera || !cam_x_out || !cam_y_out) return;
float cam_x, cam_y;
float interp_angle;
camera_get_interpolated_position(camera, &cam_x, &cam_y);
interp_angle= camera_get_interpolated_angle(camera);
float dx= world_x - cam_x;
float dy= world_y - cam_y;
float cos_a= cosf(-interp_angle);
float sin_a= sinf(-interp_angle);
*cam_x_out= dx * cos_a - dy * sin_a;
*cam_y_out= dx * sin_a + dy * cos_a;
}
void camera_camera_to_world(const Camera3D* camera, float cam_x, float cam_y, float* world_x_out, float* world_y_out) {
if(!camera || !world_x_out || !world_y_out) return;
float cam_pos_x, cam_pos_y;
float interp_angle;
camera_get_interpolated_position(camera, &cam_pos_x, &cam_pos_y);
interp_angle= camera_get_interpolated_angle(camera);
float cos_a= cosf(interp_angle);
float sin_a= sinf(interp_angle);
float dx= cam_x * cos_a - cam_y * sin_a;
float dy= cam_x * sin_a + cam_y * cos_a;
*world_x_out= dx + cam_pos_x;
*world_y_out= dy + cam_pos_y;
}
float camera_distance_to_point(const Camera3D* camera, float x, float y) {
if(!camera) return 0.0f;
float cam_x, cam_y;
camera_get_interpolated_position(camera, &cam_x, &cam_y);
float dx= x - cam_x;
float dy= y - cam_y;
float d2= dx * dx + dy * dy;
if(d2 <= 0.0f) return 0.0f;
/* Use fast inverse sqrt and invert to get distance */
float inv= fast_inv_sqrt(d2);
if(inv == 0.0f) return 0.0f;
return 1.0f / inv;
}
bool camera_point_in_front(const Camera3D* camera, float x, float y) {
if(!camera) return false;
float cam_x, cam_y;
float interp_angle;
camera_get_interpolated_position(camera, &cam_x, &cam_y);
interp_angle= camera_get_interpolated_angle(camera);
float dx= x - cam_x;
float dy= y - cam_y;
float dir_x= cosf(interp_angle);
float dir_y= sinf(interp_angle);
float dot= dx * dir_x + dy * dir_y;
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
if(camera->update_interval <= 0.0f) return camera->angle;
float t= camera->interp_time / camera->update_interval;
if(t < 0.0f) t= 0.0f;
if(t > 1.0f) t= 1.0f;
if(t <= 0.0f) return camera->angle;
return interpolate_angle(camera->prev_angle, camera->angle, t);
}
