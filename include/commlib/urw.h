/*************************************************
 * Anthor  : LuoZhongYao@gmail.com
 * Modified: 2017 Feb 24
 ************************************************/
#ifndef __URW_H__
#define __URW_H__
#include <sys/stat.h> // mode_t
#include <sys/poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define URW_RDONLY      0x0001
#define URW_WRONLY      0x0002
#define URW_RDWR        0x0003
#define URW_HUP         POLLHUP
#define URW_ERR         POLLERR
#define URW_NVAL        POLLNVAL

struct urw;
struct urwcb;
struct usock;

void urw_loop(struct urw*) __attribute__((nonnull));
struct urw *urw_alloc(const char *name, int policy, int priority);
struct urw *urw_create(const char *name, int policy, int priority);
struct urwcb *urw_add(struct urw *urw, int fd, short events, void (*cbk)(int fd, short events, void *ctx), void *ctx) __attribute__((nonnull(1,4)));
void urw_verify(struct urw *urw, uint16_t [3]) __attribute__((nonnull));


void urw_del(struct urwcb **cb) __attribute__((nonnull));
void urw_resume(struct urwcb *cb) __attribute__((nonnull));
void urw_suspend(struct urwcb *cb) __attribute__((nonnull));
bool urw_is_suspend(struct urwcb *cb) __attribute__((nonnull));
struct urw *urwcb_get(struct urwcb *cb) __attribute__((nonnull));

struct usock *urw_listen(struct urw *urw,
                         int fd,
                         short events,
                         void *(*connect)(int fd, struct urwcb *cb, void *ctx),
                         void (*process) (int fd, short events, void *ctx),
                         void *ctx) __attribute__((nonnull(1, 5)));
struct usock *urw_sock(struct urw *urw,
        const char *name,
        int type,
        mode_t perm,
        short events,
        void *(*connect)(int fd, struct urwcb *cb, void *ctx),    /* Generate the context of the process */
        void (*process) (int fd, short events, void *ctx),  /* The context comes from connect */ 
        void *ctx) __attribute__((nonnull(1, 7)));
struct urwcb *urw_usock_get_urwcb(struct usock *usock);
struct usock *urw_urwcb_get_usock(struct urwcb *cb);
void urw_rmsock(struct usock *usock) __attribute__((nonnull));
int urw_mknode(const char *name, int type, mode_t perm);
void urw_rmnode(int fd, int type, const char *name);
int urw_open(const char *name, int type);

void io_loop_event(void);
void io_del_watch(struct urwcb **cb);
struct urwcb *io_add_watch(int fd, short event, void (*handler)(int fd, short event, void *ctx), void *ctx);

#endif

