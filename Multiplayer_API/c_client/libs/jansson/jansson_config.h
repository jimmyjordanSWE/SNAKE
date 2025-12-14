
#ifndef JSON_CONFIG_H
#define JSON_CONFIG_H

#define HAVE_STDINT_H

#define JSON_USE_TAB_INDENT

#ifdef __cplusplus
#define JSON_INLINE inline
#else
#define JSON_INLINE static inline
#endif

   #define JSON_INTEGER_IS_LONG_LONG 1



#define JSON_HAVE_LOCALECONV 0

#define JSON_HAVE_ATOMIC_BUILTINS 0

#define JSON_HAVE_SYNC_BUILTINS 0

#define JSON_PARSER_MAX_DEPTH 2048

#endif
