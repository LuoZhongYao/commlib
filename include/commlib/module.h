/*************************************************
 * Anthor  : LuoZhongYao@gmail.com
 * Modified: 2017/08/15
 ************************************************/
#ifndef __MODULE_BASE_H__
#define __MODULE_BASE_H__

#include <commlib/defs.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* built-in module */
#define BUILTIN_MOD_SECTION(sec)    __attribute__((used, section(sec)))
#define BUILTIN_MOD_ALIGNED         __attribute__((aligned(8)))

struct BUILTIN_MOD_ALIGNED __builtin_mod_table
{
    const char *mod_name;
    void *(*mod_import)(void);
} ;

#define _BUILTIN_MODNAME(n) __BUILTIN_NAME_ ## n
#define BUILTIN_MODNAME(n) _BUILTIN_MODNAME(n)
#define BUILTIN_MOD_DEF(T, name, import) \
    static const struct __builtin_mod_table BUILTIN_MODNAME(__LINE__) \
    BUILTIN_MOD_ALIGNED BUILTIN_MOD_SECTION(#T) = { .mod_name = name, .mod_import = (void *(*)(void))import}

#define BUILTIN_MOD_FOREACH(T, pos, id)  \
    extern struct __builtin_mod_table __start_##T;\
    extern struct __builtin_mod_table __stop_##T;\
    for(id = 0, pos = &__start_##T;pos < &__stop_##T;pos++, id++)

#define __BUILTIN_MOD_NAME(T, name) ({\
        struct T *m = NULL;\
        struct __builtin_mod_table *item = &__start_##T;\
        for(;item < &__stop_##T;item++) { \
            if(item->mod_name && strcmp(item->mod_name, (char*)(uintptr_t)name) == 0) {\
                m = item->mod_import();\
            } \
        } m;})

#define __BUILTIN_MOD_ID(T, id) ({ \
        struct __builtin_mod_table *item = &__start_##T;\
        (item + (uintptr_t)id < &__stop_##T) ? (item + (uintptr_t)id)->mod_import() : NULL;\
        })

#define BUILTIN_MOD_IMPORT(T, id) ({\
        extern struct __builtin_mod_table __start_##T;\
        extern struct __builtin_mod_table __stop_##T;\
        __builtin_choose_expr(__builtin_types_compatible_p(__typeof__(id), char *), \
                __BUILTIN_MOD_NAME(T, id),\
                __builtin_choose_expr(__builtin_types_compatible_p(__typeof__(id), char []), \
                    __BUILTIN_MOD_NAME(T, id),\
                    __builtin_choose_expr(__builtin_types_compatible_p(__typeof__(id), int), \
                        __BUILTIN_MOD_ID(T, id), \
                        NULL)));})


#define __builtin_init  BUILTIN_MOD_SECTION("builtin_init")
extern  unsigned char __start_builtin_init;
extern  unsigned char __stop_builtin_init;

static inline void builtin_initcall(void)
{
    void (*fn)(void) = (void(*)(void))&__start_builtin_init;
    for(;fn < (void(*)(void))&__stop_builtin_init;fn++)
        fn();
}


/* external module */
struct modbase
{
    unsigned char subclass[0];
};

struct modtable
{
    struct modbase *(*mt_constr)(void);
};

#define MOD_MMSZ(subclass) \
    (sizeof(subclass) + sizeof(struct modbase))

#define MOD_ENTRY(ptr) \
    (struct modbase*)((char *)ptr - offsetof(struct modbase, subclass))

#define MOD_DEF(name, constr, type) \
    const struct modtable name##_##type = {constr}

const char *mod_strerror(void);
struct modbase *mod_import(const char *mod_dir,
        const char *mod_name,
        const char *mod_type);

#define MOD_IMPORT(dir, mod, type) ({\
        struct modbase *_m = mod_import(dir, mod, type); \
        _m ? _m->subclass : NULL;})


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MODULE_BASE_H__*/

