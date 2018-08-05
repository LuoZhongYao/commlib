#define LOG_TAG "ulib"

#if defined(__QNX__)

# include <unix.h>

#else /* __QNX__ */

#if !defined(_XOPEN_SOURCE)
# define _XOPEN_SOURCE
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif

#endif /* __QNX__ */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include <list.h>
#include <commlib/defs.h>
#include <commlib/Log.h>
#include <commlib/urw.h>

#define URW_MASK        0xFFFF0000
#define URW_WAIT        0x00010000
#define URW_BUSY        0x00020000
static struct urw *urw = NULL;

extern char *ptsname(int fd);

struct urw_ccb
{
    void *context;
    void (*cbk)(int fd, short events, void *ctx);
};

struct urwcb
{
    int fd;
    int events;
    void *context;
    void (*cbk)(int fd, short events, void *ctx);
    struct urw *urw;
    struct list_head urwcb_list;
};

struct usock
{
    int events;
    void *context;
    void *(*connect)(int fd, struct urwcb *cb, void *ctx);
    void (*process)(int fd, short events, void *ctx);
    struct urwcb cb;
};

struct urw
{
    int wfd[2];
    fd_set rfds;
    fd_set wfds;
    int policy;
    int priority;
    const char *name;
    pthread_t pid;
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexattr;
    struct list_head urw_list;
};

static bool FD_ISHUP(int fd)
{
    struct pollfd fds;
    fds.fd = fd;
    fds.events = 0;
    fds.revents = 0;

    while ((-1 == poll(&fds, 1, 0)) && (errno == EINTR));

    return fds.revents & POLLHUP;
}

void urw_loop(struct urw *urw)
{
    int ret, policy;
    char buf[10];
    struct sched_param param;
    pthread_getschedparam(urw->pid, &policy, &param);
    if(urw->priority != -1)
        param.sched_priority = urw->priority;
    if(urw->policy == -1)
        urw->policy = policy;
    if((ret = pthread_setschedparam(urw->pid, urw->policy , &param)))
        BLOGE("setschedparam: %s", strerror(ret));
    if(urw->name)
        pthread_setname_np(urw->pid, urw->name);
    while(1) {
        int maxfds = urw->wfd[0];

        FD_ZERO(&urw->rfds);
        FD_ZERO(&urw->wfds);
        FD_SET(urw->wfd[0], &urw->rfds);

        pthread_mutex_lock(&urw->mutex);
        list_for_each(pos, &urw->urw_list) {
            struct urwcb *cb = list_entry(pos, struct urwcb, urwcb_list);
            if(cb->events & URW_MASK)
                continue;
            if(cb->events & URW_RDONLY)
                FD_SET(cb->fd, &urw->rfds);
            if(cb->events & URW_WRONLY)
                FD_SET(cb->fd, &urw->wfds);
            maxfds = maxfds < cb->fd ? cb->fd : maxfds;
        }
        pthread_mutex_unlock(&urw->mutex);

        if(0 < (ret = select(maxfds + 1, &urw->rfds, &urw->wfds, NULL, NULL))) {

            if(FD_ISSET(urw->wfd[0], &urw->rfds))
                read(urw->wfd[0], buf, sizeof(buf));

            pthread_mutex_lock(&urw->mutex);
            list_for_each_safe(pos, next, &urw->urw_list) {
                short event = 0;
                struct urwcb *cb = list_entry(pos, struct urwcb, urwcb_list);
                if((cb->events & URW_HUP) && FD_ISHUP(cb->fd))
                    event |= URW_HUP;

                if(FD_ISSET(cb->fd, &urw->rfds))
                    event |= URW_RDONLY;

                if(FD_ISSET(cb->fd, &urw->wfds))
                    event |= URW_WRONLY;

                if(event & cb->events)
                    cb->cbk(cb->fd, event, cb->context);
            }
            pthread_mutex_unlock(&urw->mutex);
        } else if(ret < 0) {
            BLOGE("urw: %s", strerror(errno));
        }
    }
}

static void __urw_add(struct urw *urw, struct urwcb *cb, int fd, short events, void (*cbk)(int fd, short events, void *ctx), void *ctx)
{
    cb->urw = urw;
    cb->fd = fd;
    cb->cbk = cbk;
    cb->events = events;
    cb->context = ctx;
    INIT_LIST_HEAD(&cb->urwcb_list);

    pthread_mutex_lock(&urw->mutex);
    list_add(&cb->urwcb_list, &urw->urw_list);
    pthread_mutex_unlock(&urw->mutex);
    write(urw->wfd[1], "\0", 1);
}

