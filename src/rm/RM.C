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

#ifdef DF_POSIX /* DF_MSS */
#include <misc.h>
#include <bsdlib.h>
#endif
#ifndef _POSIX_SOURCE /* DF_MSS */
_S_ISLNK(){}
#endif

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1990 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rm.c	4.26 (Berkeley) 3/10/91";
#endif /* not lint */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _POSIX_SOURCE /* DF_MSS */
#include <errno.h>
#else
#include <sys/errno.h>
#endif

#include <fts.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int dflag, fflag, iflag, retval, stdin_ok;

#if WIN_NT
extern char *user_from_uid __P((uid_t, int));
extern char *group_from_gid __P((gid_t, int));
extern int globulate __P((int, int, char **));
extern void deglobulate __P((void));
extern int globulated_argc;
extern char **globulated_argv;
pid_t ppid;
int globulation;
#endif
void rmtree __P((char **)), rmfile __P((char **)), checkdot __P((char **));
void error __P((char *, int)), usage __P((void));
int check __P((char *, char *, struct stat *));

/*
 * rm --
 *	This rm is different from historic rm's, but is expected to match
 *	POSIX 1003.2 behavior.  The most visible difference is that -f
 *	has two specific effects now, ignore non-existent files and force
 * 	file removal.
 */

int
#if __STDC__
main (int argc, char **argv)
#else
main(argc, argv)
	int argc;
	char **argv;
