#include <assert.h>
#include <math.h>
#include <stdio.h>

static float compute_u(float interp_cam_x, float interp_cam_y, float ray_angle, int screen_h, int horizon, float pd, int proj_draw_start, int wall_h, float wall_h_world, int yy) {
    const float camera_height = 0.5f;
    float v = (float)(yy - proj_draw_start) / (float)wall_h;
    float z = (1.0f - v) * wall_h_world;
    float p = (float)(yy - horizon) / ((float)screen_h * 0.5f);
    if(fabsf(p) <= 1e-6f) p = (p < 0.0f) ? -1e-6f : 1e-6f;
    float rowDist = (camera_height - z) / p;
    if(!isfinite(rowDist) || rowDist <= 0.0f) rowDist = pd;
    /* compute only world_y since u for vertical face depends on it */
    float world_y = interp_cam_y + sinf(ray_angle) * rowDist;
    float u_frac = world_y - floorf(world_y);
    if(u_frac < 0.0f) u_frac += 1.0f;
    return u_frac;
}

int main(void) {
    /* Simulate parameters for a near-wall situation */
    float interp_cam_x = 1.2f;
    float interp_cam_y = 1.5f;
    float ray_angle = 0.3f; /* some oblique angle */
    int screen_h = 64;
    int horizon = screen_h / 2;
    float pd = 0.2f; /* very close */
    int proj_draw_start = horizon - 10;
    int wall_h = 40; /* large on-screen wall */
    float wall_h_world = 1.5f;
    int yy1 = proj_draw_start + 1;
    int yy2 = proj_draw_start + wall_h - 1;

    float u1 = compute_u(interp_cam_x, interp_cam_y, ray_angle, screen_h, horizon, pd, proj_draw_start, wall_h, wall_h_world, yy1);
    float u2 = compute_u(interp_cam_x, interp_cam_y, ray_angle, screen_h, horizon, pd, proj_draw_start, wall_h, wall_h_world, yy2);
    /* When very close and at oblique angle, u should vary noticeably between top and bottom */
    assert(fabsf(u1 - u2) > 0.0001f);
    printf("u1=%.6f u2=%.6f\n", u1, u2);
    return 0;
}
