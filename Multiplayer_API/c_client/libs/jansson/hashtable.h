
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdlib.h>
#include "jansson.h"

struct hashtable_list {
    struct hashtable_list *prev;
    struct hashtable_list *next;
};

struct hashtable_pair {
    struct hashtable_list list;
    struct hashtable_list ordered_list;
    size_t hash;
    json_t *value;
    char key[1];
};

struct hashtable_bucket {
    struct hashtable_list *first;
    struct hashtable_list *last;
};

typedef struct hashtable {
    size_t size;
    struct hashtable_bucket *buckets;
    size_t order;
    struct hashtable_list list;
    struct hashtable_list ordered_list;
} hashtable_t;

#define hashtable_key_to_iter(key_) \
    (&(container_of(key_, struct hashtable_pair, key)->ordered_list))

int hashtable_init(hashtable_t *hashtable) JSON_ATTRS(warn_unused_result);

void hashtable_close(hashtable_t *hashtable);

int hashtable_set(hashtable_t *hashtable, const char *key, json_t *value);

void *hashtable_get(hashtable_t *hashtable, const char *key);

int hashtable_del(hashtable_t *hashtable, const char *key);

void hashtable_clear(hashtable_t *hashtable);

void *hashtable_iter(hashtable_t *hashtable);

void *hashtable_iter_at(hashtable_t *hashtable, const char *key);

void *hashtable_iter_next(hashtable_t *hashtable, void *iter);

void *hashtable_iter_key(void *iter);

void *hashtable_iter_value(void *iter);

void hashtable_iter_set(void *iter, json_t *value);

#endif
