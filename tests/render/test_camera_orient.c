#include <stdio.h>
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_sprite.h"
#include "snake/persist.h"
#include "snake/types.h"
#include <stdlib.h>

int main(void) {
    Camera3D* cam = camera_create(90.0f, 64, 0.5f);
    if(!cam) return 2;
    float fov = camera_get_fov_radians(cam);
    Projection3D* proj = projection_create(64, 32, fov, (float)PERSIST_CONFIG_DEFAULT_WALL_SCALE);
    SpriteRenderer3D* sr = sprite_create(4, cam, proj);
    if(!sr) { projection_destroy(proj); camera_destroy(cam); return 2; }
    int rc = 0;
    
    for (int dir = SNAKE_DIR_UP; dir <= SNAKE_DIR_RIGHT; ++dir) {
        int px = 5, py = 5;
        camera_set_from_player(cam, px, py, dir);
        camera_set_interpolation_time(cam, camera_get_update_interval(cam));
        sprite_clear(sr);
        float cx, cy;
        camera_get_position(cam, &cx, &cy);
        switch (dir) {
            case SNAKE_DIR_UP: sprite_add(sr, cx, cy - 1.0f, 1.0f, 0.0f, true, -1, 0); break;
            case SNAKE_DIR_DOWN: sprite_add(sr, cx, cy + 1.0f, 1.0f, 0.0f, true, -1, 0); break;
            case SNAKE_DIR_LEFT: sprite_add(sr, cx - 1.0f, cy, 1.0f, 0.0f, true, -1, 0); break;
            case SNAKE_DIR_RIGHT: sprite_add(sr, cx + 1.0f, cy, 1.0f, 0.0f, true, -1, 0); break;
        }
        sprite_project_all(sr);
        if (sprite_get_count(sr) != 1) { rc = 1; break; }
        int sx, sh; bool vis;
        if(!sprite_get_screen_info(sr, 0, &sx, &sh, &vis)) { rc = 2; break; }
        if (!vis) { rc = 3; break; }
        if (abs(sx - (projection_get_screen_width(proj) / 2)) > 16) { rc = 4; break; }
    }
    sprite_destroy(sr);
    projection_destroy(proj);
    camera_destroy(cam);
    return rc;
}
