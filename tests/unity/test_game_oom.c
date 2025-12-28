#define _GNU_SOURCE
#include "unity.h"
#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* allocation simulator */
static size_t alloc_count = 0;
static long fail_after = -1;

typedef void* (*malloc_fn)(size_t);
typedef void* (*calloc_fn)(size_t, size_t);
typedef void* (*realloc_fn)(void*, size_t);
typedef void (*free_fn)(void*);
static malloc_fn real_malloc = NULL;
static calloc_fn real_calloc = NULL;
static realloc_fn real_realloc = NULL;
static free_fn real_free = NULL;

static void ensure_real_fns(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    if (!real_malloc) real_malloc = (malloc_fn)dlsym(RTLD_NEXT, "malloc");
    if (!real_calloc) real_calloc = (calloc_fn)dlsym(RTLD_NEXT, "calloc");
    if (!real_realloc) real_realloc = (realloc_fn)dlsym(RTLD_NEXT, "realloc");
    if (!real_free) real_free = (free_fn)dlsym(RTLD_NEXT, "free");
#pragma GCC diagnostic pop
}

void* malloc(size_t s) {
    ensure_real_fns();
    alloc_count++;
    if (fail_after >= 0 && (long)alloc_count > fail_after) return NULL;
    return real_malloc(s);
}
void* calloc(size_t n, size_t s) { ensure_real_fns(); alloc_count++; if (fail_after >= 0 && (long)alloc_count > fail_after) return NULL; return real_calloc(n, s); }
void* realloc(void* p, size_t s) { ensure_real_fns(); alloc_count++; if (fail_after >= 0 && (long)alloc_count > fail_after) return NULL; return real_realloc(p, s); }
void free(void* p) { ensure_real_fns(); real_free(p); }

#include "game.h"
#include "persist.h"

TEST(test_game_oom) {
    alloc_count = 0; fail_after = 0;
    Game* g = game_create(NULL, 0);
    TEST_ASSERT_TRUE(g == NULL);

    alloc_count = 0; fail_after = -1;
    GameConfig* cfg = game_config_create();
    if (!cfg) TEST_FAIL_MESSAGE("SKIP: could not allocate GameConfig; environment issue");

    fail_after = 0;
    Game* g2 = game_create(cfg, 0);
    TEST_ASSERT_TRUE(g2 == NULL);

    alloc_count = 0; fail_after = 4;
    Game* g3 = game_create(cfg, 0);
    if (g3) game_destroy(g3);

    alloc_count = 0; fail_after = 0;
    HighScore* hs = highscore_create("x", 1);
    TEST_ASSERT_TRUE(hs == NULL);

    game_config_destroy(cfg);
    /* Restore normal allocation behavior for subsequent tests */
    fail_after = -1;
    alloc_count = 0;
}

