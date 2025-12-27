#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple allocation-failure simulator: override malloc/calloc/realloc using dlsym(RTLD_NEXT).
   Set `fail_after` to X to cause allocations 1..X to succeed and allocation X+1 to fail.
*/
static size_t alloc_count = 0;
static long fail_after = -1; /* -1 means never fail */

typedef void* (*malloc_fn)(size_t);
typedef void* (*calloc_fn)(size_t, size_t);
typedef void* (*realloc_fn)(void*, size_t);
typedef void (*free_fn)(void*);
static malloc_fn real_malloc = NULL;
static calloc_fn real_calloc = NULL;
static realloc_fn real_realloc = NULL;
static free_fn real_free = NULL;

static void ensure_real_fns(void) {
    if (!real_malloc) real_malloc = (malloc_fn)dlsym(RTLD_NEXT, "malloc");
    if (!real_calloc) real_calloc = (calloc_fn)dlsym(RTLD_NEXT, "calloc");
    if (!real_realloc) real_realloc = (realloc_fn)dlsym(RTLD_NEXT, "realloc");
    if (!real_free) real_free = (free_fn)dlsym(RTLD_NEXT, "free");
}

void* malloc(size_t s) {
    ensure_real_fns();
    alloc_count++;
    if (fail_after >= 0 && (long)alloc_count > fail_after)
        return NULL;
    return real_malloc(s);
}

void* calloc(size_t n, size_t s) {
    ensure_real_fns();
    alloc_count++;
    if (fail_after >= 0 && (long)alloc_count > fail_after)
        return NULL;
    return real_calloc(n, s);
}

void* realloc(void* p, size_t s) {
    ensure_real_fns();
    alloc_count++;
    if (fail_after >= 0 && (long)alloc_count > fail_after)
        return NULL;
    return real_realloc(p, s);
}

void free(void* p) {
    ensure_real_fns();
    real_free(p);
}

/* Minimal test harness */
#include "snake/persist.h"
#include "snake/game.h"

int main(void) {
    /* Test 1: first allocation fails in game_create */
    alloc_count = 0; fail_after = 0;
    Game* g = game_create(NULL, 0); /* invalid input -> expected NULL even if malloc not invoked */
    if (g != NULL) {
        printf("FAIL: expected NULL when passing NULL cfg to game_create\n");
        return 1;
    }

    /* Test 2: simulate OOM during game_create allocations */
    alloc_count = 0; fail_after = -1; /* allow config allocation */
    /* prepare a minimal config to pass in */
    GameConfig* cfg = game_config_create();
    if (!cfg) {
        printf("SKIP: could not allocate GameConfig; environment issue\n");
        return 77;
    }
    /* ensure at least one allocation in game_create will fail */
    fail_after = 0; /* force first allocation in create to fail */
    Game* g2 = game_create(cfg, 0);
    if (g2 != NULL) {
        printf("FAIL: expected game_create to return NULL when first allocation fails\n");
        game_destroy(g2);
        game_config_destroy(cfg);
        return 1;
    }

    /* Test 3: allow several allocations then fail later to exercise cleanup paths */
    alloc_count = 0; fail_after = 4; /* let first 4 allocations succeed, fail the 5th */
    Game* g3 = game_create(cfg, 0);
    if (g3 != NULL) {
        /* depending on internal allocation ordering, it may still succeed; if it does, destroy it */
        game_destroy(g3);
    } else {
        /* OK: returned NULL due to OOM where cleanup code should have freed partial resources */
    }

    /* Test 4: highscore_create should return NULL on allocation failure */
    alloc_count = 0; fail_after = 0; /* make first allocation fail */
    HighScore* hs = highscore_create("x", 1);
    if (hs != NULL) {
        printf("FAIL: expected highscore_create to return NULL on OOM\n");
        highscore_destroy(hs);
        game_config_destroy(cfg);
        return 1;
    }

    game_config_destroy(cfg);
    printf("OK\n");
    return 0;
}
