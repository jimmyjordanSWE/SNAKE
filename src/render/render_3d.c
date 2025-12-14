#include "snake/render_3d.h"
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_raycast.h"
#include "snake/render_3d_sdl.h"
#include "snake/render_3d_sprite.h"
#include <math.h>
#include "snake/render_3d_texture.h"
#include "snake/types.h"
#include <stdio.h>
#include <stdlib.h>
typedef enum { RENDER_MODE_2D= 0, RENDER_MODE_3D, RENDER_MODE_COUNT } RenderMode;
typedef struct {
const GameState* game_state;
Camera3D camera;
Raycaster3D raycaster;
Projection3D projector;
Texture3D texture;
SpriteRenderer3D sprite_renderer;
SDL3DContext display;
Render3DConfig config;
bool initialized;
	float* column_depths;
} Render3DContext;
static Render3DContext g_render_3d= {0};
bool render_3d_init(const GameState* game_state, const Render3DConfig* config) {
if(g_render_3d.initialized) return true;
if(!game_state) return false;
g_render_3d.game_state= game_state;
if(config) {
g_render_3d.config= *config;
} else {
g_render_3d.config.active_player= 0;
g_render_3d.config.fov_degrees= 90.0f;
g_render_3d.config.show_minimap= false;
g_render_3d.config.show_stats= false;
g_render_3d.config.screen_width= 800;
g_render_3d.config.screen_height= 600;
}
if(!render_3d_sdl_init(g_render_3d.config.screen_width, g_render_3d.config.screen_height, &g_render_3d.display)) return false;
camera_init(&g_render_3d.camera, g_render_3d.config.fov_degrees, g_render_3d.config.screen_width, 0.5f);
raycast_init(&g_render_3d.raycaster, game_state->width, game_state->height, NULL);
projection_init(&g_render_3d.projector, g_render_3d.config.screen_width, g_render_3d.config.screen_height, g_render_3d.config.fov_degrees * 3.14159265359f / 180.0f);
texture_init(&g_render_3d.texture);
/* allocate per-column perpendicular depth buffer for sprite occlusion */
g_render_3d.column_depths = calloc((size_t)g_render_3d.display.width, sizeof(float));
sprite_init(&g_render_3d.sprite_renderer, 100, &g_render_3d.camera, &g_render_3d.projector);
g_render_3d.initialized= true;
return true;
}
void render_3d_draw(const GameState* game_state, const char* player_name, const void* scores, int score_count) {
(void)player_name;
(void)scores;
(void)score_count;
if(!g_render_3d.initialized || !game_state) return;
if(g_render_3d.config.active_player < game_state->num_players) {
	const PlayerState* player= &game_state->players[g_render_3d.config.active_player];
	if(player->length > 0) camera_set_from_player(&g_render_3d.camera, player->body[0].x, player->body[0].y, player->current_dir);

}
camera_update_interpolation(&g_render_3d.camera, 1.0f / 60.0f);
	uint32_t sky_color= 0xFF87CEEB;
	render_3d_sdl_clear(&g_render_3d.display, sky_color);
	uint32_t floor_color= 0xFF8B4513;
	uint32_t ceiling_color= 0xFF4169E1;
	int horizon= g_render_3d.display.height / 2;
	/* use interpolated camera position & angle for rendering */
	float interp_cam_x, interp_cam_y;
	camera_get_interpolated_position(&g_render_3d.camera, &interp_cam_x, &interp_cam_y);
	float interp_cam_angle = camera_get_interpolated_angle(&g_render_3d.camera);
	for(int x= 0; x < g_render_3d.display.width; x++) {
		float ray_angle;
		camera_get_ray_angle(&g_render_3d.camera, x, &ray_angle);
		  RayHit hit;
		  float cos_a = cosf(ray_angle);
		  float sin_a = sinf(ray_angle);
		  /* nudge origin slightly forward and perpendicular to ray to avoid
		     exact grid-line intersection artifacts (vertical seam when aligned)
		  */
		  float eps_fwd = 0.0002f;
		  float eps_perp = 0.0002f;
		  float origin_x = interp_cam_x + cos_a * eps_fwd + sin_a * eps_perp;
		  float origin_y = interp_cam_y + sin_a * eps_fwd - cos_a * eps_perp;
		  if(raycast_cast_ray(&g_render_3d.raycaster, origin_x, origin_y, ray_angle, &hit)) {
			WallProjection proj;
			/* Diagnostic: if a very short hit occurs, log for debugging */
			if(hit.distance < 0.5f) {
				fprintf(stderr, "[ray-debug] col=%d dist=%.4f hit=(%.3f,%.3f) vert=%d\n", x, hit.distance, hit.hit_x, hit.hit_y, hit.is_vertical);
			}
			/* perpendicular distance to avoid fisheye */
			projection_project_wall_perp(&g_render_3d.projector, hit.distance, ray_angle, interp_cam_angle, &proj);
render_3d_sdl_draw_column(&g_render_3d.display, x, 0, proj.draw_start - 1, ceiling_color);
Texel texel;
			float pd = hit.distance * cosf(ray_angle - interp_cam_angle);
if(pd <= 0.1f) pd = 0.1f;
/* record perpendicular depth for this screen column */
if (g_render_3d.column_depths) g_render_3d.column_depths[x] = pd;
texture_get_texel(&g_render_3d.texture, pd, hit.is_vertical, 0.0f, &texel);
render_3d_sdl_draw_column(&g_render_3d.display, x, proj.draw_start, proj.draw_end, texel.color);
render_3d_sdl_draw_column(&g_render_3d.display, x, proj.draw_end + 1, g_render_3d.display.height - 1, floor_color);
} else {
/* no hit -> mark as infinite distance so sprites are never occluded */
if (g_render_3d.column_depths) g_render_3d.column_depths[x] = INFINITY;
render_3d_sdl_draw_column(&g_render_3d.display, x, 0, horizon - 1, ceiling_color);
render_3d_sdl_draw_column(&g_render_3d.display, x, horizon, g_render_3d.display.height - 1, floor_color);
}
}
/* collect sprites for this frame */
sprite_clear(&g_render_3d.sprite_renderer);
for(int i= 0; i < game_state->food_count; i++) {
    /* place food slightly above floor, small sprite */
    sprite_add(&g_render_3d.sprite_renderer, (float)game_state->food[i].x + 0.5f, (float)game_state->food[i].y + 0.5f, 0.25f, 0.0f, true, (int)i, 0);
}
for(int p= 0; p < game_state->num_players; p++) {
    if(p == g_render_3d.config.active_player) continue;
    const PlayerState* player= &game_state->players[p];
    if(!player->active || player->length == 0) continue;
    sprite_add(&g_render_3d.sprite_renderer, (float)player->body[0].x + 0.5f, (float)player->body[0].y + 0.5f, 1.0f, 0.0f, true, (int)p, 0);
}
/* project, sort and draw using per-column occlusion */
sprite_project_all(&g_render_3d.sprite_renderer);
sprite_sort_by_depth(&g_render_3d.sprite_renderer);
sprite_draw(&g_render_3d.sprite_renderer, &g_render_3d.display, g_render_3d.column_depths);
if(!render_3d_sdl_present(&g_render_3d.display)) {}
}
void render_3d_set_active_player(int player_index) {
if(g_render_3d.initialized) g_render_3d.config.active_player= player_index;
}
void render_3d_set_fov(float fov_degrees) {
if(g_render_3d.initialized) g_render_3d.config.fov_degrees= fov_degrees;
}
void render_3d_toggle_minimap(void) {
if(g_render_3d.initialized) g_render_3d.config.show_minimap= !g_render_3d.config.show_minimap;
}
void render_3d_toggle_stats(void) {
if(g_render_3d.initialized) g_render_3d.config.show_stats= !g_render_3d.config.show_stats;
}

void render_3d_shutdown(void) {
if(!g_render_3d.initialized) return;
render_3d_sdl_shutdown(&g_render_3d.display);
if (g_render_3d.column_depths) {
    free(g_render_3d.column_depths);
    g_render_3d.column_depths = NULL;
}
/* clean up sprite renderer storage */
sprite_shutdown(&g_render_3d.sprite_renderer);
g_render_3d.initialized= false;
g_render_3d.game_state= NULL;
}
