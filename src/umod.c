#define LOG_TAG "mod"

#include <commlib/Log.h>
#include <commlib/module.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static inline char *strcsubs(char *str, char src, char dest)
{
    while(*str) {
        if(*str == src)
            *str = dest;
        str++;
    }
    return str;
}

const char *mod_strerror(void)
{
    return dlerror();
}

struct modbase *mod_import(const char *mod_dir,
                           const char *mod_name,
                           const char *mod_type)
{
    /* gdk.module.audio */
    void *handle;
    struct modbase *mod = NULL;
    struct modtable *mt = NULL;
    char *filename;
    char *sptr;
    char *sopath = malloc(strlen(mod_dir) + strlen(mod_name) + 6);
    char sym[256];

    sptr = strrchr(mod_name, '.');
    if(sptr == NULL || *(++sptr) == 0)
        goto quit;

    snprintf(sym, sizeof(sym), "%s_%s", sptr, mod_type);

    sprintf(sopath, "%s/%s", mod_dir, mod_name);
    *strrchr(sopath, '.') = '\0';
    strcsubs(sopath, '.', '/');
    strcat(sopath, ".so");

    handle = dlopen(sopath, RTLD_LAZY);
    if(handle == NULL)
        goto quit;

    mt = dlsym(handle, sym);
    if(mt != NULL)
        mod = mt->mt_constr();
quit:
    free(sopath);
    return mod;
}
