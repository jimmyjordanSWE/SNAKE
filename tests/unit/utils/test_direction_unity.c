#include "snake/direction.h"
#include "unity.h"

static void test_turn_left_and_right(void);

void test_turn_left_and_right(void) {
    TEST_ASSERT_EQUAL_INT(SNAKE_DIR_LEFT, snake_dir_turn_left(SNAKE_DIR_UP));
    TEST_ASSERT_EQUAL_INT(SNAKE_DIR_RIGHT, snake_dir_turn_left(SNAKE_DIR_DOWN));
    TEST_ASSERT_EQUAL_INT(SNAKE_DIR_DOWN, snake_dir_turn_left(SNAKE_DIR_LEFT));
    TEST_ASSERT_EQUAL_INT(SNAKE_DIR_UP, snake_dir_turn_left(SNAKE_DIR_RIGHT));

    TEST_ASSERT_EQUAL_INT(SNAKE_DIR_RIGHT, snake_dir_turn_right(SNAKE_DIR_UP));
    TEST_ASSERT_EQUAL_INT(SNAKE_DIR_LEFT, snake_dir_turn_right(SNAKE_DIR_DOWN));
    TEST_ASSERT_EQUAL_INT(SNAKE_DIR_UP, snake_dir_turn_right(SNAKE_DIR_LEFT));
    TEST_ASSERT_EQUAL_INT(SNAKE_DIR_DOWN, snake_dir_turn_right(SNAKE_DIR_RIGHT));
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_turn_left_and_right);
    return UnityEnd();
}
