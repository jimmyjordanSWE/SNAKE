#include "snake/utils.h"
#include "unity.h"

static void test_in_bounds_basic(void);

void test_in_bounds_basic(void) {
    TEST_ASSERT_TRUE_MSG(snake_in_bounds(0,0,10,10), "0,0 in bounds");
    TEST_ASSERT_TRUE_MSG(snake_in_bounds(9,9,10,10), "9,9 in bounds");
    TEST_ASSERT_TRUE_MSG(!snake_in_bounds(-1,0,10,10), "-1,0 out of bounds");
    TEST_ASSERT_TRUE_MSG(!snake_in_bounds(10,5,10,10), "10,5 out of bounds");
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_in_bounds_basic);
    return UnityEnd();
}
