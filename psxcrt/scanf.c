/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem  --  LICENSE: MIT
 * PURPOSE:     Minimal sscanf for the userland: %d/%i/%u/%x/%o/%c/%s/%l*,
 *              width + '*' suppression, whitespace and literal matching.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */
#include <stdarg.h>

extern int isspace(int);
extern int isdigit(int);

static long ScanInt(const char **s, int base, int width)
{
    long v = 0; int neg = 0, n = 0;
    if (**s == '-') { neg = 1; (*s)++; } else if (**s == '+') (*s)++;
    if (base == 16 && (*s)[0]=='0' && ((*s)[1]=='x'||(*s)[1]=='X')) *s += 2;
    while ((width == 0 || n < width))
    {
        int c = **s, d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else break;
        if (d >= base) break;
        v = v * base + d; (*s)++; n++;
    }
    return neg ? -v : v;
}

int sscanf(const char *str, const char *fmt, ...)
{
    va_list ap;
    int matched = 0;
    va_start(ap, fmt);

    while (*fmt)
    {
        if (isspace((unsigned char)*fmt)) { while (isspace((unsigned char)*str)) str++; fmt++; continue; }
        if (*fmt != '%') { if (*str != *fmt) break; str++; fmt++; continue; }

        fmt++;                              /* past '%' */
        {
            int suppress = 0, width = 0, isLong = 0;
            if (*fmt == '*') { suppress = 1; fmt++; }
            while (isdigit((unsigned char)*fmt)) width = width * 10 + (*fmt++ - '0');
            while (*fmt == 'l' || *fmt == 'h') { if (*fmt == 'l') isLong = 1; fmt++; }

            if (*fmt != 'c' && *fmt != '[') while (isspace((unsigned char)*str)) str++;

            switch (*fmt)
            {
            case 'd': case 'i': case 'u': {
                const char *p = str; long v = ScanInt(&str, 10, width);
                if (str == p) goto done;
                if (!suppress) { if (isLong) *va_arg(ap,long*)=v; else *va_arg(ap,int*)=(int)v; matched++; }
                break; }
            case 'x': case 'X': {
                const char *p = str; long v = ScanInt(&str, 16, width);
                if (str == p) goto done;
                if (!suppress) { if (isLong) *va_arg(ap,long*)=v; else *va_arg(ap,int*)=(int)v; matched++; }
                break; }
            case 'o': {
                const char *p = str; long v = ScanInt(&str, 8, width);
                if (str == p) goto done;
                if (!suppress) { *va_arg(ap,int*)=(int)v; matched++; }
                break; }
            case 's': {
                char *out = suppress ? 0 : va_arg(ap, char*); int n = 0;
                if (!*str) goto done;
                while (*str && !isspace((unsigned char)*str) && (width == 0 || n < width))
                { if (out) *out++ = *str; str++; n++; }
                if (out) *out = '\0';
                if (!suppress) matched++;
                break; }
            case 'c': {
                char *out = suppress ? 0 : va_arg(ap, char*); int n = 0, w = width ? width : 1;
                while (*str && n < w) { if (out) *out++ = *str; str++; n++; }
                if (n < w) goto done;
                if (!suppress) matched++;
                break; }
            case '%': if (*str != '%') goto done; str++; break;
            default: goto done;
            }
            fmt++;
        }
    }
done:
    va_end(ap);
    return matched;
}
