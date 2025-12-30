#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "render_3d_camera.h"
#include "render_3d_projection.h"
#include "render_3d_raycast.h"
#include "render_3d_texture.h"

static double timespec_diff_ms(const struct timespec* a, const struct timespec* b) {
    return (double)(b->tv_sec - a->tv_sec) * 1000.0 + (double)(b->tv_nsec - a->tv_nsec) / 1e6;
}

int main(void) {
    const int screen_w = 320;
    const int screen_h = 200;
    const int map_w = 32;
    const int map_h = 32;
    Camera3D* cam = camera_create(75.0f, screen_w, 0.5f);
    Projection3D* proj = projection_create(screen_w, screen_h, camera_get_fov_radians(cam), 1.5f);
    Raycaster3D* rc = raycaster_create(map_w, map_h, NULL);
    Texture3D* wall = texture_create();
    Texture3D* floor = texture_create();

    if (!cam || !proj || !rc || !wall || !floor) {
        fprintf(stderr, "render_bench: init failed\n");
        return 2;
    }

    /* Set camera position to center */
    camera_set_position(cam, (float)map_w / 2.0f, (float)map_h / 2.0f);
    camera_set_angle(cam, 0.0f);

    const int frames = 200;
    volatile uint32_t acc = 0; /* prevent optimizations */
    /* Precompute angle offsets and their trig values */
    float* angle_offsets = malloc(sizeof(float) * (size_t)screen_w);
    float* cos_offsets = malloc(sizeof(float) * (size_t)screen_w);
    float* sin_offsets = malloc(sizeof(float) * (size_t)screen_w);
    if (angle_offsets && cos_offsets && sin_offsets) {
        camera_fill_ray_angle_offsets(cam, angle_offsets);
        for(int i=0; i<screen_w; i++) {
            cos_offsets[i] = cosf(angle_offsets[i]);
            sin_offsets[i] = sinf(angle_offsets[i]);
        }
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int f = 0; f < frames; f++) {
        (void)0; /* placeholder - no camera interpolation in this microbench */
        
        float cam_angle = camera_get_interpolated_angle(cam);
        float cos_cam = cosf(cam_angle);
        float sin_cam = sinf(cam_angle);

        float origin_x = (float)map_w / 2.0f;
        float origin_y = (float)map_h / 2.0f;

        for (int x = 0; x < screen_w; x++) {
            RayHit hit;
            float ray_cos, ray_sin;
            float ray_angle = cam_angle + angle_offsets[x];
            
            if (cos_offsets && sin_offsets) {
                 ray_cos = cos_cam * cos_offsets[x] - sin_cam * sin_offsets[x];
                 ray_sin = sin_cam * cos_offsets[x] + cos_cam * sin_offsets[x];
            } else {
                 ray_cos = cosf(ray_angle);
                 ray_sin = sinf(ray_angle);
            }

            if (raycast_cast_ray_fast(rc, origin_x, origin_y, ray_cos, ray_sin, &hit)) {
                WallProjection p;
                projection_project_wall_perp(proj, hit.distance, ray_angle, ray_angle, &p);
                int wall_h = p.draw_end - p.draw_start + 1;
                if (wall_h <= 0)
                    wall_h = 1;
                float tex_coord = raycast_get_texture_coord(&hit, hit.is_vertical) * 1.0f;
                // Simple checksum loop matching original logic
                for (int yy = p.draw_start; yy <= p.draw_end; yy++) {
                    float v = (float)(yy - p.draw_start) / (float)wall_h;
                    float tex_v = v * 1.0f;
                    uint32_t col;
                    if (texture_has_image(wall))
                        col = texture_sample(wall, tex_coord, tex_v, true);
                    else {
                        Texel tix;
                        texture_get_texel(wall, hit.distance, hit.is_vertical, tex_coord, &tix);
                        col = tix.color;
                    }
                    acc ^= col;
                }
            } else {
                /* miss */
                (void)0;
            }
        }
    }
    if (angle_offsets) free(angle_offsets);
    if (cos_offsets) free(cos_offsets);
    if (sin_offsets) free(sin_offsets);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = timespec_diff_ms(&t0, &t1);
    printf("render_bench: frames=%d screen=%dx%d total_ms=%.3f avg_ms_per_frame=%.6f acc=0x%08x\n", frames, screen_w,
           screen_h, ms, ms / frames, (unsigned int)acc);

    projection_destroy(proj);
    camera_destroy(cam);
    raycaster_destroy(rc);
    texture_destroy(wall);
    texture_destroy(floor);
    return 0;
}
