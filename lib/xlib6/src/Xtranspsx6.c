/*
 * Xtranspsx6.c -- the /dev/x11 transport for the X11R6.9 libX11 port.
 *
 * Implements the _X11Trans* function surface libX11 calls (see our psx
 * <X11/Xtrans/Xtrans.h>) over the POSIX-subsystem X bridge: the client
 * opens /dev/x11 (psxss PSX_FILE_XCONN -> named pipe -> psxx11.exe) and
 * gets a blocking byte stream, so reads/writes block as needed and no
 * select()/non-blocking machinery exists.
 *
 * MIT/X license (see copyright.h).
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <X11/Xos.h>
#include <X11/Xtrans/Xtrans.h>
#include <X11/Xpoll.h>		/* fd_set + FD_* for _XPsxSelect */

#define X11_PSX_DEVICE  "/dev/x11"
#define X11_PSX_POLLDEV "/dev/xpoll"

struct _XtransConnInfo {
    int fd;
};

/*
 * The display connection's fd, for _XPsxSelect: our Xpoll.h fd_sets carry no
 * contents (there is no real select), so the wait path polls the display
 * connection via /dev/xpoll.  One display per process, which holds for every
 * client on the subsystem today.
 */
int _XPsxDisplayFd = -1;

/*
 * The readability-wait primitive.  A read() on /dev/xpoll makes psxss wait
 * for this process's /dev/x11 connection to become readable; the byte count
 * encodes the timeout (1 = probe, N = wait N-1 ms) and one result byte comes
 * back ('1' readable / '0' timed out).  Plain open/read is the only channel
 * the unmodified 1996 MS psxdll marshals without inspecting -- new opcodes
 * and private fcntl commands are rejected client-side.
 *
 * Returns 1 readable, 0 timed out, -1 if /dev/xpoll is unavailable (old
 * psxss): callers fall back to the blocking-stream behavior.
 */
static int psxPollFd = -2;		/* -2 not yet opened, -1 unavailable */

/*
 * Wait in slices no bigger than the buffer we actually own: the byte count
 * doubles as the timeout, and the 1996 psxdll may well validate the client
 * buffer for the full count before marshalling -- so every count passed must
 * be backed by real bytes.  <=1000ms per read, looped for longer waits.
 */
static int
psxPollDisplay (long ms)		/* ms < 0 = wait forever */
{
    static char pollbuf[1024];
    long slice, waited = 0;
    int n;

    if (psxPollFd == -2)
	psxPollFd = open (X11_PSX_POLLDEV, O_RDONLY);
    if (psxPollFd < 0)
	return -1;

    for (;;) {
	slice = (ms < 0) ? 1000 : ms - waited;
	if (slice > 1000)
	    slice = 1000;
	if (slice < 0)
	    slice = 0;

	n = read (psxPollFd, pollbuf, (unsigned int) slice + 1);
	if (n != 1) {
	    psxPollFd = -1;		/* device broken/absent: fall back */
	    return -1;
	}
	if (pollbuf[0] == '1')
	    return 1;
	waited += slice;
	if (ms >= 0 && waited >= ms)
	    return 0;
    }
}

static XtransConnInfo
psxNewConnInfo (int fd)
{
    XtransConnInfo ci = (XtransConnInfo) malloc (sizeof (struct _XtransConnInfo));

    if (ci)
	ci->fd = fd;
    return ci;
}

/*
 * Parse "[protocol/][host]:display[.screen]" the way the real Xtrans
 * ConnectDisplay does, but every display maps onto the one local bridge.
 */
