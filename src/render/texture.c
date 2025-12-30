#include "render_3d_raycast.h"
#include "render_3d_texture.h"
#include "stb_image.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
struct Texture3D {
    uint32_t shade_colors[TEXTURE_MAX_SHADES];
    uint32_t side_colors[2][TEXTURE_MAX_SHADES];
    uint32_t* pixels;
    int img_w;
    int img_h;
    /* Derived / hoisted constants for hot code paths */
    int pitch; /* equals img_w */
    int w_minus1;
    int h_minus1;
    float mul_x; /* (float)(w_minus1) */
    float mul_y; /* (float)(h_minus1) */
};
static void texture_update_derived(Texture3D* tex);
Texture3D* texture_create(void) {
    Texture3D* t = calloc(1, sizeof *t);
    if (!t)
        return NULL;
    texture_init(t);
    return t;
}
Texture3D* texture_create_procedural(int w, int h) {
    Texture3D* t = texture_create();
    if (!t)
        return NULL;
    if (w <= 0 || h <= 0)
        return t;
    size_t n = (size_t)w * (size_t)h;
    t->img_w = w;
    t->img_h = h;
    t->pixels = malloc(n * sizeof *t->pixels);
    if (!t->pixels) {
        texture_destroy(t);
        return NULL;
    }
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            t->pixels[y * w + x] = 0xFF000000 | ((uint32_t)(y & 0xFF) << 8) | (uint32_t)(x & 0xFF);
        }
    }
    /* update hoisted/derived values for fast sampling */
    texture_update_derived(t);
    return t;
}
void texture_destroy(Texture3D* tex) {
    if (!tex)
        return;
    texture_free_image(tex);
    free(tex);
}
const uint32_t* texture_get_pixels(const Texture3D* tex) {
    return tex ? tex->pixels : NULL;
}
int texture_get_img_w(const Texture3D* tex) {
    return tex ? tex->img_w : 0;
}
int texture_get_img_h(const Texture3D* tex) {
    return tex ? tex->img_h : 0;
}
bool texture_has_image(const Texture3D* tex) {
    return tex && tex->pixels && tex->img_w > 0 && tex->img_h > 0;
}
void texture_init(Texture3D* tex) {
    if (!tex)
        return;
    const uint32_t default_shades[TEXTURE_MAX_SHADES] = {0xFFEEEEEE, 0xFFCCCCCC, 0xFF999999,
                                                         0xFF666666, 0xFF333333, 0xFF000000};
    memcpy(tex->shade_colors, default_shades, sizeof(tex->shade_colors));
    const uint32_t side_v[TEXTURE_MAX_SHADES] = {0xFFDDDDDD, 0xFFBFBFBF, 0xFF8F8F8F,
                                                 0xFF606060, 0xFF303030, 0xFF000000};
    const uint32_t side_h[TEXTURE_MAX_SHADES] = {0xFFCCCCCC, 0xFFAAAAAA, 0xFF777777,
                                                 0xFF505050, 0xFF282828, 0xFF000000};
    memcpy(tex->side_colors[0], side_v, sizeof(side_v));
    memcpy(tex->side_colors[1], side_h, sizeof(side_h));
}
uint8_t texture_shade_from_distance(float distance) {
    if (distance < 3.0f)
        return 0;
    if (distance < 6.0f)
        return 1;
    if (distance < 9.0f)
        return 2;
    if (distance < 12.0f)
        return 3;
    if (distance < 15.0f)
        return 4;
    return TEXTURE_MAX_SHADES - 1;
}
void texture_get_texel(const Texture3D* tex, float distance, bool is_vertical, float tex_coord, Texel* texel_out) {
    if (!tex || !texel_out)
        return;
    float frac = tex_coord - floorf(tex_coord);
    if (frac < 0.0f)
        frac += 1.0f;
    if (tex->pixels && tex->img_w > 0 && tex->img_h > 0) {
        uint32_t col = texture_sample(tex, frac, 1.0f - frac, true);
        texel_out->color = col;
        return;
    }
    const int pattern_resolution = 8;
    int pattern_pos = (int)(frac * (float)pattern_resolution);
    uint8_t base_shade = texture_shade_from_distance(distance);
    uint8_t shade = base_shade + (uint8_t)(pattern_pos % 3);
    if (shade >= TEXTURE_MAX_SHADES)
        shade = TEXTURE_MAX_SHADES - 1;
    if (is_vertical && tex->side_colors[0][shade])
        texel_out->color = tex->side_colors[0][shade];
    else if (!is_vertical && tex->side_colors[1][shade])
        texel_out->color = tex->side_colors[1][shade];
    else
        texel_out->color = tex->shade_colors[shade];
}
void texture_set_shade_chars(Texture3D* tex, const char* chars) {
    (void)tex;
    (void)chars;
}
void texture_set_shade_colors(Texture3D* tex, const uint32_t* colors) {
    if (!tex || !colors)
        return;
    memcpy(tex->shade_colors, colors, TEXTURE_MAX_SHADES * sizeof(uint32_t));
}
static bool is_safe_texture_filename(const char* filename) {
    if (!filename)
        return false;
    if (filename[0] == '/')
        return false; /* absolute paths not allowed */
    if (strstr(filename, "..") != NULL)
        return false; /* reject parent traversal */
    /* reasonable length cap */
    if (strlen(filename) == 0 || strlen(filename) >= 512)
        return false;
    return true;
}
bool texture_load_from_file(Texture3D* tex, const char* filename) {
    if (!tex || !filename)
        return false;
    if (!is_safe_texture_filename(filename))
        return false;
    int w = 0, h = 0, channels = 0;
    unsigned char* data = NULL;
    const int max_parent_depth = 6;
    char buf[1024];
    data = stbi_load(filename, &w, &h, &channels, 4);
    if (data) {
    }
    if (!data) {
        if ((size_t)snprintf(buf, sizeof(buf), "./%s", filename) < sizeof(buf)) {
            data = stbi_load(buf, &w, &h, &channels, 4);
            if (data) {
            }
        }
    }
    if (!data) {
        char exe_path[1024];
        exe_path[0] = '\0';
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len > 0 && len < (ssize_t)(sizeof(exe_path) - 1)) {
            exe_path[len] = '\0';
            char* last = strrchr(exe_path, '/');
            if (last)
                *last = '\0';
            if ((size_t)snprintf(buf, sizeof(buf), "%s/%s", exe_path, filename) < sizeof(buf)) {
                data = stbi_load(buf, &w, &h, &channels, 4);
                if (data) {
                }
            }
        } /* else: readlink result truncated; skip using exe_path */
    }
    for (int depth = 1; depth <= max_parent_depth && !data; depth++) {
        size_t used = 0;
        for (int i = 0; i < depth; i++) {
            if (used + 3 >= sizeof(buf))
                break;
            buf[used++] = '.';
            buf[used++] = '.';
            buf[used++] = '/';
        }
        buf[used] = '\0';
        /* Use snprintf to safely append the filename and ensure we don't overflow */
        size_t avail = sizeof(buf) - used;
        if (avail == 0)
            break;
        int n = snprintf(buf + used, avail, "%s", filename);
        if (n < 0 || (size_t)n >= avail)
            break; /* truncated or error */
        data = stbi_load(buf, &w, &h, &channels, 4);
        if (data) {
        }
    }
    if (!data) {
        const char* base = strrchr(filename, '/');
        if (base)
            base++;
        else
            base = filename;
        data = stbi_load(base, &w, &h, &channels, 4);
        if (data) {
        }
    }
    if (!data) {
        fprintf(stderr,
                "texture_load_from_file: failed to load '%s' (stb_image "
                "returned NULL). Existence check:\n",
                filename);
        char check[1024];
        FILE* f = fopen(filename, "rb");
        fprintf(stderr, "  '%s' -> %s\n", filename, f ? "exists" : "missing");
        if (f)
            fclose(f);
        if ((size_t)snprintf(check, sizeof(check), "./%s", filename) < sizeof(check)) {
            f = fopen(check, "rb");
            fprintf(stderr, "  '%s' -> %s\n", check, f ? "exists" : "missing");
            if (f)
                fclose(f);
        }
        for (int depth = 1; depth <= 6; depth++) {
            size_t used = 0;
            for (int i = 0; i < depth && used + 3 < sizeof(check); i++) {
                check[used++] = '.';
                check[used++] = '.';
                check[used++] = '/';
            }
            check[used] = '\0';
            /* Safely append filename */
            size_t avail = sizeof(check) - used;
            if (avail == 0)
                break;
            int n = snprintf(check + used, avail, "%s", filename);
            if (n < 0 || (size_t)n >= avail)
                break; /* truncated or error */
            f = fopen(check, "rb");
            fprintf(stderr, "  '%s' -> %s\n", check, f ? "exists" : "missing");
            if (f)
                fclose(f);
        }
        const char* base = strrchr(filename, '/');
        if (base)
            base++;
        else
            base = filename;
        f = fopen(base, "rb");
        fprintf(stderr, "  '%s' -> %s\n", base, f ? "exists" : "missing");
        if (f)
            fclose(f);
        return false;
    }
    if (tex->pixels) {
        free(tex->pixels);
        tex->pixels = NULL;
    }
    if (w <= 0 || h <= 0) {
        stbi_image_free(data);
        return false;
    }
    size_t n_pixels = (size_t)w * (size_t)h;
    if (n_pixels / (size_t)w != (size_t)h) {
        stbi_image_free(data);
        return false;
    }
    if (n_pixels > (SIZE_MAX / sizeof(uint32_t))) {
        stbi_image_free(data);
        return false;
    }
    tex->img_w = w;
    tex->img_h = h;
    tex->pixels = malloc(n_pixels * sizeof *tex->pixels);
    if (!tex->pixels) {
        stbi_image_free(data);
        return false;
    }
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int i = (y * w + x) * 4;
            unsigned char r = data[i + 0];
            unsigned char g = data[i + 1];
            unsigned char b = data[i + 2];
            unsigned char a = data[i + 3];
            tex->pixels[y * w + x] = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
        }
    }
    /* update derived values used by hot sampling paths */
    texture_update_derived(tex);
    stbi_image_free(data);
    return true;
}
void texture_free_image(Texture3D* tex) {
    if (!tex)
        return;
    if (tex->pixels) {
        free(tex->pixels);
        tex->pixels = NULL;
    }
    tex->img_w = tex->img_h = 0;
    /* clear derived values */
    tex->pitch = tex->w_minus1 = tex->h_minus1 = 0;
    tex->mul_x = tex->mul_y = 0.0f;
}
static void texture_update_derived(Texture3D* tex) {
    if (!tex)
        return;
    if (tex->pixels && tex->img_w > 0 && tex->img_h > 0) {
        tex->pitch = tex->img_w;
        tex->w_minus1 = tex->img_w - 1;
        tex->h_minus1 = tex->img_h - 1;
        tex->mul_x = (float)tex->w_minus1;
        tex->mul_y = (float)tex->h_minus1;
    } else {
        tex->pitch = tex->w_minus1 = tex->h_minus1 = 0;
        tex->mul_x = tex->mul_y = 0.0f;
    }
}
static inline bool uv_in_unit_range(float u, float v) {
    return u >= 0.0f && u < 1.0f && v >= 0.0f && v < 1.0f;
}
static uint32_t sample_nearest_norm(const Texture3D* tex, float u, float v) {
    int x = (int)(u * (float)tex->img_w);
    int y = (int)(v * (float)tex->img_h);
    if (x < 0)
        x = 0;
    else if (x >= tex->img_w)
        x = tex->img_w - 1;
    if (y < 0)
        y = 0;
    else if (y >= tex->img_h)
        y = tex->img_h - 1;
    return tex->pixels[y * tex->img_w + x];
}
static uint32_t sample_nearest(const Texture3D* tex, float u, float v) {
    if (!tex || !tex->pixels || tex->img_w <= 0 || tex->img_h <= 0)
        return 0;
    /* Fast path when coords already in [0,1): avoid normalization branching */
    if (uv_in_unit_range(u, v))
        return sample_nearest_norm(tex, u, v);
    /* Normalize and then sample */
    if (u >= 0.0f && u < 16777216.0f) {
        u -= (float)(int)u;
    } else {
        u -= floorf(u);
        if (u < 0.0f)
            u += 1.0f;
    }
    if (v >= 0.0f && v < 16777216.0f) {
        v -= (float)(int)v;
    } else {
        v -= floorf(v);
        if (v < 0.0f)
            v += 1.0f;
    }
    return sample_nearest_norm(tex, u, v);
}
/* Fast integer bilinear sampler for u,v in [0,1).
 * Uses Q8 fixed-point interpolation (weights 0..255). Falls back to float path
 * when coordinates fall outside 0..1 or for other safety cases. */
