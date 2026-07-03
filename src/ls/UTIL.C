/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Fischbein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef DF_POSIX /* DF_MSS */
#include <misc.h>
#include <bsdlib.h>
#include <sys/cdefs.h>
#endif

#ifndef lint
static char sccsid[] = "@(#)util.c	5.8 (Berkeley) 7/22/90";
#endif /* not lint */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#if WIN_NT
void nomem __P((void));
extern void deglobulate __P((void));
extern pid_t ppid;
extern int globulation;
#endif

void
#if __STDC__
prcopy (register char *src, register char *dest, register int len)
#else
prcopy(src, dest, len)
	register char *src, *dest;
	register int len;
#endif
{
	register int ch;

	while(len--) {
		ch = *src++;
		*dest++ = (char)((isprint(ch)) ? ch : '?');
	}
}

char *
#if __STDC__
emalloc (u_int size)
#else
emalloc(size)
	u_int size;
#endif
{
#if WIN_NT
	char *retval;
#else
	char *retval, *malloc();
#endif

#if __STDC__
	if (!(retval = (char *) malloc((size_t)size)))
#else
	if (!(retval = (char *) malloc(size)))
#endif
		nomem();
	return(retval);
}

void
#if __STDC__
nomem (void)
#else
nomem()
#endif
{
	(void)fprintf(stderr, "ls: out of memory.\n");
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	exit(EXIT_FAILURE);
}

void
#if __STDC__
usage (void)
#else
usage()
#endif
{
#if _POSIX_SOURCE
	(void)fprintf(stderr, "usage: ls [-1ACFLRTacdfiklqrstu] [file ...]\n");
#else
	(void)fprintf(stderr, "usage: ls [-1ACFLRTacdfgiklqrstu] [file ...]\n");
#endif
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	exit(EXIT_FAILURE);
}
