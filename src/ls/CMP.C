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

#ifndef lint
static char sccsid[] = "@(#)cmp.c	5.4 (Berkeley) 3/8/91";
#endif /* not lint */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ls.h"

int
#if __STDC__
namecmp (LS *a, LS *b)
#else
namecmp(a, b)
	LS *a, *b;
#endif
{
	return(strcmp(a->name, b->name));
}

int
#if __STDC__
revnamecmp (LS *a, LS *b)
#else
revnamecmp(a, b)
	LS *a, *b;
#endif
{
	return(strcmp(b->name, a->name));
}

int
#if __STDC__
modcmp (LS *a, LS *b)
#else
modcmp(a, b)
	LS *a, *b;
#endif
{
	return((int)(b->lstat.st_mtime - a->lstat.st_mtime));
}

int
#if __STDC__
revmodcmp (LS *a, LS *b)
#else
revmodcmp(a, b)
	LS *a, *b;
#endif
{
	return((int)(a->lstat.st_mtime - b->lstat.st_mtime));
}

int
#if __STDC__
acccmp (LS *a, LS *b)
#else
acccmp(a, b)
	LS *a, *b;
#endif
{
	return((int)(b->lstat.st_atime - a->lstat.st_atime));
}

int
#if __STDC__
revacccmp (LS *a, LS *b)
#else
revacccmp(a, b)
	LS *a, *b;
#endif
{
	return((int)(a->lstat.st_atime - b->lstat.st_atime));
}

int
#if __STDC__
statcmp (LS *a, LS *b)
#else
statcmp(a, b)
	LS *a, *b;
#endif
{
	return((int)(b->lstat.st_ctime - a->lstat.st_ctime));
}

int
#if __STDC__
revstatcmp (LS *a, LS *b)
#else
revstatcmp(a, b)
	LS *a, *b;
#endif
{
	return((int)(a->lstat.st_ctime - b->lstat.st_ctime));
}
