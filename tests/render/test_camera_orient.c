#include <stdio.h>
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_sprite.h"
#include "snake/persist.h"
#include "snake/types.h"
#include <stdlib.h>

int main(void) {
    Camera3D cam;
    camera_init(&cam, 90.0f, 64, 0.5f);
    Projection3D proj;
    projection_init(&proj, 64, 32, cam.fov_radians, (float)PERSIST_CONFIG_DEFAULT_WALL_SCALE);
    SpriteRenderer3D sr;
    int rc = 0;
    sprite_init(&sr, 4, &cam, &proj);
    /* Test each snake direction: place sprite one cell in front and expect it visible and near center */
    for (int dir = SNAKE_DIR_UP; dir <= SNAKE_DIR_RIGHT; ++dir) {
        int px = 5, py = 5;
        camera_set_from_player(&cam, px, py, dir);
        camera_set_interpolation_time(&cam, cam.update_interval);
        sprite_clear(&sr);
        switch (dir) {
            case SNAKE_DIR_UP: sprite_add(&sr, cam.x, cam.y - 1.0f, 1.0f, 0.0f, true, -1, 0); break;
            case SNAKE_DIR_DOWN: sprite_add(&sr, cam.x, cam.y + 1.0f, 1.0f, 0.0f, true, -1, 0); break;
            case SNAKE_DIR_LEFT: sprite_add(&sr, cam.x - 1.0f, cam.y, 1.0f, 0.0f, true, -1, 0); break;
            case SNAKE_DIR_RIGHT: sprite_add(&sr, cam.x + 1.0f, cam.y, 1.0f, 0.0f, true, -1, 0); break;
        }
        sprite_project_all(&sr);
        if (sr.count != 1) { rc = 1; break; }
        if (!sr.sprites[0].visible) { rc = 2; break; }
        if (abs(sr.sprites[0].screen_x - (proj.screen_width / 2)) > 16) { rc = 3; break; }
    }
    sprite_shutdown(&sr);
    return rc;
}
