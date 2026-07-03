/*
 * Xtrans.h -- psx replacement for the X Transport Interface public header.
 *
 * The ReactOS POSIX subsystem has exactly one X transport: the /dev/x11
 * bridge fd (psxss XCONN -> named pipe -> psxx11.exe).  The real Xtrans
 * socket/TLI/DECnet machinery is not compiled; instead Xtranspsx6.c
 * implements the small function surface libX11 actually calls, declared
 * here.  XtransConnInfo stays opaque to libX11 (only the transport file
 * looks inside), matching how the real Xtrans is used.
 *
 * Derived from the X11R6.9 <X11/Xtrans/Xtrans.h> interface.
 * MIT/X license (see copyright.h).
 */
#ifndef _XTRANS_H_
#define _XTRANS_H_

#include <X11/Xfuncproto.h>

/* Transport connection object -- opaque outside Xtranspsx6.c. */
typedef struct _XtransConnInfo *XtransConnInfo;

/* Byte count for BytesReadable */
typedef long BytesReadable_t;

/* Generic transport address (only auth code inspects these; unused on psx) */
typedef struct {
    unsigned char a[128];
} Xtransaddr;

/* Options for TRANS(SetOption) -- accepted and ignored (blocking stream). */
#define TRANS_NONBLOCKING	1
#define TRANS_CLOSEONEXEC	2

/* TRANS(Connect) return codes (from the real Xtrans) */
#define TRANS_CONNECT_FAILED	-1
#define TRANS_TRY_CONNECT_AGAIN	-2
#define TRANS_IN_PROGRESS	-3

/* TRANS(Accept) status values (from the real Xtrans) */
#define TRANS_ACCEPT_BAD_MALLOC		-1
#define TRANS_ACCEPT_FAILED		-2
#define TRANS_ACCEPT_MISC_ERROR		-3

/* Scatter/gather vector.  The psx SDK has no <sys/uio.h>; libX11's
 * XlibInt.c/_XSendClientPrefix use this with TRANS(Readv/Writev). */
#ifndef _XPSX_IOVEC_DEFINED
#define _XPSX_IOVEC_DEFINED
struct iovec {
    caddr_t iov_base;
    int     iov_len;
};
#endif

_XFUNCPROTOBEGIN

/* The X11 client transport set (prefix _X11Trans, as libX11 calls them). */
extern XtransConnInfo _X11TransConnectDisplay(
    char *display_name,
    char **fullnamep,       /* RETURN */
    int *dpynump,           /* RETURN */
    int *screenp,           /* RETURN */
    char **auth_namep,      /* RETURN */
    int *auth_namelenp,     /* RETURN */
    char **auth_datap,      /* RETURN */
    int *auth_datalenp);    /* RETURN */

extern XtransConnInfo _X11TransOpenCOTSClient(char *address);
extern int  _X11TransConnect(XtransConnInfo, char *address);
extern int  _X11TransGetConnectionNumber(XtransConnInfo);
extern int  _X11TransRead(XtransConnInfo, char *buf, int size);
extern int  _X11TransWrite(XtransConnInfo, char *buf, int size);
extern int  _X11TransReadv(XtransConnInfo, struct iovec *, int iovcnt);
extern int  _X11TransWritev(XtransConnInfo, struct iovec *, int iovcnt);
extern int  _X11TransBytesReadable(XtransConnInfo, BytesReadable_t *);
extern int  _X11TransDisconnect(XtransConnInfo);
extern int  _X11TransClose(XtransConnInfo);
extern int  _X11TransCloseForCloning(XtransConnInfo);
extern void _X11TransFreeConnInfo(XtransConnInfo);
extern int  _X11TransIsLocal(XtransConnInfo);
extern int  _X11TransSetOption(XtransConnInfo, int option, int arg);
extern int  _X11TransGetMyAddr(XtransConnInfo, int *familyp, int *addrlenp,
                               Xtransaddr **addrp);
extern int  _X11TransGetPeerAddr(XtransConnInfo, int *familyp, int *addrlenp,
                                 Xtransaddr **addrp);
extern int  _X11TransConvertAddress(int *familyp, int *addrlenp,
                                    Xtransaddr **addrp);

/* The ICE transport set (libICE -> session manager).  No SM server exists
 * on the subsystem, so opening/listening fails gracefully (the same client
 * behavior as an unset SESSION_MANAGER on unix); see ice/Xtranspsxice.c. */
extern XtransConnInfo _IceTransOpenCOTSClient(char *address);
extern int  _IceTransConnect(XtransConnInfo, char *address);
extern XtransConnInfo _IceTransAccept(XtransConnInfo, int *status_ret);
extern int  _IceTransSetOption(XtransConnInfo, int option, int arg);
extern int  _IceTransClose(XtransConnInfo);
extern int  _IceTransRead(XtransConnInfo, char *buf, int size);
extern int  _IceTransWrite(XtransConnInfo, char *buf, int size);
extern int  _IceTransGetConnectionNumber(XtransConnInfo);
extern int  _IceTransIsLocal(XtransConnInfo);
extern char *_IceTransGetMyNetworkId(XtransConnInfo);
extern char *_IceTransGetPeerNetworkId(XtransConnInfo);
extern int  _IceTransMakeAllCOTSServerListeners(char *port, int *partial,
                                                int *count_ret,
                                                XtransConnInfo **ciptrs_ret);

/* The XIM transport set (remote input-method protocol).  Declared for the
 * i18n modules; on psx these connect the same /dev/x11-style way or fail. */
extern XtransConnInfo _XimXTransOpenCOTSClient(char *address);
extern int  _XimXTransConnect(XtransConnInfo, char *address);
extern int  _XimXTransGetConnectionNumber(XtransConnInfo);
extern int  _XimXTransRead(XtransConnInfo, char *buf, int size);
extern int  _XimXTransWrite(XtransConnInfo, char *buf, int size);
extern int  _XimXTransBytesReadable(XtransConnInfo, BytesReadable_t *);
extern int  _XimXTransDisconnect(XtransConnInfo);
extern int  _XimXTransClose(XtransConnInfo);
extern void _XimXTransFreeConnInfo(XtransConnInfo);
extern int  _XimXTransIsLocal(XtransConnInfo);

/* Select() stub used by <X11/Xpoll.h>: zero timeout -> "nothing readable"
 * (the connection-liveness probe must not block); otherwise report ready and
 * let the blocking read()/write() do the waiting. */
struct timeval;         /* <sys/time.h> */
extern int _XPsxSelect(int maxfds, void *readfds, void *writefds,
                       void *exceptfds, struct timeval *timeout);

_XFUNCPROTOEND

#endif /* _XTRANS_H_ */
