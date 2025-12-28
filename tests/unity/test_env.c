#define _POSIX_C_SOURCE 200809L
#include "unity.h"
#include <stdlib.h>
#include "env.h"

TEST(test_env) {
    setenv("TEST_ENV_BOOL_TRUE", "1", 1);
    setenv("TEST_ENV_BOOL_YES", "yes", 1);
    setenv("TEST_ENV_BOOL_FALSE", "0", 1);
    setenv("TEST_ENV_BOOL_NO", "no", 1);
    setenv("TEST_ENV_BOOL_LONG", "thisisaverylongvaluethatexceedssomesaneamount", 1);

    TEST_ASSERT_EQUAL_INT(1, env_bool("TEST_ENV_BOOL_TRUE", 0));
    TEST_ASSERT_EQUAL_INT(1, env_bool("TEST_ENV_BOOL_YES", 0));
    TEST_ASSERT_EQUAL_INT(0, env_bool("TEST_ENV_BOOL_FALSE", 1));
    TEST_ASSERT_EQUAL_INT(0, env_bool("TEST_ENV_BOOL_NO", 1));
    /* long values should be handled safely and not crash */
    (void)env_bool("TEST_ENV_BOOL_LONG", 0);
}