struct urwcb *urw_add(struct urw *urw, int fd, short events, void (*cbk)(int fd, short events, void *ctx), void *ctx)
{
    struct urwcb *cb = malloc(sizeof(struct urwcb));
    __urw_add(urw, cb, fd, events, cbk, ctx);
    return cb;
}

static void __urw_del(struct urwcb *cb)
{
    pthread_mutex_lock(&cb->urw->mutex);
    list_del(&cb->urwcb_list);
    pthread_mutex_unlock(&cb->urw->mutex);
    write(cb->urw->wfd[1], "\0", 1);
}

void urw_del(struct urwcb **cb)
{
    __urw_del(*cb);
    free(*cb);
    *cb = NULL;
}

void urw_resume(struct urwcb *cb)
{
    /* In the callback function, can not be locked, do not need to wake up */
    pthread_mutex_lock(&cb->urw->mutex);
    cb->events &= ~URW_WAIT;
    pthread_mutex_unlock(&cb->urw->mutex);
    write(cb->urw->wfd[1], "\0", 1);
}

void urw_suspend(struct urwcb *cb)
{
    pthread_mutex_lock(&cb->urw->mutex);
    cb->events |= URW_WAIT;
    pthread_mutex_unlock(&cb->urw->mutex);
    write(cb->urw->wfd[1], "\0", 1);
}

bool urw_is_suspend(struct urwcb *cb)
{
    bool suspend;
    pthread_mutex_lock(&cb->urw->mutex);
    suspend = cb->events & URW_WAIT;
    pthread_mutex_unlock(&cb->urw->mutex);
    return suspend;
}

struct urw *urw_alloc(const char *name, int policy, int priority)
{
    struct urw *urw = (struct urw*)calloc(1, sizeof(struct urw));
    INIT_LIST_HEAD(&urw->urw_list);
    pthread_mutexattr_init(&urw->mutexattr);
    pthread_mutexattr_settype(&urw->mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&urw->mutex, &urw->mutexattr);
    if(pipe(urw->wfd) == -1) {
        free(urw);
        return NULL;
    }

    fcntl( urw->wfd[0], F_SETFL, fcntl(urw->wfd[0], F_GETFL) | O_NONBLOCK);
    fcntl( urw->wfd[1], F_SETFL, fcntl(urw->wfd[1], F_GETFL) | O_NONBLOCK);
    urw->name = name;
    urw->pid = pthread_self();
    urw->policy = policy;
    urw->priority = priority;
    return urw;
}

struct urw *urw_create(const char *name, int policy, int priority)
{
    struct urw *urw = urw_alloc(name, policy, priority);
    assert(urw);
    pthread_create(&urw->pid, NULL, (void *(*)(void*))urw_loop, (void*)urw);
    return urw;
}

struct urw *urwcb_get(struct urwcb *cb)
{
    return cb->urw;
}

static int make_socket_name(const char* name, struct sockaddr_un* addr)  
{  
    socklen_t socket_len;
    memset(addr, 0 , sizeof(*addr));
    addr->sun_family = AF_UNIX;
    if(*name == '@') {
        strncpy(addr->sun_path, name, sizeof(addr->sun_path));
        addr->sun_path[0] = '\0';
        socket_len = offsetof(struct sockaddr_un, sun_path) + strlen(addr->sun_path + 1) + 1;
    } else {
        strncpy(addr->sun_path, name, sizeof(addr->sun_path));
        socket_len = offsetof(struct sockaddr_un, sun_path) + strlen(addr->sun_path) + 1;
        unlink(addr->sun_path);
    }
    return socket_len;
}

static void urw_sock_accept(int fd, short events, void *ctx)
{
    int cfd;
    struct sockaddr_un peeraddr;
    socklen_t socklen = sizeof(peeraddr);
    struct usock *usock = (struct usock*)ctx;
    struct urwcb *cb;

    if(0 > (cfd = accept(fd, (struct sockaddr*)&peeraddr, &socklen)))
        return;
    cb = malloc(sizeof(struct urwcb));

    __urw_add(urwcb_get(&usock->cb),
              cb,
              cfd,
              usock->events,
              usock->process,
              usock->connect ? usock->connect(cfd, cb, usock->context) : NULL);
}

