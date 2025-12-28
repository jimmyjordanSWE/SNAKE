#include "render_3d_sprite.h"
#include "render_3d_sdl.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv) {
    const int frames = 200;
    const int width = 320;
    const int height = 200;
    SDL3DContext* ctx = render_3d_sdl_create(width, height);
    if (!ctx) {
        fprintf(stderr, "failed to create ctx\n");
        return 1;
    }
    Camera3D* cam = camera_create(75.0f, width, 0.5f);
    if (!cam) {
        fprintf(stderr, "failed to create camera\n");
        render_3d_sdl_destroy(ctx);
        return 1;
    }
    Projection3D* proj = projection_create(width, height, camera_get_fov_radians(cam), 1.5f);
    if (!proj) {
        fprintf(stderr, "failed to create projection\n");
        camera_destroy(cam);
        render_3d_sdl_destroy(ctx);
        return 1;
    }

    SpriteRenderer3D* sr = sprite_create(1024, cam, proj);
    if (!sr) {
        fprintf(stderr, "failed to create sprite renderer\n");
        projection_destroy(proj);
        camera_destroy(cam);
        render_3d_sdl_destroy(ctx);
        return 1;
    }
    /* Add many simple sprites spread across the screen */
    for (int i = 0; i < 512; ++i) {
        float x = (float)(i % 32) * 1.5f;
        float y = (float)(i / 32) * 1.5f;
        sprite_add_color(sr, x, y, 1.0f, 0.5f, true, -1, 0, 0x00FF00FF);
    }
    /* Prepare fake column depths */
    float* columns = calloc((size_t)width, sizeof(float));
    for (int x = 0; x < width; ++x)
        columns[x] = 1e6f;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int f = 0; f < frames; ++f) {
        sprite_project_all(sr);
        sprite_sort_by_depth(sr);
        sprite_draw(sr, ctx, columns);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double total_ms = (double)(t1.tv_sec - t0.tv_sec) * 1000.0 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
    printf("sprite_bench: frames=%d screen=%dx%d total_ms=%.3f avg_ms_per_frame=%.6f\n",
           frames, width, height, total_ms, total_ms / frames);
    /* Cleanup */
    free(columns);
    sprite_destroy(sr);
    render_3d_sdl_destroy(ctx);
    return 0;
}
