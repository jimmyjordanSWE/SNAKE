#include "snake/render_3d_texture.h"
#include "snake/render_3d_raycast.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    Texture3D tex;
    texture_init(&tex);
    Texel a = {0}, b = {0};
    /* two tex_coords on different halves should produce different colors */
    texture_get_texel(&tex, 2.0f, true, 0.1f, &a);
    texture_get_texel(&tex, 2.0f, true, 0.6f, &b);
    if (a.color == b.color) {
        fprintf(stderr, "texture stripe pattern failed: colors equal (0x%08x == 0x%08x)\n", a.color, b.color);
        return 1;
    }
    /* vertical vs horizontal should prefer different side colors */
    Texel v = {0}, h = {0};
    texture_get_texel(&tex, 1.0f, true, 0.25f, &v);
    texture_get_texel(&tex, 1.0f, false, 0.25f, &h);
    if (v.color == h.color) {
        fprintf(stderr, "vertical/horizontal side color difference not observed (0x%08x == 0x%08x)\n", v.color, h.color);
        return 1;
    }
    /* raycast texture coord fractional mapping */
    RayHit hit = {0};
    hit.hit_x = 3.25f;
    hit.hit_y = 4.75f;
    float vc = raycast_get_texture_coord(&hit, true);
    float hc = raycast_get_texture_coord(&hit, false);
    if (vc < 0.749f || vc > 0.751f) { fprintf(stderr, "vertical coord %.6f not ~0.75\n", vc); return 1; }
    if (hc < 0.249f || hc > 0.251f) { fprintf(stderr, "horizontal coord %.6f not ~0.25\n", hc); return 1; }
    return 0;
}