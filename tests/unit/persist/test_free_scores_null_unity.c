#include "snake/persist.h"
#include "unity.h"

static void test_free_scores_handles_null(void);

void test_free_scores_handles_null(void) {
    
    persist_free_scores(NULL, 0);
    
    HighScore** arr = (HighScore**)calloc(1, sizeof(HighScore*));
    persist_free_scores(arr, 0);
    TEST_ASSERT_TRUE_MSG(1, "persist_free_scores handled NULL and empty arrays without crashing");
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_free_scores_handles_null);
    return UnityEnd();
}
