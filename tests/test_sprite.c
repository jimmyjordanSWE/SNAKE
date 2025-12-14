#include <stdio.h>
#include "snake/render_3d_sprite.h"
#include "snake/render_3d_camera.h"
#include "snake/types.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_sdl.h"
#include <stdlib.h>
#include <string.h>

static int run_additional_tests(void);

int main(void) {
    SpriteRenderer3D sr_stack;
    /* initialize with small capacity; pass NULL camera/proj for test stub */
    sprite_init(&sr_stack, 8, NULL, NULL);
    if (!sprite_add(&sr_stack, 1.0f, 1.0f, 1.0f, 0.0f, false, -1, 0)) {
        fprintf(stderr, "sprite_add failed\n");
        return 1;
    }
    sprite_clear(&sr_stack);
    sprite_shutdown(&sr_stack);
    int ret = run_additional_tests();
    if (ret != 0) {
        fprintf(stderr, "additional tests failed: %d\n", ret);
        return ret;
    }
    printf("test_sprite: OK\n");
    return 0;
}

/* Additional functional tests: projection and occlusion */
__attribute__((unused)) static int run_additional_tests(void) {
    Camera3D cam;
    camera_init(&cam, 90.0f, 64, 0.5f);
    /* place camera at (1,1) facing +X */
    camera_set_from_player(&cam, 1, 1, SNAKE_DIR_RIGHT);
    /* ensure interpolation returns current camera position */
    camera_set_interpolation_time(&cam, cam.update_interval);
    Projection3D proj;
    projection_init(&proj, 64, 32, cam.fov_radians);
    SpriteRenderer3D sr;
    sprite_init(&sr, 4, &cam, &proj);
    int rc = 0;
    /* sprite directly in front of camera */
    sprite_add(&sr, cam.x + 1.0f, cam.y, 1.0f, 0.0f, true, -1, 0);
    sprite_project_all(&sr);
    if (sr.count != 1) { rc = 1; goto cleanup; }
    Sprite3D* s = &sr.sprites[0];
    if (!s->visible) { rc = 2; goto cleanup; }
    /* expect roughly center X */
    if (abs(s->screen_x - (proj.screen_width / 2)) > 16) { rc = 3; goto cleanup; }
    /* test occlusion: draw with column depths closer than sprite -> no pixels written */
    SDL3DContext ctx = {0};
    ctx.width = proj.screen_width;
    ctx.height = proj.screen_height;
    ctx.pixels = calloc((size_t)ctx.width * (size_t)ctx.height, sizeof(uint32_t));
    float* depths = malloc(sizeof(float) * (size_t)ctx.width);
    for (int i = 0; i < ctx.width; ++i) depths[i] = 0.5f; /* closer than sprite */
    sprite_draw(&sr, &ctx, depths);
    /* check center column around sprite x for no drawing (still zero) */
    int cx = s->screen_x;
    int any_nonzero = 0;
    for (int y = 0; y < ctx.height; ++y) if (ctx.pixels[y * ctx.width + cx] != 0) any_nonzero = 1;
    if (any_nonzero) { rc = 4; goto cleanup; }
    /* now allow drawing */
    for (int i = 0; i < ctx.width; ++i) depths[i] = INFINITY;
    sprite_draw(&sr, &ctx, depths);
    any_nonzero = 0;
    for (int y = 0; y < ctx.height; ++y) if (ctx.pixels[y * ctx.width + cx] != 0) any_nonzero = 1;
    if (!any_nonzero) { rc = 5; goto cleanup; }
    /* pivot test: bottom vs top anchoring */
    sprite_clear(&sr);
    sprite_add(&sr, cam.x + 1.0f, cam.y, 1.0f, 0.0f, true, -1, 0);
    sprite_add(&sr, cam.x + 1.0f, cam.y + 0.0f, 1.0f, 1.0f, true, -1, 0);
    sprite_project_all(&sr);
    if (sr.count < 2) { rc = 6; goto cleanup; }
    Sprite3D* s0 = &sr.sprites[0];
    Sprite3D* s1 = &sr.sprites[1];
    if (!(s0->screen_y_top < s1->screen_y_top)) { rc = 7; goto cleanup; }
cleanup:
    if (ctx.pixels) free(ctx.pixels);
    if (depths) free(depths);
    sprite_shutdown(&sr);
    return rc;
}
