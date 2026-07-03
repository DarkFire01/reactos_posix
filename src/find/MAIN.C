/*-
 * Copyright (c) 1990 The Regents of the University of California.
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
static char sccsid[] = "@(#)main.c	5.9 (Berkeley) 5/24/91";
#endif /* not lint */

#ifdef DF_POSIX	/* DF_MSS */
#include <misc.h>
#include <bsdlib.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <fts.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#if _POSIX_SOURCE
# include <unistd.h>
#endif
#include "find.h"

time_t now;			/* time find was run */
#ifdef _POSIX_SOURCE /* DF_DSC */
    char curr_dir[PATH_MAX +1];   /* starting directory */
    char *curr_dir_ptr;
#else
    int dotfd;			/* starting directory */
#endif
int ftsoptions;			/* options for the ftsopen(3) call */
int isdeprecated;		/* using deprecated syntax */
int isdepth;			/* do directories on post-order visit */
int isoutput;			/* user specified output operator */
int isxargs;			/* don't permit xargs delimiting chars */

#if WIN_NT
extern int globulate __P((int, int, char **));
extern void deglobulate __P((void));
extern int globulated_argc;
extern char **globulated_argv;
pid_t ppid;
int globulation;
#endif

static void usage __P((void));

int
#if __STDC__
main (int argc, char **argv)
#else
main(argc, argv)
	int argc;
	char **argv;
#endif
{
	register char **p, **start;
	PLAN *find_formplan __P((char **));
	int ch;

#if WIN_NT
	ppid = getppid();
	if (ppid == (pid_t) 1) /* if parent is CMD.EXE */
	{
		globulation = globulate(1, argc, argv);
		if (globulation == 0)
		{
			argc = globulated_argc;
			argv = globulated_argv;
		}
	}
# if 0
	(void) fprintf(stderr, "%s", *argv);
	for (ch = 1; ch < argc; ++ch)
	{
		(void) fprintf(stderr, " %s", argv[ch]);
	}
	(void) fputc('\n', stderr);
	(void) fflush(stderr);
# endif
#endif
	(void)time(&now);	/* initialize the time-of-day */

	p = start = argv;
	ftsoptions = FTS_NOSTAT|FTS_PHYSICAL;
	while ((ch = getopt(argc, argv, "df:sXx")) != EOF) {
#if 0 && DF_POSIX
	printf("opt: %c", ch);
#endif
		switch(ch) {
		case 'd':
			isdepth = 1;
			break;
		case 'f':
			*p++ = optarg;
			break;
		case 's':
			ftsoptions &= ~FTS_PHYSICAL;
			ftsoptions |= FTS_LOGICAL;
			break;
		case 'X':
			isxargs = 1;
			break;
		case 'x':
			ftsoptions &= ~FTS_NOSTAT;
			ftsoptions |= FTS_XDEV;
			break;
		case '?':
		default:
			break;
		}
	}
	argc -= optind;	
	argv += optind;
	/* Find first option to delimit the file list. */
	while (*argv) {
		if (option(*argv))
			break;
		*p++ = *argv++;
	}
	if (p == start)
		usage();
	*p = NULL;

#ifdef _POSIX_SOURCE /* DF_DSC: because fchdir (in function.c) not in POSIX */
        if ((curr_dir_ptr = getcwd(curr_dir, sizeof(curr_dir))) == NULL)
#else
	if ((dotfd = open(".", O_RDONLY, 0)) < 0)
#endif
		err(".: %s", strerror(errno));
	find_execute(find_formplan(argv), start);
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	return EXIT_SUCCESS;
}

static void
#if __STDC__
usage (void)
#else
usage()
#endif
{
	(void)fprintf(stderr,
	    "usage: find [-dsXx] [-f file] [file ...] expression\n");
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	exit(EXIT_FAILURE);
}
