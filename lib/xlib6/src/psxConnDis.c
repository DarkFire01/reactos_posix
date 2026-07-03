/*
 * psxConnDis.c -- psx replacement for the X11R6.9 ConnDis.c.
 *
 * The original ConnDis.c is the socket/DECnet/TLI display-connection engine
 * plus the Xau/XDM/Kerberos authorization machinery.  On the ReactOS POSIX
 * subsystem the only transport is the local /dev/x11 bridge (opened by
 * _X11TransConnectDisplay in Xtranspsx6.c) and there is no authorization,
 * so this file keeps just the pieces the rest of libX11 links against:
 *   _XConnectDisplay / _XDisconnectDisplay / _XSendClientPrefix /
 *   XSetAuthorization
 * faithful to the originals minus the auth protocols.
 *
 * Derived from X11R6.9 lib/X11/ConnDis.c; MIT/X license (see copyright.h).
 */

#include <X11/Xlibint.h>
#include <X11/Xtrans/Xtrans.h>
#include <X11/Xauth.h>
#include "Xintconn.h"

/*
 * Kept for compatibility (the test suite relies on this interface).
 */
int _XConnectDisplay (
    char *display_name,
    char **fullnamep,			/* RETURN */
    int *dpynump,			/* RETURN */
    int *screenp,			/* RETURN */
    char **auth_namep,			/* RETURN */
    int *auth_namelenp,			/* RETURN */
    char **auth_datap,			/* RETURN */
    int *auth_datalenp)			/* RETURN */
{
   XtransConnInfo trans_conn;

   trans_conn = _X11TransConnectDisplay (
		      display_name, fullnamep, dpynump, screenp,
		      auth_namep, auth_namelenp, auth_datap, auth_datalenp);

   if (trans_conn)
   {
       int fd = _X11TransGetConnectionNumber (trans_conn);
       _X11TransFreeConnInfo (trans_conn);
       return (fd);
   }
   else
       return (-1);
}

/*
 * Disconnect from server.
 */
int _XDisconnectDisplay (trans_conn)

XtransConnInfo	trans_conn;

{
    _X11TransDisconnect(trans_conn);
    _X11TransClose(trans_conn);
    return 0;
}


Bool
_XSendClientPrefix (dpy, client, auth_proto, auth_string, prefix)
     Display *dpy;
     xConnClientPrefix *client;		/* contains count for auth_* */
     char *auth_proto, *auth_string;	/* NOT null-terminated */
     xConnSetupPrefix *prefix;		/* prefix information */
{
    int auth_length = client->nbytesAuthProto;
    int auth_strlen = client->nbytesAuthString;
    static char padbuf[3];		/* for padding to 4x bytes */
    int pad;
    struct iovec iovarray[5], *iov = iovarray;
    int niov = 0;
    int len = 0;

#define add_to_iov(b,l) \
  { iov->iov_base = (b); iov->iov_len = (l); iov++, niov++; len += (l); }

    add_to_iov ((caddr_t) client, SIZEOF(xConnClientPrefix));

    /*
     * write authorization protocol name and data
     */
    if (auth_length > 0) {
	add_to_iov (auth_proto, auth_length);
	pad = -auth_length & 3; /* pad auth_length to a multiple of 4 */
	if (pad) add_to_iov (padbuf, pad);
    }
    if (auth_strlen > 0) {
	add_to_iov (auth_string, auth_strlen);
	pad = -auth_strlen & 3; /* pad auth_strlen to a multiple of 4 */
	if (pad) add_to_iov (padbuf, pad);
    }

#undef add_to_iov

    len -= _X11TransWritev (dpy->trans_conn, iovarray, niov);

    /* the psx bridge stays a blocking stream; SetOption is a no-op */
    _X11TransSetOption(dpy->trans_conn, TRANS_NONBLOCKING, 1);

    if (len != 0)
	return -1;

    return 0;
}


/*
 * Routine for setting authorization data.  Stored faithfully, though the
 * local bridge never sends it (ConnectDisplay reports no-auth).
 */
static int xauth_namelen = 0;
static char *xauth_name = NULL;	 /* NULL means use default mechanism */
static int xauth_datalen = 0;
static char *xauth_data = NULL;	 /* NULL means get default data */

void XSetAuthorization (name, namelen, data, datalen)
    int namelen, datalen;		/* lengths of name and data */
    char *name, *data;			/* NULL or arbitrary array of bytes */
{
    char *tmpname, *tmpdata;

    _XLockMutex(_Xglobal_lock);
    if (xauth_name) Xfree (xauth_name);	 /* free any existing data */
    if (xauth_data) Xfree (xauth_data);

    xauth_name = xauth_data = NULL;	/* mark it no longer valid */
    xauth_namelen = xauth_datalen = 0;
    _XUnlockMutex(_Xglobal_lock);

    if (namelen < 0) namelen = 0;	/* check for bogus inputs */
    if (datalen < 0) datalen = 0;	/* maybe should return? */

    if (namelen > 0)  {			/* try to allocate space */
	tmpname = Xmalloc ((unsigned) namelen);
	if (!tmpname) return;
	memcpy (tmpname, name, namelen);
    } else {
	tmpname = NULL;
    }

    if (datalen > 0)  {
	tmpdata = Xmalloc ((unsigned) datalen);
	if (!tmpdata) {
	    if (tmpname) Xfree (tmpname);
	    return;
	}
	memcpy (tmpdata, data, datalen);
    } else {
	tmpdata = NULL;
    }

    _XLockMutex(_Xglobal_lock);
    xauth_name = tmpname;		/* and store the suckers */
    xauth_namelen = namelen;
    xauth_data = tmpdata;
    xauth_datalen = datalen;
    _XUnlockMutex(_Xglobal_lock);
}
