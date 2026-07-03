/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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

#ifndef lint
static char sccsid[] = "@(#)path.c	5.1 (Berkeley) 4/3/91";
#endif /* not lint */
#ifdef DF_POSIX /* DF_DSC */
#include <misc.h>
#include <bsdlib.h>
#else
#include <sys/param.h>
#endif
#include <stdio.h>
#include <string.h>
#include "cp.h"

/*
 * These functions manipulate paths in PATH_T structures.
 * 
 * They eliminate multiple slashes in paths when they notice them,
 * and keep the path non-slash terminated.
 *
 * Both path_set() and path_append() return 0 if the requested name
 * would be too long.
 */

#define	STRIP_TRAILING_SLASH(p) { \
	while ((p)->p_end > (p)->p_path && (p)->p_end[-1] == '/') \
		*--(p)->p_end = 0; \
}

/*
 * Move specified string into path.  Convert "" to "." to handle BSD
 * semantics for a null path.  Strip trailing slashes.
 */
int
#if __STDC__
path_set (register PATH_T *p, char *string)
#else
path_set(p, string)
	register PATH_T *p;
	char *string;
#endif
{
	if (strlen(string) > MAXPATHLEN) {
		(void)fprintf(stderr,
		    "%s: %s: name too long.\n", progname, string);
		return(0);
	}

	(void)strcpy(p->p_path, string);
	p->p_end = p->p_path + strlen(p->p_path);

	if (p->p_path == p->p_end) {
		*p->p_end++ = '.';
		*p->p_end = 0;
	}

	STRIP_TRAILING_SLASH(p);
	return(1);
}

/*
 * Append specified string to path, inserting '/' if necessary.  Return a
 * pointer to the old end of path for restoration.
 */
char *
#if __STDC__
path_append (register PATH_T *p, char *name, int len)
#else
path_append(p, name, len)
	register PATH_T *p;
	char *name;
	int len;
#endif
{
	char *old;

	old = p->p_end;
	if (len == -1)
		len = strlen(name);

	/* The "+ 1" accounts for the '/' between old path and name. */
	if ((len + p->p_end - p->p_path + 1) > MAXPATHLEN) {
		(void)fprintf(stderr,
		    "%s: %s/%s: name too long.\n", progname, p->p_path, name);
		return(0);
	}

	/*
	 * This code should always be executed, since paths shouldn't
	 * end in '/'.
	 */
	if (p->p_end[-1] != '/') {
		*p->p_end++ = '/';
		*p->p_end = 0;
	}

	(void)strncat(p->p_end, name, len);
	p->p_end += len;
	*p->p_end = 0;

	STRIP_TRAILING_SLASH(p);
	return(old);
}

/*
 * Restore path to previous value.  (As returned by path_append.)
 */
void
#if __STDC__
path_restore (PATH_T *p, char *old)
#else
path_restore(p, old)
	PATH_T *p;
	char *old;
#endif
{
	p->p_end = old;
	*p->p_end = 0;
}

/*
 * Return basename of path.
 */
char *
#if __STDC__
path_basename (PATH_T *p)
#else
path_basename(p)
	PATH_T *p;
#endif
{
	char *basename;

	basename = rindex(p->p_path, '/');
	return(basename ? basename + 1 : p->p_path);
}
