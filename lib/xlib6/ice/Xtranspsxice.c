/*
 * Xtranspsxice.c -- the ICE transport for the ReactOS POSIX subsystem.
 *
 * ICE connections go client <-> session manager (or other ICE-speaking
 * peers).  No such server exists on the subsystem, so every open/listen
 * fails cleanly -- the same client-visible behavior as an unset
 * SESSION_MANAGER on unix, which every Xt/SM client already tolerates
 * (IceOpenConnection returns NULL, the app runs session-less).  The
 * byte-level functions operate on a real fd so that, if a local ICE
 * rendezvous ever exists (a psxss bridge like /dev/x11), only
 * OpenCOTSClient/Connect need upgrading.
 *
 * MIT/X license (see copyright.h).
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <X11/Xos.h>
#include <X11/Xtrans/Xtrans.h>

struct _XtransConnInfo {
    int fd;
};

XtransConnInfo
_IceTransOpenCOTSClient (char *address)
{
    return NULL;		/* no ICE-speaking peer reachable */
}

int
_IceTransConnect (XtransConnInfo ci, char *address)
{
    return TRANS_CONNECT_FAILED;
}

XtransConnInfo
_IceTransAccept (XtransConnInfo ci, int *status_ret)
{
    if (status_ret)
	*status_ret = -1;
    return NULL;
}

int
_IceTransSetOption (XtransConnInfo ci, int option, int arg)
{
    return 0;
}

int
_IceTransClose (XtransConnInfo ci)
{
    if (ci) {
	if (ci->fd >= 0)
	    close (ci->fd);
	free ((char *) ci);
    }
    return 1;
}

int
_IceTransRead (XtransConnInfo ci, char *buf, int size)
{
    return ci ? read (ci->fd, buf, size) : -1;
}

int
_IceTransWrite (XtransConnInfo ci, char *buf, int size)
{
    return ci ? write (ci->fd, buf, size) : -1;
}

int
_IceTransGetConnectionNumber (XtransConnInfo ci)
{
    return ci ? ci->fd : -1;
}

int
_IceTransIsLocal (XtransConnInfo ci)
{
    return 1;
}

char *
_IceTransGetMyNetworkId (XtransConnInfo ci)
{
    return NULL;
}

char *
_IceTransGetPeerNetworkId (XtransConnInfo ci)
{
    return NULL;
}

int
_IceTransMakeAllCOTSServerListeners (char *port, int *partial,
				     int *count_ret,
				     XtransConnInfo **ciptrs_ret)
{
    if (partial)
	*partial = 0;
    if (count_ret)
	*count_ret = 0;
    if (ciptrs_ret)
	*ciptrs_ret = NULL;
    return -1;			/* cannot listen; IceListen* fails cleanly */
}