struct usock *urw_listen(struct urw *urw,
                         int fd,
                         short events,
                         void *(*connect)(int fd, struct urwcb *cb, void *ctx),
                         void (*process) (int fd, short events, void *ctx),
                         void *ctx)
{
    struct usock *usock  = malloc(sizeof(struct usock));
    usock->context = ctx;
    usock->events = events;
    usock->connect = connect;
    usock->process = process;
    __urw_add(urw, &usock->cb, fd, URW_RDONLY, urw_sock_accept, (void*)usock);
    return usock;
}

struct urwcb *urw_usock_get_urwcb(struct usock *usock)
{
    return &usock->cb;
}

struct usock *urw_sock(struct urw *urw,
                        const char *name,
                        int type,
                        mode_t perm,
                        short events,
                        void *(*connect)(int fd, struct urwcb *cb, void *ctx),
                        void (*process) (int fd, short events, void *ctx),
                        void *ctx)
{
    int fd;

    if(-1 == (fd = urw_mknode(name, type, perm)))
        return NULL;

    return urw_listen(urw, fd, events, connect, process, ctx);
}

void urw_rmsock(struct usock *usock)
{
    __urw_del(&usock->cb);
    free(usock);
}

int mkptms(const char* target, mode_t perm)
{
    int fdm, oflags;
    struct termios ts;

#if __QNX__
    int slave;
    char slavename[128];
    if(openpty(&fdm, &slave, slavename, NULL, NULL) < 0) {
        BLOGE("openpty %s: %s", target, strerror(errno));
        return -1;
    }
#else
    char *slavename;
    fdm = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fdm < 0) {
        BLOGE("/dev/ptmx: %s", strerror(errno));
        return -1;
    }
	grantpt(fdm); /* change permission of slave */
	unlockpt(fdm); /* unlock slave */
	slavename = ptsname(fdm); /* get name of slave */
    open(slavename, O_NONBLOCK);
#endif

    unlink(target);
    oflags = fcntl(fdm, F_GETFL);
    oflags |= (O_NONBLOCK | O_NOCTTY);
    fcntl(fdm, F_SETFL, oflags);

    if(0 == tcgetattr(fdm, &ts)) {
        cfmakeraw(&ts);
        tcsetattr (fdm, TCSANOW, &ts);
    }

    if(0 != chmod(slavename, perm)) {
        BLOGE("chmod %s: %s", slavename, strerror(errno));
    }

    if (0 != symlink(slavename, target)) {
        BLOGE("symlink %s %s: %s", slavename, target, strerror(errno));
        goto __err;
    }
    return fdm;
__err:
    close(fdm);
    return -1;
}

static int mksock(const char *name, int type, mode_t perm)
{
    int fd;
    socklen_t socklen;
    struct sockaddr_un addr;

    if(0 > (fd = socket(PF_UNIX, type, 0)))
        return -1;

    socklen = make_socket_name(name, &addr);

    if(bind(fd, (struct sockaddr *) &addr, sizeof (addr)))
        goto out_close;

    if(listen(fd, 10))
        goto out_close;

    //chown(addr.sun_path, uid, gid);
    if(*name != '@')
        chmod(addr.sun_path, perm);

    return fd;

out_close:
    close(fd);
    return -1;
}

int urw_mknode(const char *name, int type, mode_t perm)
{
    return type == -1 ? mkptms(name, perm) : mksock(name, type, perm);
}

static int open_sock(const char *name, int type)
{
    int fd;
    socklen_t socklen;
    struct sockaddr_un addr;

    if(0 > (fd = socket(PF_UNIX, type, 0)))
        return -1;

    socklen = make_socket_name(name, &addr);

    if(connect(fd, (struct sockaddr *) &addr, sizeof (addr)))
        goto out_close;

    return fd;

out_close:
    close(fd);
    return -1;
}

int urw_open(const char *name, int type)
{
    return type == -1 ? open(name, O_RDWR | O_NOCTTY) : open_sock(name, type);
}

void urw_rmnode(int fd, int type, const char *name)
{
    if(type == -1)
        unlink(name);
    close(fd);
}

void io_loop_event(void)
{
    urw_loop(urw);
}

struct urwcb *io_add_watch(int fd, short event, void (*handler)(int fd, short event, void *ctx), void *ctx)
{
    if(urw == NULL)
        urw = urw_alloc("def-loop", -1, -1);
    return urw_add(urw, fd, event, handler, ctx);
}

void io_del_watch(struct urwcb **cb)
{
    urw_del(cb);
}

