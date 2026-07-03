/*
 * printf and fprintf
 */

/* $Header */

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdio.h>

#if _V7 || _BSD

/* printf to stdout */
int
#if __STDC__
printf(Const char *fmt, ...) {
#else
printf(va_alist) va_dcl
{
	char *fmt;
#endif
	va_list va;

#if __STDC__
	va_start(va, fmt);
#else
	va_start(va);
	fmt = va_arg(va, char *);
#endif
	vfprintf(stdout, fmt, va);
	va_end(va);
	return 0;
}

int
#if __STDC__
fprintf(FILE *f, Const char *fmt, ...) {
#else
fprintf(va_alist) va_dcl
{
	FILE *f;
	char *fmt;
#endif
	va_list va;

#if __STDC__
	va_start(va, fmt);
#else
	va_start(va);
	f = va_arg(va, FILE *);
	fmt = va_arg(va, char *);
#endif
	vfprintf(f, fmt, va);
	va_end(va);
	return 0;
}

#endif
