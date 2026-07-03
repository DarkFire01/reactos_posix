/*
 * X11/extensions/Print.h -- psx-minimal stub of the Xprint extension header.
 *
 * OpenMotif's <Xm/Xm.h> includes this unconditionally, but the only Xprint type
 * that reaches the Motif *headers* is XPContext (one field in XmPrintShellPart).
 * The one file that calls the Xp* client library (PrintS.c / XmPrintShell) is
 * excluded from libXm2 -- CDE does not use X print.  This stub supplies just
 * enough for the headers to parse.  MIT-licensed (ReactOS POSIX subsystem).
 */
#ifndef _XPRINT_PSXSTUB_H_
#define _XPRINT_PSXSTUB_H_

#include <X11/Xlib.h>       /* XID, and everything Xm.h has already pulled in */

typedef XID XPContext;

typedef enum { XPGetDocFinished, XPGetDocSecondConsumer } XPGetDocStatus;

/* XPFinishProc: Xprint's job-finished callback -- Motif's <Xm/Print.h> (pulled
 * in by XmAll.h) declares a proto using it.  Generic signature is enough to
 * parse; no Xp client code is compiled (PrintS.c excluded). */
typedef void (*XPFinishProc)(Display *, XPContext, XPGetDocStatus);

/* Public Xprint enums referenced (defensively) by print-aware code paths. */
typedef enum {
    XPStartJobNotify   = 1,
    XPEndJobNotify     = 2,
    XPStartDocNotify   = 3,
    XPEndDocNotify     = 4,
    XPStartPageNotify  = 5,
    XPEndPageNotify    = 6
} XPPrintOperation;

typedef enum { XPSpool, XPGetData }        XPSaveData;
typedef enum { XPDocNormal, XPDocRaw }     XPDocumentType;
typedef enum { XPJobAttr, XPDocAttr, XPPageAttr, XPPrinterAttr, XPServerAttr }
    XPAttributes;

#endif /* _XPRINT_PSXSTUB_H_ */
