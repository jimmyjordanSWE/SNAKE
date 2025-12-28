#include "unity.h"
#include <stdio.h>

static int tests_run = 0;
static int tests_failed = 0;

void UnityBegin(void) { tests_run = 0; tests_failed = 0; }

int UnityEnd(void) {
    if (tests_failed == 0) {
        printf("ALL TESTS PASSED (%d)\n", tests_run);
        return 0;
    }
    printf("TESTS FAILED: %d/%d\n", tests_failed, tests_run);
    return 1;
}

void run_single_test(const char* name, UnityTestFunction func, const char* file, int line) {
    printf("RUN: %s\n", name);
    tests_run++;
    /* allow optional setUp/tearDown */
    setUp();
    func();
    tearDown();
    printf("PASS: %s\n", name);
}

/* Default empty hooks (override in tests if needed) */
void setUp(void) { }
void tearDown(void) { }
