#include "snake/utils.h"
#include "unity.h"
#include <stdint.h>
#include <limits.h>

static void test_rng_repeatability(void);
static void test_rng_range_small(void);
static void test_rng_range_full_int(void);

void test_rng_repeatability(void) {
    uint32_t s1 = 0, s2 = 0;
    snake_rng_seed(&s1, 12345);
    snake_rng_seed(&s2, 12345);
    for(int i=0;i<100;i++) {
        uint32_t a = snake_rng_next_u32(&s1);
        uint32_t b = snake_rng_next_u32(&s2);
        TEST_ASSERT_EQUAL_INT(a, b);
    }
}

void test_rng_range_small(void) {
    uint32_t s = 0; snake_rng_seed(&s, 12345);
    for(int i=0;i<100;i++) {
        int r = snake_rng_range(&s, -5, 5);
        TEST_ASSERT_TRUE_MSG(r >= -5 && r <= 5, "rng in [-5,5]");
    }
}

void test_rng_range_full_int(void) {
    uint32_t s = 0; snake_rng_seed(&s, 54321);
    for(int i=0;i<1000;i++) {
        int r = snake_rng_range(&s, INT32_MIN, INT32_MAX);
        TEST_ASSERT_TRUE_MSG(r >= INT32_MIN && r <= INT32_MAX, "rng in full int range");
    }
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_rng_repeatability);
    RUN_TEST(test_rng_range_small);
    RUN_TEST(test_rng_range_full_int);
    return UnityEnd();
}
