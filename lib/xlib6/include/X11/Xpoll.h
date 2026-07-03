/*
 * Xpoll.h -- psx replacement for the X11R6.9 <X11/Xpoll.h>.
 *
 * The ReactOS POSIX subsystem has no select(); the /dev/x11 connection is a
 * blocking byte stream polled via /dev/xpoll.  Xlib's simple wait loop is
 * tolerant of a fake select, but Xt (FindInputs in NextEvent.c) walks the
 * returned fd_set bit by bit and indexes app->input_list[fd] for every set
 * bit -- so the fd_set MUST be a real bitmask and Select() MUST return an
 * honest set (only fds that are genuinely ready).  A constant-true FD_ISSET
 * makes Xt process input on fd 0 and dereference garbage.
 *
 * MIT/X license (see copyright.h).
 */
#ifndef _XPOLL_H_
#define _XPOLL_H_

#include <sys/time.h>		/* struct timeval */
#include <X11/Xtrans/Xtrans.h>	/* _XPsxSelect */

#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif
#define XFD_SETSIZE FD_SETSIZE

#define __PSX_NFDBITS	(8 * (int)sizeof(long))
#define __PSX_FDELT(n)	((n) / __PSX_NFDBITS)
#define __PSX_FDMASK(n)	((long)1 << ((n) % __PSX_NFDBITS))

typedef struct fd_set {
    long fds_bits[FD_SETSIZE / __PSX_NFDBITS];
} fd_set;

#define FD_SET(n, p)	((p)->fds_bits[__PSX_FDELT(n)] |= __PSX_FDMASK(n))
#define FD_CLR(n, p)	((p)->fds_bits[__PSX_FDELT(n)] &= ~__PSX_FDMASK(n))
#define FD_ISSET(n, p)	(((p)->fds_bits[__PSX_FDELT(n)] & __PSX_FDMASK(n)) != 0)
#define FD_ZERO(p)	memset((char *)(p), 0, sizeof(*(p)))

/* Xt/Xlib bulk-set helpers (bitwise over the whole mask array) */
#define XFD_ANYSET(p)		__psx_fd_anyset(p)
#define XFD_COPYSET(src, dst)	(*(dst) = *(src))
#define XFD_ANDSET(dst, b1, b2)	__psx_fd_op(dst, b1, b2, 0)
#define XFD_ORSET(dst, b1, b2)	__psx_fd_op(dst, b1, b2, 1)
#define XFD_UNSET(dst, b1)	__psx_fd_unset(dst, b1)

static __inline int __psx_fd_anyset(fd_set *p)
{
    unsigned i;
    for (i = 0; i < FD_SETSIZE / __PSX_NFDBITS; i++)
	if (p->fds_bits[i]) return 1;
    return 0;
}
static __inline void __psx_fd_op(fd_set *d, fd_set *a, fd_set *b, int orop)
{
    unsigned i;
    for (i = 0; i < FD_SETSIZE / __PSX_NFDBITS; i++)
	d->fds_bits[i] = orop ? (a->fds_bits[i] | b->fds_bits[i])
			      : (a->fds_bits[i] & b->fds_bits[i]);
}
static __inline void __psx_fd_unset(fd_set *d, fd_set *b)
{
    unsigned i;
    for (i = 0; i < FD_SETSIZE / __PSX_NFDBITS; i++)
	d->fds_bits[i] &= ~b->fds_bits[i];
}

#define Select(n, r, w, e, t) \
	_XPsxSelect((n), (void *)(r), (void *)(w), (void *)(e), (t))

#endif /* _XPOLL_H_ */
