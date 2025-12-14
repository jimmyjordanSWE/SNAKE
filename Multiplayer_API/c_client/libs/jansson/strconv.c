#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef __MINGW32__
#undef __NO_ISOCEXT
#endif
#include "jansson_private.h"
#include "strbuffer.h"

#include "jansson_config.h"

#ifdef __MINGW32__
#define strtod __strtod
#endif

#if JSON_HAVE_LOCALECONV
#include <locale.h>

static void to_locale(strbuffer_t *strbuffer)
{
    const char *point;
    char *pos;

    point = localeconv()->decimal_point;
    if(*point == '.') {

        return;
    }

    pos = strchr(strbuffer->value, '.');
    if(pos)
        *pos = *point;
}

static void from_locale(char *buffer)
{
    const char *point;
    char *pos;

    point = localeconv()->decimal_point;
    if(*point == '.') {

        return;
    }

    pos = strchr(buffer, *point);
    if(pos)
        *pos = '.';
}
#endif

int jsonp_strtod(strbuffer_t *strbuffer, double *out)
{
    double value;
    char *end;

#if JSON_HAVE_LOCALECONV
    to_locale(strbuffer);
#endif

    errno = 0;
    value = strtod(strbuffer->value, &end);
    assert(end == strbuffer->value + strbuffer->length);

    if((value == HUGE_VAL || value == -HUGE_VAL) && errno == ERANGE) {

        return -1;
    }

    *out = value;
    return 0;
}

int jsonp_dtostr(char *buffer, size_t size, double value, int precision)
{
    int ret;
    char *start, *end;
    size_t length;

    if (precision == 0)
        precision = 17;

    ret = snprintf(buffer, size, "%.*g", precision, value);
    if(ret < 0)
        return -1;

    length = (size_t)ret;
    if(length >= size)
        return -1;

#if JSON_HAVE_LOCALECONV
    from_locale(buffer);
#endif


    if(strchr(buffer, '.') == NULL &&
       strchr(buffer, 'e') == NULL)
    {
        if(length + 3 >= size) {

            return -1;
        }
        buffer[length] = '.';
        buffer[length + 1] = '0';
        buffer[length + 2] = '\0';
        length += 2;
    }


    start = strchr(buffer, 'e');
    if(start) {
        start++;
        end = start + 1;

        if(*start == '-')
            start++;

        while(*end == '0')
            end++;

        if(end != start) {
            memmove(start, end, length - (size_t)(end - buffer));
            length -= (size_t)(end - start);
        }
    }

    return (int)length;
}
