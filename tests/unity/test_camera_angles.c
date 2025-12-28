#include "unity.h"
#include "render_3d_camera.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static float angle_diff(float a, float b) {
    float d = a - b;
    while (d > (float)M_PI)
        d -= 2.0f * (float)M_PI;
    while (d < -(float)M_PI)
        d += 2.0f * (float)M_PI;
    return d;
}

void test_camera_cached_offsets_match_direct_per_column(void) {
    const float eps = 1e-6f;
    const int widths[] = {16, 64, 320};
    const float fovs[] = {30.0f, 60.0f, 90.0f};

    for (size_t iw = 0; iw < sizeof(widths) / sizeof(widths[0]); iw++) {
        for (size_t ifo = 0; ifo < sizeof(fovs) / sizeof(fovs[0]); ifo++) {
            int w = widths[iw];
            float fov = fovs[ifo];
            Camera3D* cam = camera_create(fov, w, 0.1f);
            TEST_ASSERT_TRUE(cam != NULL);

            /* set some interpolation state to ensure interpolated angle is meaningful */
            camera_set_prev_angle(cam, 0.0f);
            camera_set_angle(cam, 0.12345f);
            camera_set_interpolation_time(cam, 0.5f);

            /* Prepare cached offsets (should compute once and be available) */
            camera_prepare_angle_offsets(cam, w);
            const float* cached = camera_get_cached_angle_offsets(cam);
            TEST_ASSERT_TRUE(cached != NULL);

            float interp = camera_get_interpolated_angle(cam);
            for (int col = 0; col < w; col++) {
                float direct = 0.0f;
                camera_get_ray_angle(cam, col, &direct);
                float expected_offset = angle_diff(direct, interp);
                float cached_value = cached[col];
                float d = fabsf(cached_value - expected_offset);
                TEST_ASSERT_TRUE(d <= eps);
            }

            camera_destroy(cam);
        }
    }
}
