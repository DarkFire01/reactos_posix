/*
 * xpsxcompat.c -- BSD byte/string primitives for the libX11 port, mapped onto
 * the C library, plus the unix-isms libXt needs (gettimeofday/gethostname/
 * strcoll/vsnprintf). Note bcopy's argument order: (src, dst, n), unlike memcpy.
 * MIT/X license (see copyright.h). Part of the ReactOS psx libX11 port.
 */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/utsname.h>

void bcopy(const void *src, void *dst, int n)
{
    memmove(dst, src, (size_t)n);
}

void bzero(void *dst, int n)
{
    memset(dst, 0, (size_t)n);
}

int bcmp(const void *a, const void *b, int n)
{
    return memcmp(a, b, (size_t)n);
}

char *index(const char *s, int c)
{
    return strchr(s, c);
}

char *rindex(const char *s, int c)
{
    return strrchr(s, c);
}

/*
 * gettimeofday for Xt's timer bookkeeping.  The MS psxdll computes times()'s
 * return value client-side from NtQuerySystemTime at CLK_TCK=100 (10ms ticks),
 * so this is a cheap, consistent, monotonically-increasing clock -- exactly
 * what XtAppAddTimeOut needs (it only takes deltas).  Not wall-clock time.
 */
#define XPSX_CLK_TCK 100

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    struct tms t;
    long ticks = (long) times(&t);

    if (tv) {
        tv->tv_sec  = ticks / XPSX_CLK_TCK;
        tv->tv_usec = (ticks % XPSX_CLK_TCK) * (1000000L / XPSX_CLK_TCK);
    }
    return 0;
}

int gethostname(char *name, size_t namelen)
{
    struct utsname un;

    if (uname(&un) == 0 && un.nodename[0]) {
        strncpy(name, un.nodename, namelen - 1);
        name[namelen - 1] = '\0';
    } else {
        strncpy(name, "reactos", namelen - 1);
        name[namelen - 1] = '\0';
    }
    return 0;
}

int strcoll(const char *a, const char *b)
{
    return strcmp(a, b);        /* POSIX locale collation == byte order */
}

/* era-appropriate: psxcrt's own snprintf ignores the bound the same way */
extern int vsprintf(char *s, const char *fmt, va_list va);

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
    return vsprintf(str, fmt, ap);
}

char *strdup(const char *s)
{
    char *d = (char *) malloc(strlen(s) + 1);

    if (d)
        strcpy(d, s);
    return d;
}

/* C locale (X_LOCALE): every character is one byte */
int mblen(const char *s, size_t n)
{
    if (s == 0)
        return 0;               /* no state-dependent encodings */
    if (n == 0)
        return -1;
    return *s ? 1 : 0;
}

/*
 * putenv -- cooperates with psxcrt getenv(), which walks `environ`.
 * The string is stored by reference (POSIX semantics); on append the
 * vector is reallocated (the old one leaks, as in most classic libcs).
 */
extern char **environ;

int putenv(char *string)
{
    const char *eq = strchr(string, '=');
    size_t namelen;
    char **p, **nv;
    int count = 0;

    if (!eq)
        return -1;
    namelen = (size_t)(eq - string);

    if (environ) {
        for (p = environ; *p; p++, count++) {
            if (strncmp(*p, string, namelen) == 0 && (*p)[namelen] == '=') {
                *p = string;            /* replace in place */
                return 0;
            }
        }
    }

    nv = (char **) malloc(sizeof(char *) * (count + 2));
    if (!nv)
        return -1;
    for (p = environ, count = 0; p && *p; p++, count++)
        nv[count] = *p;
    nv[count] = string;
    nv[count + 1] = 0;
    environ = nv;
    return 0;
}

/*
 * Message catalogs (see compat/nl_types.h): no catalogs exist on the
 * subsystem, so catopen always fails and catgets returns the caller's
 * default string -- the standard fallback, CDE shows built-in messages.
 */
void *catopen(const char *name, int oflag)
{
    return (void *)-1;
}

char *catgets(void *catd, int set_id, int msg_id, const char *s)
{
    return (char *)s;
}

int catclose(void *catd)
{
    return 0;
}
