#include "unity.h"
#include "snake/tty.h"
#include <limits.h>
#include <stdint.h>


void test_tty_calc_resize_small(void) {
    size_t cells = 0, pixel_bytes = 0, write_sz = 0;
    TEST_ASSERT_TRUE(tty_calc_resize_requirements(80, 24, &cells, &pixel_bytes, &write_sz));
    TEST_ASSERT_TRUE(cells == (size_t)80 * 24);
    TEST_ASSERT_TRUE(pixel_bytes == (size_t)cells * sizeof(struct ascii_pixel));
    TEST_ASSERT_TRUE(write_sz == (size_t)cells * 32);
}

void test_tty_calc_resize_invalid_sizes(void) {
    size_t out1, out2, out3;
    TEST_ASSERT_FALSE(tty_calc_resize_requirements(0, 24, &out1, &out2, &out3));
    TEST_ASSERT_FALSE(tty_calc_resize_requirements(-1, 24, &out1, &out2, &out3));
    TEST_ASSERT_FALSE(tty_calc_resize_requirements(24, 0, &out1, &out2, &out3));
}

void test_tty_calc_resize_pixel_overflow(void) {
    /* cells * sizeof(struct ascii_pixel) overflow */
    size_t max_cells = SIZE_MAX / sizeof(struct ascii_pixel);
    size_t big_cells = max_cells + 1;
    if (big_cells > (size_t)LONG_MAX) {
        return; /* platform 'long' too small for this overflow test */
    }
    long big_w = (long)big_cells;
    TEST_ASSERT_FALSE(tty_calc_resize_requirements(big_w, 1, NULL, NULL, NULL));
}

void test_tty_calc_resize_writebuf_overflow(void) {
    /* cells * 32 overflow */
    size_t max_cells = SIZE_MAX / 32;
    size_t big_cells = max_cells + 1;
    if (big_cells > (size_t)LONG_MAX) {
        return; /* platform 'long' too small for this overflow test */
    }
    long big_w = (long)big_cells;
    TEST_ASSERT_FALSE(tty_calc_resize_requirements(big_w, 1, NULL, NULL, NULL));
}
