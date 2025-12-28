#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*UnityTestFunction)(void);

void UnityBegin(void);
int UnityEnd(void);
void run_single_test(const char* name, UnityTestFunction func, const char* file, int line);

#define RUN_TEST(fn) run_single_test(#fn, fn, __FILE__, __LINE__)

/* Assertions */
#define TEST_ASSERT_TRUE(expr) do { if (!(expr)) { fprintf(stderr, "FAIL: %s:%d: %s is false\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)
#define TEST_ASSERT_FALSE(expr) do { if (expr) { fprintf(stderr, "FAIL: %s:%d: %s is true\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)
#define TEST_ASSERT_EQUAL_INT(expected, actual) do { if ((expected) != (actual)) { fprintf(stderr, "FAIL: %s:%d: expected %d but was %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); exit(1); } } while (0)
#define TEST_ASSERT_EQUAL_STRING(expected, actual) do { if (strcmp((expected), (actual)) != 0) { fprintf(stderr, "FAIL: %s:%d: expected \"%s\" but was \"%s\"\n", __FILE__, __LINE__, (expected), (actual)); exit(1); } } while (0)
#define TEST_FAIL_MESSAGE(msg) do { fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg)); exit(1); } while (0)

/* Hooks */
void setUp(void);
void tearDown(void);

#ifndef UNITY_NO_EXTRAS
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#define TEST(name) void name(void)
#endif
