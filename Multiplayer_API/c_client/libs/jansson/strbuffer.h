
#ifndef STRBUFFER_H
#define STRBUFFER_H

#include <stdlib.h>

typedef struct {
    char *value;
    size_t length;
    size_t size;
} strbuffer_t;

int strbuffer_init(strbuffer_t *strbuff) JSON_ATTRS(warn_unused_result);
void strbuffer_close(strbuffer_t *strbuff);

void strbuffer_clear(strbuffer_t *strbuff);

const char *strbuffer_value(const strbuffer_t *strbuff);

char *strbuffer_steal_value(strbuffer_t *strbuff);

int strbuffer_append_byte(strbuffer_t *strbuff, char byte);
int strbuffer_append_bytes(strbuffer_t *strbuff, const char *data, size_t size);

char strbuffer_pop(strbuffer_t *strbuff);

#endif