XtransConnInfo
_X11TransConnectDisplay (
    char *display_name,
    char **fullnamep,		/* RETURN */
    int *dpynump,		/* RETURN */
    int *screenp,		/* RETURN */
    char **auth_namep,		/* RETURN */
    int *auth_namelenp,		/* RETURN */
    char **auth_datap,		/* RETURN */
    int *auth_datalenp)		/* RETURN */
{
    XtransConnInfo ci;
    char *lastc, *p;
    int idisplay = 0;
    int iscreen = 0;
    int fd;

    if (!display_name || !*display_name)
	display_name = ":0";

    /* the display number follows the last ':' */
    lastc = NULL;
    for (p = display_name; *p; p++)
	if (*p == ':')
	    lastc = p;
    if (!lastc)
	return NULL;

    p = lastc + 1;
    if (*p >= '0' && *p <= '9')
	idisplay = atoi (p);

    /* optional ".screen" */
    while (*p && *p != '.')
	p++;
    if (*p == '.')
	iscreen = atoi (p + 1);

    fd = open (X11_PSX_DEVICE, O_RDWR);
    if (fd < 0)
	return NULL;

    ci = psxNewConnInfo (fd);
    if (!ci) {
	close (fd);
	return NULL;
    }
    _XPsxDisplayFd = fd;

    if (fullnamep) {
	*fullnamep = (char *) malloc (strlen (display_name) + 1);
	if (*fullnamep)
	    strcpy (*fullnamep, display_name);
    }
    if (dpynump)  *dpynump = idisplay;
    if (screenp)  *screenp = iscreen;

    /* no authorization on the local bridge */
    if (auth_namep)    *auth_namep = NULL;
    if (auth_namelenp) *auth_namelenp = 0;
    if (auth_datap)    *auth_datap = NULL;
    if (auth_datalenp) *auth_datalenp = 0;

    return ci;
}

XtransConnInfo
_X11TransOpenCOTSClient (char *address)
{
    int fd = open (X11_PSX_DEVICE, O_RDWR);

    if (fd < 0)
	return NULL;
    return psxNewConnInfo (fd);
}

int
_X11TransConnect (XtransConnInfo ci, char *address)
{
    return 0;			/* open() above already connected */
}

int
_X11TransGetConnectionNumber (XtransConnInfo ci)
{
    return ci->fd;
}

int
_X11TransRead (XtransConnInfo ci, char *buf, int size)
{
    return read (ci->fd, buf, size);
}

int
_X11TransWrite (XtransConnInfo ci, char *buf, int size)
{
    return write (ci->fd, buf, size);
}

int
_X11TransReadv (XtransConnInfo ci, struct iovec *iov, int iovcnt)
{
    int i, n, total = 0;

    for (i = 0; i < iovcnt; i++) {
	if (iov[i].iov_len == 0)
	    continue;
	n = read (ci->fd, iov[i].iov_base, iov[i].iov_len);
	if (n < 0)
	    return total ? total : n;
	total += n;
	if (n < iov[i].iov_len)
	    break;		/* short read: let the caller retry */
    }
    return total;
}

int
_X11TransWritev (XtransConnInfo ci, struct iovec *iov, int iovcnt)
{
    int i, n, total = 0;

    for (i = 0; i < iovcnt; i++) {
	if (iov[i].iov_len == 0)
	    continue;
	n = write (ci->fd, iov[i].iov_base, iov[i].iov_len);
	if (n < 0)
	    return total ? total : n;
	total += n;
	if (n < iov[i].iov_len)
	    break;
    }
    return total;
}

/*
 * Readability via a /dev/xpoll probe (timeout 0).  When data waits, report a
 * full reply/event's worth (32) -- psxx11 writes whole 32-byte events, so the
 * follow-up read never blocks meaningfully.  This is what lets
 * _XEventsQueued/XPending actually pull events for Xt's event loop (with a
 * hardwired 0, Xt would never read at all).
 */
int
_X11TransBytesReadable (XtransConnInfo ci, BytesReadable_t *pend)
{
    *pend = (psxPollDisplay (0) == 1) ? 32 : 0;
    return 0;
}

int
_X11TransDisconnect (XtransConnInfo ci)
{
    return 0;
}

int
_X11TransClose (XtransConnInfo ci)
{
    int ret = close (ci->fd);

    free ((char *) ci);
    return ret == 0;
}

int
_X11TransCloseForCloning (XtransConnInfo ci)
{
    return _X11TransClose (ci);
}

void
_X11TransFreeConnInfo (XtransConnInfo ci)
{
    free ((char *) ci);
}

int
_X11TransIsLocal (XtransConnInfo ci)
{
    return 1;
}

int
_X11TransSetOption (XtransConnInfo ci, int option, int arg)
{
    return 0;			/* blocking stream; nothing to set */
}

