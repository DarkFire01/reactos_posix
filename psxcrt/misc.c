/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Small <stdlib.h> pieces the reskit userland needs that psxdll and
 *              SH/STD don't provide: getenv, the atoi/strto* conversions, qsort,
 *              bsearch, abs, abort.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <sys/types.h>

extern char **environ;
extern int   strncmp(const char *, const char *, size_t);
extern size_t strlen(const char *);
extern void  _exit(int Status);
extern int   write(int Fd, const void *Buf, unsigned Count);

char *getenv(const char *name)
{
    size_t n = strlen(name);
    char **p;
    if (environ == 0)
        return 0;
    for (p = environ; *p != 0; p++)
        if (strncmp(*p, name, n) == 0 && (*p)[n] == '=')
            return *p + n + 1;
    return 0;
}

static int Digit(int c, int base)
{
    int d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
    else return -1;
    return (d < base) ? d : -1;
}

static unsigned long StrToU(const char *s, char **end, int base, int *neg)
{
    unsigned long v = 0;
    int d;
    *neg = 0;
    while (*s == ' ' || *s == '\t' || *s == '\n') s++;
    if (*s == '+') s++;
    else if (*s == '-') { *neg = 1; s++; }
    if (base == 0)
    {
        if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (*s == '0') { base = 8; s++; }
        else base = 10;
    }
    else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    while ((d = Digit(*s, base)) >= 0) { v = v * base + d; s++; }
    if (end) *end = (char *)s;
    return v;
}

long strtol(const char *s, char **end, int base)
{
    int neg;
    unsigned long v = StrToU(s, end, base, &neg);
    return neg ? -(long)v : (long)v;
}

unsigned long strtoul(const char *s, char **end, int base)
{
    int neg;
    unsigned long v = StrToU(s, end, base, &neg);
    return neg ? (unsigned long)(-(long)v) : v;
}

int  atoi(const char *s) { return (int)strtol(s, 0, 10); }
long atol(const char *s) { return strtol(s, 0, 10); }
int  abs(int v)          { return v < 0 ? -v : v; }
long labs(long v)        { return v < 0 ? -v : v; }

void abort(void)
{
    static const char msg[] = "abort\n";
    write(2, msg, sizeof(msg) - 1);
    _exit(0x86);        /* 134 = 128 + SIGABRT-ish */
}

static void SwapBytes(char *a, char *b, size_t n)
{
    char t;
    while (n--) { t = *a; *a++ = *b; *b++ = t; }
}

/* Shell sort -- simple, no recursion/stack risk, adequate for the userland. */
void qsort(void *base, size_t nmemb, size_t size,
           int (*cmp)(const void *, const void *))
{
    char *a = (char *)base;
    size_t gap, i, j;
    for (gap = nmemb / 2; gap > 0; gap /= 2)
        for (i = gap; i < nmemb; i++)
            for (j = i - gap;
                 (long)j >= 0 && cmp(a + j * size, a + (j + gap) * size) > 0;
                 j -= gap)
                SwapBytes(a + j * size, a + (j + gap) * size, size);
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*cmp)(const void *, const void *))
{
    const char *a = (const char *)base;
    size_t lo = 0, hi = nmemb;
    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        int r = cmp(key, a + mid * size);
        if (r == 0) return (void *)(a + mid * size);
        if (r < 0) hi = mid;
        else lo = mid + 1;
    }
    return 0;
}

/* rand/srand -- ANSI C linear congruential generator ($RANDOM in the shell). */
static unsigned long g_RandNext = 1;
int rand(void)
{
    g_RandNext = g_RandNext * 1103515245UL + 12345UL;
    return (int)((g_RandNext >> 16) & 0x7fff);
}
void srand(unsigned int seed) { g_RandNext = seed; }
