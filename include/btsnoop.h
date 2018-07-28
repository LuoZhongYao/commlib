/*************************************************
 * Anthor  : LuoZhongYao@gmail.com
 * Modified: 2016 Apr 20
 ************************************************/
#ifndef __BTSNOOP_H__
#define __BTSNOOP_H__

#include <stdbool.h>
#include <sys/uio.h>

struct btsnoop;
struct btsnoop *btsnoop_open(const char *p_path);
void btsnoop_close(struct btsnoop* btsnoop);
void btsnoop_capture(struct btsnoop *btsnoop, int channel, const void *p_buf, bool is_recv);
void btsnoop_capturev(struct btsnoop *btsnoop, const struct iovec iov[], size_t iovn, bool is_received);

#endif

