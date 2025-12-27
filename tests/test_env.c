#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "env.h"

int main(void) {
    setenv("TEST_ENV_BOOL_TRUE", "1", 1);
    setenv("TEST_ENV_BOOL_YES", "yes", 1);
    setenv("TEST_ENV_BOOL_FALSE", "0", 1);
    setenv("TEST_ENV_BOOL_NO", "no", 1);
    setenv("TEST_ENV_BOOL_LONG", "thisisaverylongvaluethatexceedssomesaneamount", 1);

    assert(env_bool("TEST_ENV_BOOL_TRUE", 0) == 1);
    assert(env_bool("TEST_ENV_BOOL_YES", 0) == 1);
    assert(env_bool("TEST_ENV_BOOL_FALSE", 1) == 0);
    assert(env_bool("TEST_ENV_BOOL_NO", 1) == 0);
    /* long values should be handled safely and not crash; they may default to 0 */
    (void)env_bool("TEST_ENV_BOOL_LONG", 0);

    printf("test_env: OK\n");
    return 0;
}
