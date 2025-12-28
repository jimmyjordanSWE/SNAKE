#include "unity.h"
#include "render_3d_projection.h"

void test_projection_wall_pixel_count_consistent(void) {
    Projection3D* p = projection_create(80, 60, 1.0472f, 1.5f);
    TEST_ASSERT_TRUE(p != NULL);

    float distances[] = {0.1f, 0.5f, 1.0f, 2.0f, 5.0f, 20.0f};
    for (size_t i = 0; i < sizeof(distances) / sizeof(distances[0]); i++) {
        WallProjection w;
        projection_project_wall(p, distances[i], &w);
        TEST_ASSERT_TRUE(w.draw_start <= w.draw_end);
        TEST_ASSERT_TRUE(w.draw_start >= 0);
        TEST_ASSERT_TRUE(w.draw_end < 60);
        /* Inclusive pixel count must equal reported wall_height */
        TEST_ASSERT_EQUAL_INT(w.wall_height, (w.draw_end - w.draw_start + 1));
    }

    projection_destroy(p);
}
