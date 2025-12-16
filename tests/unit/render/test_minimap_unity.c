#include "snake/render_3d.h"
#include "unity.h"

static void test_minimap_cell_px_large_display(void) {
    int px = render_3d_compute_minimap_cell_px(800, 600, 48, 24);
    TEST_ASSERT_TRUE_MSG(px >= 4, "cell_px should be >=4 for 48x24 on 800x600");
}

static void test_minimap_cell_px_canonical_display(void) {
    int px = render_3d_compute_minimap_cell_px(800, 600, 40, 20);
    TEST_ASSERT_TRUE_MSG(px >= 4, "cell_px should be >=4 for 40x20 on 800x600");
}

static void test_minimap_cell_px_80x80(void) {
    int px = render_3d_compute_minimap_cell_px(800, 600, 80, 80);
    TEST_ASSERT_TRUE_MSG(px >= 3, "cell_px should be >=3 for 80x80 on 800x600");
}

static void test_minimap_cell_px_small_display(void) {
    int px = render_3d_compute_minimap_cell_px(200, 100, 48, 24);
    TEST_ASSERT_TRUE_MSG(px >= 2, "cell_px should be >=2 on small displays");
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_minimap_cell_px_large_display);
    RUN_TEST(test_minimap_cell_px_canonical_display);
    RUN_TEST(test_minimap_cell_px_80x80);
    RUN_TEST(test_minimap_cell_px_small_display);
    return UnityEnd();
}
