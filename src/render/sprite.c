#include "render_3d_camera.h"
#include "render_3d_projection.h"
#include "render_3d_sdl.h"
#include "render_3d_sprite.h"
#include "render_3d_sprite_internal.h"
#include <stddef.h>
#include <stdlib.h>

struct SpriteRenderer3D {
    Sprite3D* sprites;
    int max_sprites;
    int count;
    const Camera3D* camera;
    const Projection3D* proj;
};
#include <math.h>
#include <stdlib.h>
#include <string.h>
void sprite_init(SpriteRenderer3D* sr, int max_sprites, const Camera3D* camera, const Projection3D* proj) {
    if (!sr)
        return;
    sr->sprites = calloc((size_t)max_sprites, sizeof(Sprite3D));
    if (!sr->sprites) {
        sr->max_sprites = 0;
        sr->count = 0;
        sr->camera = camera;
        sr->proj = proj;
        return;
    }
    sr->max_sprites = max_sprites;
    sr->count = 0;
    sr->camera = camera;
    sr->proj = proj;
}
void sprite_clear(SpriteRenderer3D* sr) {
    if (!sr)
        return;
    sr->count = 0;
}
bool sprite_add(SpriteRenderer3D* sr,
                float world_x,
                float world_y,
                float world_height,
                float pivot,
                bool face_camera,
                int texture_id,
                int frame) {
    if (!sr || sr->count >= sr->max_sprites)
        return false;
    Sprite3D* s = &sr->sprites[sr->count++];
    s->world_x = world_x;
    s->world_y = world_y;
    s->world_height = world_height;
    s->pivot = pivot;
    s->face_camera = face_camera;
    s->texture_id = texture_id;
    s->frame = frame;
    s->color = render_3d_sdl_color(0, 128, 0, 255);
    s->perp_distance = 0.0f;
    s->screen_x = s->screen_w = s->screen_h = s->screen_y_top = 0;
    s->visible = false;
    return true;
}
bool sprite_add_color(SpriteRenderer3D* sr,
                      float world_x,
                      float world_y,
                      float world_height,
                      float pivot,
                      bool face_camera,
                      int texture_id,
                      int frame,
                      uint32_t color) {
    if (!sr || sr->count >= sr->max_sprites)
        return false;
    Sprite3D* s = &sr->sprites[sr->count++];
    s->world_x = world_x;
    s->world_y = world_y;
    s->world_height = world_height;
    s->pivot = pivot;
    s->face_camera = face_camera;
    s->texture_id = texture_id;
    s->frame = frame;
    s->color = color;
    s->perp_distance = 0.0f;
    s->screen_x = s->screen_w = s->screen_h = s->screen_y_top = 0;
    s->visible = false;
    s->is_rect = false;
    return true;
}
bool sprite_add_rect_color(SpriteRenderer3D* sr,
                           float world_x,
                           float world_y,
                           float world_height,
                           float pivot,
                           bool face_camera,
                           int texture_id,
                           int frame,
                           uint32_t color) {
    if (!sr || sr->count >= sr->max_sprites)
        return false;
    Sprite3D* s = &sr->sprites[sr->count++];
    s->world_x = world_x;
    s->world_y = world_y;
    s->world_height = world_height;
    s->pivot = pivot;
    s->face_camera = face_camera;
    s->texture_id = texture_id;
    s->frame = frame;
    s->color = color;
    s->perp_distance = 0.0f;
    s->screen_x = s->screen_w = s->screen_h = s->screen_y_top = 0;
    s->visible = false;
    s->is_rect = true;
    return true;
}
void sprite_project_all(SpriteRenderer3D* sr) {
    if (!sr || !sr->camera || !sr->proj)
        return;
    float cam_x, cam_y;
    camera_get_interpolated_position(sr->camera, &cam_x, &cam_y);
    float cam_angle = camera_get_interpolated_angle(sr->camera);
    float half_fov = projection_get_fov_radians(sr->proj) * 0.5f;
    for (int i = 0; i < sr->count; ++i) {
        Sprite3D* s = &sr->sprites[i];
        float dx = s->world_x - cam_x;
        float dy = s->world_y - cam_y;
        float dist = sqrtf(dx * dx + dy * dy);
        float angle_to_sprite = atan2f(dy, dx);
        float delta = angle_to_sprite - cam_angle;
        while (delta > 3.14159265358979323846f)
            delta -= 2.0f * 3.14159265358979323846f;
        while (delta < -3.14159265358979323846f)
            delta += 2.0f * 3.14159265358979323846f;
        if (cosf(delta) <= 0.0f || fabsf(delta) > half_fov) {
            s->visible = false;
            continue;
        }
        float perp = dist * cosf(delta);
        if (perp <= 0.0f) {
            s->visible = false;
            continue;
        }
        WallProjection wp;
        projection_project_wall_perp(sr->proj, perp, angle_to_sprite, cam_angle, &wp);
        int screen_h = (int)((float)wp.wall_height * s->world_height + 0.5f);
        if (screen_h <= 0) {
            s->visible = false;
            continue;
        }
        int screen_w = screen_h;
        if (s->is_rect) {
            screen_w = (int)((float)screen_h * 1.5f + 0.5f);
        }
        float nx = delta / half_fov;
        if (nx < -1.0f)
            nx = -1.0f;
        if (nx > 1.0f)
            nx = 1.0f;
        int center_x = (int)((nx + 1.0f) * 0.5f * (float)projection_get_screen_width(sr->proj) + 0.5f);
        int top = wp.draw_end - (int)((float)screen_h * (1.0f - s->pivot));
        s->perp_distance = perp;
        s->screen_x = center_x;
        s->screen_w = screen_w;
        s->screen_h = screen_h;
        s->screen_y_top = top;
        s->visible = true;
    }
    for (int i = 0; i < sr->count; ++i) {
        Sprite3D* a = &sr->sprites[i];
        if (a->texture_id == -1)
            continue;
        for (int j = 0; j < sr->count; ++j) {
            if (i == j)
                continue;
            Sprite3D* b = &sr->sprites[j];
            if (b->texture_id != -1)
                continue;
            if (fabsf(a->world_x - b->world_x) < 1e-3f && fabsf(a->world_y - b->world_y) < 1e-3f) {
                a->perp_distance -= 1e-3f;
                break;
            }
        }
    }
}
void sprite_sort_by_depth(SpriteRenderer3D* sr) {
    if (!sr)
        return;
    for (int i = 1; i < sr->count; ++i) {
        Sprite3D key = sr->sprites[i];
        int j = i - 1;
        while (j >= 0 && (sr->sprites[j].perp_distance < key.perp_distance ||
                          (fabsf(sr->sprites[j].perp_distance - key.perp_distance) < 1e-4f &&
                           sr->sprites[j].texture_id != -1 && key.texture_id == -1))) {
            sr->sprites[j + 1] = sr->sprites[j];
            --j;
        }
        sr->sprites[j + 1] = key;
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (int k = 0; k + 1 < sr->count; ++k) {
            Sprite3D* a = &sr->sprites[k];
            Sprite3D* b = &sr->sprites[k + 1];
            if (a->texture_id != -1 && b->texture_id == -1 && fabsf(a->world_x - b->world_x) < 0.6f &&
                fabsf(a->world_y - b->world_y) < 0.6f) {
                Sprite3D tmp = *a;
                *a = *b;
                *b = tmp;
                changed = true;
            }
        }
    }
}
void sprite_draw(SpriteRenderer3D* sr, SDL3DContext* ctx, const float* column_depths) {
    if (!sr || !ctx || !column_depths)
        return;
    for (int i = 0; i < sr->count; ++i) {
        Sprite3D* s = &sr->sprites[i];
        if (!s->visible)
            continue;
        int half_w = s->screen_w / 2;
        int x1 = s->screen_x - half_w;
        int x2 = s->screen_x + half_w;
        if (x1 < 0)
            x1 = 0;
        if (x2 >= render_3d_sdl_get_width(ctx))
            x2 = render_3d_sdl_get_width(ctx) - 1;
        int y0 = s->screen_y_top;
        int y1 = y0 + s->screen_h - 1;
        if (y0 < 0)
            y0 = 0;
        if (y1 >= render_3d_sdl_get_height(ctx))
            y1 = render_3d_sdl_get_height(ctx) - 1;
        uint32_t col = s->color ? s->color : render_3d_sdl_color(0, 128, 0, 255);
        if (s->texture_id == -1) {
            int center_x = s->screen_x;
            int center_y = s->screen_y_top + s->screen_h / 2;
            int radius = s->screen_w / 2;
            if (radius <= 0)
                radius = 1;
            if (s->is_rect) {
                int rw = s->screen_w;
                int rh = s->screen_h;
                int rx0 = center_x - rw / 2;
                int ry0 = center_y - rh / 2;
                for (int yy = ry0; yy < ry0 + rh; ++yy) {
                    if (yy < 0 || yy >= render_3d_sdl_get_height(ctx))
                        continue;
                    for (int xx = rx0; xx < rx0 + rw; ++xx) {
                        if (xx < 0 || xx >= render_3d_sdl_get_width(ctx))
                            continue;
                        if (s->perp_distance < column_depths[xx]) {
                            render_3d_sdl_set_pixel(ctx, xx, yy, col);
                        }
                    }
                }
            } else {
                int bx0 = center_x - radius;
                if (bx0 < 0)
                    bx0 = 0;
                int bx1 = center_x + radius;
                if (bx1 >= render_3d_sdl_get_width(ctx))
                    bx1 = render_3d_sdl_get_width(ctx) - 1;
                int by0 = center_y - radius;
                if (by0 < 0)
                    by0 = 0;
                int by1 = center_y + radius;
                if (by1 >= render_3d_sdl_get_height(ctx))
                    by1 = render_3d_sdl_get_height(ctx) - 1;
                for (int yy = by0; yy <= by1; ++yy) {
                    for (int xx = bx0; xx <= bx1; ++xx) {
                        int dx = xx - center_x;
                        int dy = yy - center_y;
                        if (dx * dx + dy * dy <= radius * radius) {
                            if (s->perp_distance < column_depths[xx]) {
                                render_3d_sdl_set_pixel(ctx, xx, yy, col);
                            }
                        }
                    }
                }
            }
        } else {
            for (int x = x1; x <= x2; ++x) {
                if (s->perp_distance < column_depths[x]) {
                    render_3d_sdl_draw_column(ctx, x, y0, y1, col);
                }
            }
        }
    }
}
void sprite_shutdown(SpriteRenderer3D* sr) {
    if (!sr)
        return;
    free(sr->sprites);
    sr->sprites = NULL;
    sr->max_sprites = 0;
    sr->count = 0;
    sr->camera = NULL;
    sr->proj = NULL;
}
SpriteRenderer3D* sprite_create(int max_sprites, const Camera3D* camera, const Projection3D* proj) {
    SpriteRenderer3D* sr = (SpriteRenderer3D*)calloc(1, sizeof(*sr));
    if (!sr)
        return NULL;
    sprite_init(sr, max_sprites, camera, proj);
    return sr;
}
void sprite_destroy(SpriteRenderer3D* sr) {
    if (!sr)
        return;
    sprite_shutdown(sr);
    free(sr);
}
int sprite_get_count(const SpriteRenderer3D* sr) {
    return sr ? sr->count : 0;
}
bool sprite_get_screen_info(const SpriteRenderer3D* sr,
                            int idx,
                            int* screen_x_out,
                            int* screen_h_out,
                            bool* visible_out) {
    if (!sr || idx < 0 || idx >= sr->count)
        return false;
    const Sprite3D* s = &sr->sprites[idx];
    if (visible_out)
        *visible_out = s->visible;
    if (screen_x_out)
        *screen_x_out = s->screen_x;
    if (screen_h_out)
        *screen_h_out = s->screen_h;
    return true;
}
int sprite_get_texture_id(const SpriteRenderer3D* sr, int idx, int* texture_id_out) {
    if (!sr || idx < 0 || idx >= sr->count)
        return 0;
    if (texture_id_out)
        *texture_id_out = sr->sprites[idx].texture_id;
    return 1;
}
