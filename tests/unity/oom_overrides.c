#define _GNU_SOURCE
#include "oom_overrides.h"
#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>

/* allocation simulator shared by multiple tests */
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

void* malloc(size_t s) { ensure_real_fns(); alloc_count++; if (fail_after >= 0 && (long)alloc_count > fail_after) return NULL; return real_malloc(s); }
void* calloc(size_t n, size_t s) { ensure_real_fns(); alloc_count++; if (fail_after >= 0 && (long)alloc_count > fail_after) return NULL; return real_calloc(n, s); }
void* realloc(void* p, size_t s) { ensure_real_fns(); alloc_count++; if (fail_after >= 0 && (long)alloc_count > fail_after) return NULL; return real_realloc(p, s); }
void free(void* p) { ensure_real_fns(); real_free(p); }

/* test helpers */
void oom_reset(void) { alloc_count = 0; fail_after = -1; }
void oom_set_fail_after(long f) { fail_after = f; }
