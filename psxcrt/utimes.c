/* utimes -> utime (psxdll exports utime, not utimes). MIT. */
#include <sys/types.h>
#include <sys/time.h>
#include <utime.h>
extern int utime(const char *, const struct utimbuf *);
int utimes(const char *path, const struct timeval *tv)
{
    struct utimbuf ut;
    ut.actime  = tv[0].tv_sec;
    ut.modtime = tv[1].tv_sec;
    return utime(path, &ut);
}
