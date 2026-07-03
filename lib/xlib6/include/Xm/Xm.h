/*
 * Xm/Xm.h -- psx placeholder until Motif 2.1 is ported.
 *
 * Pre-Motif CDE clients (dthello) include <Xm/Xm.h> but link no Motif; give
 * them the version macros and the Xt world.  The real header replaces this
 * when libXm lands (Phase 5).
 * MIT/X license (see copyright.h).
 */
#ifndef _XmXm_h
#define _XmXm_h

#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#define XmVERSION	2
#define XmREVISION	1
#define XmUPDATE_LEVEL	0
#define XmVersion	(XmVERSION * 1000 + XmREVISION)
#define XmVERSION_STRING "@(#)Motif Version 2.1.0 (psx placeholder)"

#endif /* _XmXm_h */