static inline uint32_t sample_bilinear_fast(const Texture3D* tex, float u, float v) {
    /* Assumes u,v in [0,1) and img_w/img_h > 1 */
    const int pitch = tex->pitch;
    const int w_m1 = tex->w_minus1;
    const int h_m1 = tex->h_minus1;
    const float mul_x = tex->mul_x;
    const float mul_y = tex->mul_y;
    float xf = u * mul_x;
    float yf = v * mul_y;
    int x0 = (int)xf;
    int y0 = (int)yf;
    int x1 = x0 + 1;
    if (x1 > w_m1)
        x1 = w_m1;
    int y1 = y0 + 1;
    if (y1 > h_m1)
        y1 = h_m1;
    int sx = (int)((xf - (float)x0) * 256.0f + 0.5f);
    int sy = (int)((yf - (float)y0) * 256.0f + 0.5f);
    int row0 = y0 * pitch;
    int row1 = y1 * pitch;
    uint32_t c00 = tex->pixels[row0 + x0];
    uint32_t c10 = tex->pixels[row0 + x1];
    uint32_t c01 = tex->pixels[row1 + x0];
    uint32_t c11 = tex->pixels[row1 + x1];
    int a00 = (int)((c00 >> 24) & 0xFF);
    int b00 = (int)((c00 >> 16) & 0xFF);
    int g00 = (int)((c00 >> 8) & 0xFF);
    int r00 = (int)(c00 & 0xFF);
    int a10 = (int)((c10 >> 24) & 0xFF);
    int b10 = (int)((c10 >> 16) & 0xFF);
    int g10 = (int)((c10 >> 8) & 0xFF);
    int r10 = (int)(c10 & 0xFF);
    int a01 = (int)((c01 >> 24) & 0xFF);
    int b01 = (int)((c01 >> 16) & 0xFF);
    int g01 = (int)((c01 >> 8) & 0xFF);
    int r01 = (int)(c01 & 0xFF);
    int a11 = (int)((c11 >> 24) & 0xFF);
    int b11 = (int)((c11 >> 16) & 0xFF);
    int g11 = (int)((c11 >> 8) & 0xFF);
    int r11 = (int)(c11 & 0xFF);
    int a0 = (a00 * (256 - sx) + a10 * sx + 128) >> 8;
    int r0 = (r00 * (256 - sx) + r10 * sx + 128) >> 8;
    int g0 = (g00 * (256 - sx) + g10 * sx + 128) >> 8;
    int b0 = (b00 * (256 - sx) + b10 * sx + 128) >> 8;
    int a1 = (a01 * (256 - sx) + a11 * sx + 128) >> 8;
    int r1 = (r01 * (256 - sx) + r11 * sx + 128) >> 8;
    int g1 = (g01 * (256 - sx) + g11 * sx + 128) >> 8;
    int b1 = (b01 * (256 - sx) + b11 * sx + 128) >> 8;
    int a = (a0 * (256 - sy) + a1 * sy + 128) >> 8;
    int r = (r0 * (256 - sy) + r1 * sy + 128) >> 8;
    int g = (g0 * (256 - sy) + g1 * sy + 128) >> 8;
    int b = (b0 * (256 - sy) + b1 * sy + 128) >> 8;
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
}
static uint32_t sample_bilinear_slow_normalized(const Texture3D* tex, float u, float v) {
    /* Assumes u,v normalized into [0,1) */
    const int pitch = tex->pitch;
    const float mul_x = tex->mul_x;
    const float mul_y = tex->mul_y;
    float x = u * mul_x;
    float y = v * mul_y;
    int x0 = (int)x;
    int x1 = x0 + 1;
    if (x1 > tex->w_minus1)
        x1 = tex->w_minus1;
    int y0 = (int)y;
    int y1 = y0 + 1;
    if (y1 > tex->h_minus1)
        y1 = tex->h_minus1;
    float sx = x - (float)x0;
    float sy = y - (float)y0;
    int row0 = y0 * pitch;
    int row1 = y1 * pitch;
    uint32_t c00 = tex->pixels[row0 + x0];
    uint32_t c10 = tex->pixels[row0 + x1];
    uint32_t c01 = tex->pixels[row1 + x0];
    uint32_t c11 = tex->pixels[row1 + x1];
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
static uint32_t sample_bilinear(const Texture3D* tex, float u, float v) {
    if (!tex || !tex->pixels || tex->img_w <= 0 || tex->img_h <= 0)
        return 0;
    /* Fast path when coords already in [0,1): avoid normalization cost */
    if (uv_in_unit_range(u, v) && tex->img_w > 1 && tex->img_h > 1)
        return sample_bilinear_fast(tex, u, v);
    /* Normalize coords and try fast path again */
    if (u >= 0.0f && u < 16777216.0f) {
        u -= (float)(int)u;
    } else {
        u -= floorf(u);
        if (u < 0.0f)
            u += 1.0f;
    }
    if (v >= 0.0f && v < 16777216.0f) {
        v -= (float)(int)v;
    } else {
        v -= floorf(v);
        if (v < 0.0f)
            v += 1.0f;
    }
    if (uv_in_unit_range(u, v) && tex->img_w > 1 && tex->img_h > 1)
        return sample_bilinear_fast(tex, u, v);
    return sample_bilinear_slow_normalized(tex, u, v);
}
uint32_t texture_sample(const Texture3D* tex, float u, float v, bool bilinear) {
    if (!tex || !tex->pixels)
        return 0;
    if (!bilinear)
        return sample_nearest(tex, u, v);
    return sample_bilinear(tex, u, v);
}
