/*
 * xpsxpre6.h -- force-included prelude for the X11R6.9 libX11 port (via /FI).
 * Pulls the C runtime BEFORE any <X11/...> header so Xlib's `typedef int
 * Status` can't poison POSIX SDK prototypes that use `Status` as a parameter
 * name, and supplies the BSD-isms the sources expect.
 * MIT/X license (see copyright.h).
 */
#ifndef _XPSXPRE6_H_
#define _XPSXPRE6_H_

#include <stddef.h>     /* size_t, ptrdiff_t, wchar_t */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <strings.h>    /* bcopy/bzero/index/rindex prototypes */

/* The NT POSIX subsystem has no non-blocking I/O; the connection is a
 * blocking byte stream, so this errno never actually occurs. */
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif

/* Motif's bundled extras/wchar <wchar.h> (FreeBSD-derived) would otherwise
 * redefine wchar_t as `int` (clashing with psxcrt's 16-bit wchar_t) and
 * typedef mbstate_t from an undefined __mbstate_t.  Pre-set the BSD guards so
 * it keeps our wchar_t (from <stddef.h> above) and we supply mbstate_t.  These
 * guard names are BSD-specific (psxcrt uses _WCHAR_T_DEFINED), so non-Motif
 * targets sharing this prelude are unaffected. */
#define _WCHAR_T_DECLARED
#ifndef _MBSTATE_T_DECLARED
#define _MBSTATE_T_DECLARED
typedef int mbstate_t;
#endif

#endif /* _XPSXPRE6_H_ */
