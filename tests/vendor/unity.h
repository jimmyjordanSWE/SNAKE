#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct { int tests; int failed; const char* current; } UnityState;
extern UnityState Unity;

static inline void UnityBegin(void) { Unity.tests = 0; Unity.failed = 0; Unity.current = NULL; }
static inline int UnityEnd(void) { if(Unity.failed) { fprintf(stderr, "%d test(s) failed out of %d\n", Unity.failed, Unity.tests); return 1; } fprintf(stdout, "%d test(s) OK\n", Unity.tests); return 0; }

#define RUN_TEST(func) do { Unity.tests++; Unity.current = #func; fprintf(stdout, "RUN: %s\n", #func); func(); fprintf(stdout, "PASS: %s\n", #func); } while(0)

#define TEST_ASSERT_TRUE(msg, expr) do { if(!(expr)) { Unity.failed++; fprintf(stderr, "FAIL %s: %s\n", Unity.current, (msg)); return; } } while(0)
#define TEST_ASSERT_EQUAL_INT(expected, actual) do { if((expected) != (actual)) { Unity.failed++; fprintf(stderr, "FAIL %s: expected %d got %d\n", Unity.current, (expected), (actual)); return; } } while(0)
#define TEST_ASSERT_EQUAL_STRING(expected, actual) do { const char* _e = (expected); const char* _a = (actual); if((_e == NULL && _a != NULL) || (_e != NULL && _a == NULL) || (_e && _a && strcmp(_e,_a) != 0)) { Unity.failed++; fprintf(stderr, "FAIL %s: expected '%s' got '%s'\n", Unity.current, (_e?_e:"(null)"), (_a?_a:"(null)")); return; } } while(0)
#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) do { if(fabs((expected)-(actual)) > (delta)) { Unity.failed++; fprintf(stderr, "FAIL %s: expected %f +/- %f got %f\n", Unity.current, (expected), (delta), (actual)); return; } } while(0)
#define TEST_ASSERT_TRUE_MSG(expr, msg) TEST_ASSERT_TRUE((msg), (expr))

