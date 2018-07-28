/*************************************************
 * Anthor  : LuoZhongYao@gmail.com
 * Modified: 2016 Sep 13
 ************************************************/
#ifndef __LOG_H__
#define __LOG_H__

#ifndef LOG_TAG
#define LOG_TAG ""
#endif

#define _DEBUG

#if defined(__ANDROID__)

#include <android/log.h>
#define BLOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define BLOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define BLOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define BLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define BLOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#else
#ifdef __linux__
#include <execinfo.h>
#define FNAME(addr) do { \
    const char **strings;\
    void *buf[1] = {(void*)addr};\
    if((strings = backtrace_symbols(buf, 1))) {\
        BLOGD("# %s", strings[0]);\
        free(strings);\
    }} while(0)
#endif

#include <stdio.h>
#define __log_print(fmt, ...) do { fprintf(stdout, fmt, ##__VA_ARGS__); fflush(stdout); } while(0)
#define __err_print(fmt, ...) do { fprintf(stderr, fmt, ##__VA_ARGS__); fflush(stderr); } while(0)
#define BLOGV(fmt,...) __log_print("V/%-9s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define BLOGI(fmt,...) __log_print("I/%-9s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define BLOGD(fmt,...) __log_print("D/%-9s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define BLOGE(fmt,...) __err_print("E/%-9s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define BLOGW(fmt,...) __err_print("W/%-9s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)

#endif

#define BLOGX(p, size) do {\
    int len = 0;\
    char buf[82];\
    unsigned char *tmp = (unsigned char*)p;\
    for(int i = 0, c = (size);i < c;i++) {\
        if(i && !(i % 16)) {\
            BLOGD("%s", buf); \
            len = 0; \
        }\
        len += sprintf(buf + len, "0x%02x ", tmp[i]); \
    }\
    if(len) {\
        BLOGD("%s", buf); \
    } } while(0)

#define BIMPL(fmt, ...) BLOGW(fmt " %s Not Impl %s:%d", ##__VA_ARGS__, __func__, __FILE__, __LINE__)

#define BD_FMT "%04x%02x%06x"
#define BD_STR(bd) (bd)->nap, (bd)->uap, (bd)->lap
#define BD_COLON_FMT "%02X:%02X:%02X:%02X:%02X:%02X"
#define BD_COLON_STR(bd) ((bd)->nap >> 8) & 0xff, (bd)->nap & 0xff, (bd)->uap, (((bd)->lap) >> 16) & 0xff, (((bd)->lap) >> 8) & 0xff, (bd)->lap & 0xff
#define enforce_process(f) ({BLOGI("%s:%d ExitProcess", __func__, __LINE__); exit(f);})

#ifndef FNAME
#define FNAME(addr)
#endif

#endif