#endif
{
	extern char *optarg;
	extern int optind;
	int ch, rflag;

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
#endif
        rflag = 0;
	while ((ch = getopt(argc, argv, "dfiRr")) != EOF)
		switch(ch) {
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			iflag = 0;
			break;
		case 'i':
			fflag = 0;
			iflag = 1;
			break;
		case 'R':
		case 'r':			/* compatibility */
			rflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	checkdot(argv);
	if (!*argv) {
#if WIN_NT
		if (ppid == (pid_t) 1 && globulation == 0)
			deglobulate();
#endif
		exit(retval);
	}

	stdin_ok = isatty(STDIN_FILENO);

	if (rflag)
		rmtree(argv);
	else
		rmfile(argv);
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	return retval;
}

void
#if __STDC__
rmtree (char **argv)
#else
rmtree(argv)
	char **argv;
#endif
{
	register FTS *fts;
	register FTSENT *p;
	register int needstat;

	/*
	 * Remove a file hierarchy.  If forcing removal (-f), or interactive
	 * (-i) or can't ask anyway (stdin_ok), don't stat the file.
	 */
	needstat = !fflag && !iflag && stdin_ok;

	/*
	 * If the -i option is specified, the user can skip on the pre-order
	 * visit.  The fts_number field flags skipped directories.
	 */
#define	SKIPPED	1

	if (!(fts = fts_open(argv,
	    needstat ? FTS_PHYSICAL : FTS_PHYSICAL|FTS_NOSTAT,
	    (int (*) __P((const FTSENT *, const FTSENT *)))NULL))) {
		(void)fprintf(stderr, "rm: %s.\n", strerror(errno));
#if WIN_NT
		if (ppid == (pid_t) 1 && globulation == 0)
			deglobulate();
#endif
		exit(EXIT_FAILURE);
	}
	while ((p = fts_read(fts)) != NULL) {
		switch(p->fts_info) {
		case FTS_DNR:
		case FTS_ERR:
			error(p->fts_path, errno);
#if WIN_NT
			if (ppid == (pid_t) 1 && globulation == 0)
				deglobulate();
#endif
			exit(EXIT_FAILURE);
		/*
		 * FTS_NS: assume that if can't stat the file, it can't be
		 * unlinked.
		 */
		case FTS_NS:
			if (!needstat)
				break;
			if (!fflag || errno != ENOENT)
				error(p->fts_path, errno);
			continue;
		/* Pre-order: give user chance to skip. */
		case FTS_D:
			if (iflag && !check(p->fts_path, p->fts_accpath,
			    &p->fts_statb)) {
				(void)fts_set(fts, p, FTS_SKIP);
				p->fts_number = SKIPPED;
			}
			continue;
		/* Post-order: see if user skipped. */
		case FTS_DP:
			if (p->fts_number == SKIPPED)
				continue;
			break;
		}

		if (!fflag &&
		    !check(p->fts_path, p->fts_accpath, &p->fts_statb))
			continue;
		/*
		 * If we can't read or search the directory, may still be
		 * able to remove it.  Don't print out the un{read,search}able
		 * message unless the remove fails.
		 */
		if (p->fts_info == FTS_DP || p->fts_info == FTS_DNR) {
			if (!rmdir(p->fts_accpath))
				continue;
			if (errno == ENOENT) {
				if (fflag)
					continue;
			} else if (p->fts_info != FTS_DP)
				(void)fprintf(stderr,
				    "rm: unable to read %s.\n", p->fts_path);
		} else if (!unlink(p->fts_accpath) || fflag && errno == ENOENT)
			continue;
		error(p->fts_path, errno);
	}
}

void
#if __STDC__
rmfile (char **argv)
#else
rmfile(argv)
	char **argv;
#endif
{
	register int df;
	register char *f;
	struct stat sb;

	df = dflag;
	/*
	 * Remove a file.  POSIX 1003.2 states that, by default, attempting
	 * to remove a directory is an error, so must always stat the file.
	 */
	while ((f = *argv++) != NULL) {
		/* Assume if can't stat the file, can't unlink it. */
#if _POSIX_SOURCE
		if (stat(f, &sb)) {
#else
		if (lstat(f, &sb)) {
#endif
			if (!fflag || errno != ENOENT)
				error(f, errno);
			continue;
		}
		if (S_ISDIR(sb.st_mode) && !df) {
			(void)fprintf(stderr, "rm: %s: is a directory\n", f);
			retval = EXIT_FAILURE;
			continue;
		}
		if (!fflag && !check(f, f, &sb))
			continue;
		if ((S_ISDIR(sb.st_mode) ? rmdir(f) : unlink(f)) &&
		    (!fflag || errno != ENOENT))
			error(f, errno);
	}
}

int
#if __STDC__
check (char *path, char *name, struct stat *sp)
#else
check(path, name, sp)
	char *path, *name;
	struct stat *sp;
#endif
{
	register int first, ch;
#if WIN_NT
	char modep[15];
#else
	char modep[15], *user_from_uid(), *group_from_gid();
#endif

	/* Check -i first. */
	if (iflag)
		(void)fprintf(stderr, "remove %s? ", path);
	else {
		/*
		 * If it's not a symbolic link and it's unwritable and we're
		 * talking to a terminal, ask.  Symbolic links are excluded
		 * because their permissions are meaningless.
		 */

#ifdef _POSIX_SOURCE /* DF_MSS */
                if (!stdin_ok || !access(name, W_OK))
#else
                if (S_ISLNK(sp->st_mode) || !stdin_ok || !access(name, W_OK))
#endif
                return(1);

		strmode(sp->st_mode, modep);
		(void)fprintf(stderr, "override %s%s%s/%s for %s? ",
		    modep + 1, modep[9] == ' ' ? "" : " ",
		    user_from_uid(sp->st_uid, 0),
		    group_from_gid(sp->st_gid, 0), path);

	}
	(void)fflush(stderr);

	first = ch = getchar();
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	return(first == 'y');
}

#define ISDOT(a)	((a)[0] == '.' && (!(a)[1] || (a)[1] == '.' && !(a)[2]))
void
#if __STDC__
checkdot (char **argv)
#else
checkdot(argv)
	char **argv;
#endif
{
	register char *p, **t, **save;
	int complained;

	complained = 0;
	for (t = argv; *t;) {
		if ((p = rindex(*t, '/')) != NULL)
			++p;
		else
			p = *t;
		if (ISDOT(p)) {
			if (!complained++)
			    (void)fprintf(stderr,
				"rm: \".\" and \"..\" may not be removed.\n");
			retval = EXIT_FAILURE;
			for (save = t; (t[0] = t[1]) != NULL; ++t);
			t = save;
		} else
			++t;
	}
}

void
#if __STDC__
error (char *name, int val)
#else
error(name, val)
	char *name;
	int val;
#endif
{
	(void)fprintf(stderr, "rm: %s: %s.\n", name, strerror(val));
	retval = EXIT_FAILURE;
}

void
#if __STDC__
usage (void)
#else
usage()
#endif
{
	(void)fprintf(stderr, "usage: rm [-dfiRr] file ...\n");
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	exit(EXIT_FAILURE);
}