int
_X11TransGetMyAddr (XtransConnInfo ci, int *familyp, int *addrlenp,
		    Xtransaddr **addrp)
{
    if (familyp)  *familyp = 256;	/* FamilyLocal */
    if (addrlenp) *addrlenp = 0;
    if (addrp)    *addrp = NULL;
    return 0;
}

int
_X11TransGetPeerAddr (XtransConnInfo ci, int *familyp, int *addrlenp,
		      Xtransaddr **addrp)
{
    return _X11TransGetMyAddr (ci, familyp, addrlenp, addrp);
}

int
_X11TransConvertAddress (int *familyp, int *addrlenp, Xtransaddr **addrp)
{
    return -1;
}

/*
 * The XIM transport set (remote X Input Method servers).  There is no way to
 * reach a remote IM server from inside the subsystem, so opening always
 * fails and XOpenIM falls back to the local input method; the rest exist
 * only so the statically-linked i18n modules resolve.
 */
XtransConnInfo
_XimXTransOpenCOTSClient (char *address)
{
    return NULL;
}

int
_XimXTransConnect (XtransConnInfo ci, char *address)
{
    return -1;
}

int
_XimXTransGetConnectionNumber (XtransConnInfo ci)
{
    return ci ? ci->fd : -1;
}

int
_XimXTransRead (XtransConnInfo ci, char *buf, int size)
{
    return ci ? read (ci->fd, buf, size) : -1;
}

int
_XimXTransWrite (XtransConnInfo ci, char *buf, int size)
{
    return ci ? write (ci->fd, buf, size) : -1;
}

int
_XimXTransBytesReadable (XtransConnInfo ci, BytesReadable_t *pend)
{
    *pend = 0;
    return 0;
}

int
_XimXTransDisconnect (XtransConnInfo ci)
{
    return 0;
}

int
_XimXTransClose (XtransConnInfo ci)
{
    if (ci) {
	close (ci->fd);
	free ((char *) ci);
    }
    return 1;
}

void
_XimXTransFreeConnInfo (XtransConnInfo ci)
{
    if (ci)
	free ((char *) ci);
}

int
_XimXTransIsLocal (XtransConnInfo ci)
{
    return 1;
}

/*
 * Select() behind <X11/Xpoll.h>.  We poll the one display connection via
 * /dev/xpoll and return an HONEST fd_set: readfds ends up containing only
 * the display fd (and only when it is genuinely readable), everything else
 * cleared.  This matters for Xt's FindInputs, which walks the returned mask
 * bit by bit and indexes app->input_list[fd] for each set bit -- a bogus bit
 * (e.g. a constant-true FD_ISSET reporting fd 0) makes it dereference garbage
 * and crash.  write/except fds are cleared (our writes never block, so the
 * event loop never legitimately waits on them).
 *
 *   zero timeout   -> non-blocking probe
 *   NULL timeout   -> wait forever for an X event
 *   finite timeout -> wait up to that long (Xt timers ride this)
 * Returns the number of ready fds (0 or 1).
 */
int
_XPsxSelect (int maxfds, void *readfds, void *writefds, void *exceptfds,
	     struct timeval *timeout)
{
    fd_set *rf = (fd_set *)readfds;
    fd_set *wf = (fd_set *)writefds;
    fd_set *ef = (fd_set *)exceptfds;
    int wantread;
    long ms;
    int r;

    /* clear write/except: our stream never blocks on write, never excepts */
    if (wf) FD_ZERO(wf);
    if (ef) FD_ZERO(ef);

    /* did the caller ask about the display fd for reading? */
    wantread = (rf && _XPsxDisplayFd >= 0 && FD_ISSET(_XPsxDisplayFd, rf));
    if (rf) FD_ZERO(rf);

    if (!wantread)
	return 0;					/* nothing we can serve */

    if (!timeout)
	ms = -1;					/* infinite */
    else
	ms = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;

    r = psxPollDisplay (ms);
    if (r < 0)						/* old psxss: no xpoll */
	r = (ms == 0) ? 0 : 1;				/* probe:no / wait:yes */

    if (r == 1) {
	FD_SET(_XPsxDisplayFd, rf);
	return 1;
    }
    return 0;
}
