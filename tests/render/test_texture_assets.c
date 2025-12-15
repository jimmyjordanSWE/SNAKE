#include "snake/render_3d_texture.h"
#include <stdio.h>
#include <unistd.h>
int main(void) {
    Texture3D tex = {0};
    texture_init(&tex);
    /* Ensure asset file exists and can be loaded */
    if(access("assets/wall.png", R_OK) != 0) {
        fprintf(stderr, "missing asset: assets/wall.png\n");
        return 1;
    }
    if(!texture_load_from_file(&tex, "assets/wall.png")) {
        fprintf(stderr, "failed to load assets/wall.png\n");
        return 1;
    } else {
        fprintf(stderr, "loaded assets/wall.png (%dx%d)\n", tex.img_w, tex.img_h);
        uint32_t c1 = texture_sample(&tex, 0.0f, 0.0f, false);
        uint32_t c2 = texture_sample(&tex, 0.5f, 0.5f, false);
        fprintf(stderr, "samples: 0,0 = 0x%08x; 0.5,0.5 = 0x%08x\n", c1, c2);
    }
    texture_free_image(&tex);
    Texture3D f = {0}; texture_init(&f);
    if(access("assets/floor.png", R_OK) != 0) {
        fprintf(stderr, "missing asset: assets/floor.png\n");
        texture_free_image(&tex);
        return 1;
    }
    if(!texture_load_from_file(&f, "assets/floor.png")) {
        fprintf(stderr, "failed to load assets/floor.png\n");
        texture_free_image(&tex);
        return 1;
    } else {
        fprintf(stderr, "loaded assets/floor.png (%dx%d)\n", f.img_w, f.img_h);
        uint32_t c1 = texture_sample(&f, 0.0f, 0.0f, false);
        uint32_t c2 = texture_sample(&f, 0.5f, 0.5f, false);
        fprintf(stderr, "floor samples: 0,0 = 0x%08x; 0.5,0.5 = 0x%08x\n", c1, c2);
    }
    texture_free_image(&f);
    return 0;
}
