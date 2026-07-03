/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem  --  LICENSE: MIT
 * PURPOSE:     The BSD err/warn family (err/errx/warn/warnx) + __progname.
 *              Kept in its OWN object so a util that defines its own err() does
 *              not pull this one in (which would collide) -- it can still take
 *              perror/strerror from err.c. find/vi (which have no local err) get
 *              these from the library.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */
#include <stdio.h>
#include <stdarg.h>

extern int   errno;
extern char *strerror(int);
extern void  exit(int);

/* err/warn print "<progname>: ...". Generic default; apps may override. */
char *__progname = "posix";

static void vwarn(const char *fmt, va_list ap, int useerrno)
{
    int e = errno;
    fprintf(stderr, "%s: ", __progname);
    if (fmt) vfprintf(stderr, fmt, ap);
    if (useerrno) fprintf(stderr, ": %s", strerror(e));
    fputc('\n', stderr);
}

void warn(const char *fmt, ...)
    { va_list ap; va_start(ap, fmt); vwarn(fmt, ap, 1); va_end(ap); }
void warnx(const char *fmt, ...)
    { va_list ap; va_start(ap, fmt); vwarn(fmt, ap, 0); va_end(ap); }
void err(int code, const char *fmt, ...)
    { va_list ap; va_start(ap, fmt); vwarn(fmt, ap, 1); va_end(ap); exit(code); }
void errx(int code, const char *fmt, ...)
    { va_list ap; va_start(ap, fmt); vwarn(fmt, ap, 0); va_end(ap); exit(code); }
