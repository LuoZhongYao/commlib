/*************************************************
 * Anthor  : LuoZhongYao@gmail.com
 * Modified: 2016 Sep 24
 ************************************************/
#ifndef __ULIB_DEFS_H__
#define __ULIB_DEFS_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __packed
#define __packed  __attribute__((__packed__))
#endif

#ifndef __unused
#define __unused  __attribute__((unused))
#endif

#ifndef __printf
#define __printf(v1, v2) __attribute__((format(printf, v1, v2)))
#endif

#ifndef __used
#define __used  __attribute__((used))
#endif

#define __ffs(n) __builtin_ffs(n)
#define __popcount(n) __builtin_popcount(n)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
# define __same_type(a, b) __builtin_types_compatible_p(__typeof__(a), __typeof__(b))
#define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))

#ifdef __cplusplus
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#else
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
#endif

#define container_of(ptr, type, member) ({                      \
        const __typeof__( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x) \
({  type __dummy; \
    typeof(x) __dummy2; \
    (void)(&__dummy == &__dummy2); \
    1; \
})
 
/*
 * Check at compile time that 'function' is a certain type, or is a pointer
 * to that type (needs to use typedef for the function type.)
 */
#define typecheck_fn(type,function) \
({  typeof(type) __tmp = function; \
    (void)__tmp; \
})


#ifdef __cplusplus
}
#endif

#endif

