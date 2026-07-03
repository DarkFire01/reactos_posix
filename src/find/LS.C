/*
 * Copyright (c) 1989 The Regents of the University of California.
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
static char sccsid[] = "@(#)ls.c	5.6 (Berkeley) 3/9/91";
#endif /* not lint */

#ifdef DF_POSIX	/* DF_MSS */
#include <misc.h>
#include <bsdlib.h>
#include <sys/cdefs.h>
#else
#include <sys/param.h>
#endif
#include <sys/stat.h>
#include <time.h>
#include <tzfile.h>
#include <errno.h>
#include <utmp.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#if WIN_NT

extern char *group_from_gid __P((gid_t, int));
extern char *user_from_uid __P((uid_t, int));
#endif

void printtime __P((time_t));

/* Derived from the print routines in the ls(1) source code. */

void
#if __STDC__
printlong (char *name, char *accpath, struct stat *sb)
#else
printlong(name, accpath, sb)
	char *name;			/* filename to print */
	char *accpath;			/* current valid path to filename */
	struct stat *sb;		/* stat buffer */
#endif
{
#if WIN_NT
	char modep[15];
#else
	char modep[15], *user_from_uid(), *group_from_gid();
#endif

#ifdef _POSIX_SOURCE /* DF_DSC */
	(void)printf("%6lu %4ld ", sb->st_ino, ((sb->st_size/BLOCK_SIZE) + 1));
#else
	(void)printf("%6lu %4ld ", sb->st_ino, sb->st_blocks);
#endif
	(void)strmode(sb->st_mode, modep);
	(void)printf("%s %3u %-*s %-*s ", modep, sb->st_nlink, UT_NAMESIZE,
	    user_from_uid(sb->st_uid, 0), UT_NAMESIZE,
	    group_from_gid(sb->st_gid, 0));

#ifdef _POSIX_SOURCE /* DF_DSC: No devices for the moment in Windows NT POSIX */
		(void)printf("%8ld ", sb->st_size);
#else
	if (S_ISCHR(sb->st_mode) || S_ISBLK(sb->st_mode))
		(void)printf("%3d, %3d ", major(sb->st_rdev),
		    minor(sb->st_rdev));
	else
		(void)printf("%8ld ", sb->st_size);
#endif
	printtime(sb->st_mtime);
	(void)printf("%s", name);
#ifdef _POSIX_SOURCE /* DF_DSC: softlinks not in Windows NT POSIX */
	if (accpath == NULL)
		;
#else
	if (S_ISLNK(sb->st_mode))
		printlink(accpath);
#endif
	(void)putchar('\n');
}

void
#if __STDC__
printtime (time_t ftime)
#else
printtime(ftime)
	time_t ftime;
#endif
{
	int i;
#if WIN_NT
	char *longstring;
#else
	char *longstring, *ctime();
	time_t time();
#endif

	longstring = ctime((long *)&ftime);
	for (i = 4; i < 11; ++i)
		(void)putchar(longstring[i]);

#define	SIXMONTHS	((DAYSPERNYEAR / 2) * SECSPERDAY)
	if (ftime + SIXMONTHS > time((time_t *)NULL))
		for (i = 11; i < 16; ++i)
			(void)putchar(longstring[i]);
	else {
		(void)putchar(' ');
		for (i = 20; i < 24; ++i)
			(void)putchar(longstring[i]);
	}
	(void)putchar(' ');
}

#ifdef _POSIX_SOURCE /* DF_DSC: softlinks not in Windows NT POSIX */
#else
# if __STDC__
printlink (char *name)
# else
printlink(name)
	char *name;
# endif
{
	int lnklen;
	char path[MAXPATHLEN + 1];

	if ((lnklen = readlink(name, path, MAXPATHLEN)) == -1) {
		(void)fprintf(stderr,
		    "\nfind: %s: %s\n", name, strerror(errno));
		return;
	}
	path[lnklen] = '\0';
	(void)printf(" -> %s", path);
}
#endif
