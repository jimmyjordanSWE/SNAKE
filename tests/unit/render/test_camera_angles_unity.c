
#include "unity.h"
#include "snake/render_3d_camera.h"
#include "snake/types.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float dir_to_angle(int dir) {
    switch(dir) {
        case SNAKE_DIR_UP: return (float)M_PI * 1.5f;
        case SNAKE_DIR_DOWN: return (float)M_PI * 0.5f;
        case SNAKE_DIR_LEFT: return (float)M_PI;
        case SNAKE_DIR_RIGHT: return 0.0f;
        default: return 0.0f;
    }
}

static void test_interpolated_angle_and_dir_at_zero_interp(void) {
    Camera3D* cam = camera_create(90.0f, 64, 0.5f);
    TEST_ASSERT_TRUE_MSG(cam != NULL, "camera_create failed");

    for (int dir = SNAKE_DIR_UP; dir <= SNAKE_DIR_RIGHT; ++dir) {
        camera_set_from_player(cam, 5, 5, dir);

        
        float a = camera_get_interpolated_angle(cam);
        float expected = dir_to_angle(dir);

        
        float d = a - expected;
        while (d > (float)M_PI) d -= 2.0f * (float)M_PI;
        while (d < -(float)M_PI) d += 2.0f * (float)M_PI;
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, d);

        float dx = cosf(a);
        float dy = sinf(a);
        float dir_x, dir_y;
        camera_get_dir(cam, &dir_x, &dir_y);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, dx, dir_x);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, dy, dir_y);
    }

    camera_destroy(cam);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_interpolated_angle_and_dir_at_zero_interp);
    return UnityEnd();
}
