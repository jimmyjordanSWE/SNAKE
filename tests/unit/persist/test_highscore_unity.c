#include "snake/persist.h"
#include "unity.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static void test_highscore_basic(void);
static void test_highscore_name_truncate(void);


void test_highscore_basic(void) {
    HighScore* hs = highscore_create("alice", 42);
    TEST_ASSERT_EQUAL_STRING("alice", highscore_get_name(hs));
    TEST_ASSERT_EQUAL_INT(42, highscore_get_score(hs));
    highscore_set_name(hs, "bob");
    highscore_set_score(hs, 7);
    TEST_ASSERT_EQUAL_STRING("bob", highscore_get_name(hs));
    TEST_ASSERT_EQUAL_INT(7, highscore_get_score(hs));
    highscore_destroy(hs);
}

void test_highscore_name_truncate(void) {
    char longname[256];
    for(int i=0;i<250;i++) longname[i] = 'x';
    longname[250] = '\0';
    HighScore* hs = highscore_create(longname, 1);
    const char* name = highscore_get_name(hs);
    
    TEST_ASSERT_TRUE_MSG(name != NULL, "name should not be NULL");
    TEST_ASSERT_TRUE_MSG((int)strlen(name) < PERSIST_NAME_MAX, "name should be truncated to PERSIST_NAME_MAX-1");
    highscore_destroy(hs);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_highscore_basic);
    RUN_TEST(test_highscore_name_truncate);
    return UnityEnd();
}
