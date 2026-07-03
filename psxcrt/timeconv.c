/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem  --  LICENSE: MIT
 * PURPOSE:     <time.h> conversions (gmtime/localtime/ctime/asctime/mktime) for
 *              the userland. No timezone database -- local time == UTC.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */
#include <sys/types.h>
#include <time.h>
extern int sprintf(char *, const char *, ...);

static const int g_MDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
#define IS_LEAP(y) (((y) % 4 == 0 && (y) % 100 != 0) || (y) % 400 == 0)

struct tm *gmtime(const time_t *timer)
{
    static struct tm tm;
    long secs = *timer, days, rem;
    int y, m, leap;

    days = secs / 86400; rem = secs % 86400;
    if (rem < 0) { rem += 86400; days--; }
    tm.tm_hour = (int)(rem / 3600); rem %= 3600;
    tm.tm_min  = (int)(rem / 60);   tm.tm_sec = (int)(rem % 60);
    tm.tm_wday = (int)((4 + days) % 7);            /* 1970-01-01 = Thursday */
    if (tm.tm_wday < 0) tm.tm_wday += 7;

    for (y = 1970; ; y++)
    {
        long yd = IS_LEAP(y) ? 366 : 365;
        if (days < yd) break;
        days -= yd;
    }
    tm.tm_year = y - 1900;
    tm.tm_yday = (int)days;
    leap = IS_LEAP(y);
    for (m = 0; ; m++)
    {
        int dm = g_MDays[m] + (m == 1 && leap ? 1 : 0);
        if (days < dm) break;
        days -= dm;
    }
    tm.tm_mon = m;
    tm.tm_mday = (int)days + 1;
    tm.tm_isdst = 0;
    return &tm;
}

struct tm *localtime(const time_t *timer) { return gmtime(timer); }

char *asctime(const struct tm *tm)
{
    static const char wd[7][4]  = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char mo[12][4] = {"Jan","Feb","Mar","Apr","May","Jun",
                                   "Jul","Aug","Sep","Oct","Nov","Dec"};
    static char buf[32];
    sprintf(buf, "%s %s %2d %02d:%02d:%02d %d\n",
            wd[tm->tm_wday % 7], mo[tm->tm_mon % 12], tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
    return buf;
}

char *ctime(const time_t *timer) { return asctime(localtime(timer)); }

double difftime(time_t a, time_t b) { return (double)(a - b); }

time_t mktime(struct tm *tm)
{
    long days = 0;
    int y;
    for (y = 1970; y < tm->tm_year + 1900; y++)
        days += IS_LEAP(y) ? 366 : 365;
    {
        int m, leap = IS_LEAP(tm->tm_year + 1900);
        for (m = 0; m < tm->tm_mon; m++)
            days += g_MDays[m] + (m == 1 && leap ? 1 : 0);
    }
    days += tm->tm_mday - 1;
    return (time_t)days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
}

/* MSVC emits a reference to __fltused when a TU uses floating point; with
 * /NODEFAULTLIB nothing defines it. This marker satisfies the linker. */
int _fltused = 0x9875;
