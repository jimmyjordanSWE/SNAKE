#include "unity.h"
#include "render_3d_texture.h"
#include <math.h>

static uint32_t reference_bilinear(const Texture3D* tex, float u, float v) {
    const uint32_t* pixels = texture_get_pixels(tex);
    int w = texture_get_img_w(tex);
    int h = texture_get_img_h(tex);
    if (!pixels || w <= 0 || h <= 0)
        return 0;
    /* normalize u,v into [0,1) */
    u -= floorf(u);
    if (u < 0.0f) u += 1.0f;
    v -= floorf(v);
    if (v < 0.0f) v += 1.0f;
    float xf = u * (float)(w - 1);
    float yf = v * (float)(h - 1);
    int x0 = (int)xf;
    int y0 = (int)yf;
    int x1 = x0 + 1; if (x1 >= w) x1 = w - 1;
    int y1 = y0 + 1; if (y1 >= h) y1 = h - 1;
    float sx = xf - (float)x0;
    float sy = yf - (float)y0;
    uint32_t c00 = pixels[y0 * w + x0];
    uint32_t c10 = pixels[y0 * w + x1];
    uint32_t c01 = pixels[y1 * w + x0];
    uint32_t c11 = pixels[y1 * w + x1];
    float a00 = (float)((c00 >> 24) & 0xFF);
    float b00 = (float)((c00 >> 16) & 0xFF);
    float g00 = (float)((c00 >> 8) & 0xFF);
    float r00 = (float)(c00 & 0xFF);
    float a10 = (float)((c10 >> 24) & 0xFF);
    float b10 = (float)((c10 >> 16) & 0xFF);
    float g10 = (float)((c10 >> 8) & 0xFF);
    float r10 = (float)(c10 & 0xFF);
    float a01 = (float)((c01 >> 24) & 0xFF);
    float b01 = (float)((c01 >> 16) & 0xFF);
    float g01 = (float)((c01 >> 8) & 0xFF);
    float r01 = (float)(c01 & 0xFF);
    float a11 = (float)((c11 >> 24) & 0xFF);
    float b11 = (float)((c11 >> 16) & 0xFF);
    float g11 = (float)((c11 >> 8) & 0xFF);
    float r11 = (float)(c11 & 0xFF);
    float a0 = a00 * (1 - sx) + a10 * sx;
    float r0 = r00 * (1 - sx) + r10 * sx;
    float g0 = g00 * (1 - sx) + g10 * sx;
    float b0 = b00 * (1 - sx) + b10 * sx;
    float a1 = a01 * (1 - sx) + a11 * sx;
    float r1 = r01 * (1 - sx) + r11 * sx;
    float g1 = g01 * (1 - sx) + g11 * sx;
    float b1 = b01 * (1 - sx) + b11 * sx;
    float a = a0 * (1 - sy) + a1 * sy;
    float r = r0 * (1 - sy) + r1 * sy;
    float g = g0 * (1 - sy) + g1 * sy;
    float b = b0 * (1 - sy) + b1 * sy;
    uint32_t ia = (uint32_t)(a + 0.5f);
    uint32_t ir = (uint32_t)(r + 0.5f);
    uint32_t ig = (uint32_t)(g + 0.5f);
    uint32_t ib = (uint32_t)(b + 0.5f);
    return (ia << 24) | (ib << 16) | (ig << 8) | ir;
}

void test_texture_bilinear_fast_matches_reference_for_inrange(void) {
    Texture3D* t = texture_create_procedural(16, 16);
    TEST_ASSERT_TRUE(t != NULL);
    for (float u = 0.0f; u < 1.0f; u += 0.07f) {
        for (float v = 0.0f; v < 1.0f; v += 0.11f) {
            uint32_t c_fast = texture_sample(t, u, v, true);
            uint32_t c_ref = reference_bilinear(t, u, v);
            TEST_ASSERT_EQUAL_HEX32(c_ref, c_fast);
        }
    }
    texture_destroy(t);
}
