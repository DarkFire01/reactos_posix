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
#endif

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ls.c	5.48 (Berkeley) 4/3/91";
#endif /* not lint */

#ifdef DF_POSIX /* DF_DSC */
#include <sys/cdefs.h>
#else
#include <sys/param.h>
#endif
#include <sys/stat.h>
#ifdef _POSIX_SOURCE /* DF_DSC */
#include <stdlib.h>
#include <unistd.h>
#else
#include <sys/ioctl.h>
#endif
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "ls.h"

int (*sortfcn) __P((LS *, LS *));
void (*printfcn) __P((LS *, register int));
#ifndef _POSIX_SOURCE
int lstat();
#endif
extern char *emalloc __P((u_int));
#if WIN_NT
extern int acccmp __P((LS *, LS *)), modcmp __P((LS *, LS *));
extern int namecmp __P((LS *, LS *));
extern void prcopy __P((register char *, register char *, int));
extern void printcol __P((LS *, register int));
extern void printlong __P((LS *, register int));
extern void printscol __P((register LS *, register int));
extern int revacccmp __P((LS *, LS *)), revmodcmp __P((LS *, LS *));
extern int revnamecmp __P((LS *, LS *)), revstatcmp __P((LS *, LS *));
extern int statcmp __P((LS *, LS *));
extern void nomem __P((void));
extern void usage __P((void));
extern int globulate __P((int, int, char **));
extern void deglobulate __P((void));
extern int globulated_argc;
extern char **globulated_argv;
pid_t ppid;
int globulation;
#endif

int termwidth = 80;		/* default terminal width */

/* flags */
int f_accesstime;		/* use time of last access */
int f_column;			/* columnated format */
int f_group;			/* show group ownership of a file */
int f_ignorelink;		/* indirect through symbolic link operands */
int f_inode;			/* print inode */
int f_kblocks;			/* print size in kilobytes */
int f_listalldot;		/* list . and .. as well */
int f_listdir;			/* list actual directory, not contents */
int f_listdot;			/* list files beginning with . */
int f_longform;			/* long listing format */
int f_needstat;			/* if need to stat files */
int f_newline;			/* if precede with newline */
int f_nonprint;			/* show unprintables as ? */
int f_nosort;			/* don't sort output */
int f_recursive;		/* ls subdirectories also */
int f_reversesort;		/* reverse whatever sort is used */
int f_sectime;			/* print the real time for all files */
int f_singlecol;		/* use single column output */
int f_size;			/* list size in short listing */
int f_statustime;		/* use time of last mode change */
int f_dirname;			/* if precede with directory name */
int f_timesort;			/* sort by time vice name */
int f_total;			/* if precede with "total" line */
int f_type;			/* add type character for non-regular files */

#if _POSIX_SOURCE
int (*statfcn) __P((const char *, struct stat *));
#else
int (*statfcn)(), stat(), lstat();
#endif

#if WIN_NT
void doargs __P((int, char **));
void displaydir __P((LS *, register int));
void subdir __P((LS *));
int tabdir __P((LS *, LS **, char **));
#endif

int
#if __STDC__
main (int argc, char **argv)
#else
main(argc, argv)
	int argc;
	char **argv;
#endif
{
#ifdef _POSIX_SOURCE /* DF_DSC */
	extern int optind;
#else
	extern int optind, stat();
	struct winsize win;
#endif

	int ch;
#if !WIN_NT
	char *p, *getenv();
	int acccmp(), modcmp(), namecmp(), prcopy(), printcol();
	int printlong(), printscol(), revacccmp(), revmodcmp(), revnamecmp();
	int revstatcmp(), statcmp();
#endif

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
	/* terminal defaults to -Cq, non-terminal defaults to -1 */
	if (isatty(1)) {
		f_nonprint = 1;
#ifndef _POSIX_SOURCE /* DF_MSS */
		if (ioctl(1, TIOCGWINSZ, &win) == -1 || !win.ws_col) {
			if (p = getenv("COLUMNS"))
				termwidth = atoi(p);
		}
		else
			termwidth = win.ws_col;
#endif
		f_column = 1;
	} else
		f_singlecol = 1;

	/* root is -A automatically */
	if (!getuid())
		f_listdot = 1;

#if _POSIX_SOURCE
	while ((ch = getopt(argc, argv, "1ACFLRTacdfiklqrstu")) != EOF) {
#else
	while ((ch = getopt(argc, argv, "1ACFLRTacdfgiklqrstu")) != EOF) {
#endif
		switch (ch) {
		/*
		 * -1, -C and -l all override each other
		 * so shell aliasing works right
		 */
		case '1':
			f_singlecol = 1;
			f_column = f_longform = 0;
#if _POSIX_SOURCE
			f_group = 0;
#endif
			break;
		case 'C':
			f_column = 1;
			f_longform = f_singlecol = 0;
#if _POSIX_SOURCE
			f_group = 0;
#endif
			break;
		case 'l':
			f_longform = 1;
#if _POSIX_SOURCE
			f_group = 1;
#endif
			f_column = f_singlecol = 0;
			break;
		/* -c and -u override each other */
		case 'c':
			f_statustime = 1;
			f_accesstime = 0;
			break;
		case 'u':
			f_accesstime = 1;
			f_statustime = 0;
			break;
		case 'F':
			f_type = 1;
			break;
		case 'L':
			f_ignorelink = 1;
			break;
		case 'R':
			f_recursive = 1;
			break;
		case 'a':
			f_listalldot = 1;
			/* FALLTHROUGH */
		case 'A':
			f_listdot = 1;
			break;
		case 'd':
			f_listdir = 1;
			break;
		case 'f':
			f_nosort = 1;
			break;
#if !_POSIX_SOURCE
		case 'g':
			f_group = 1;
			break;
#endif
		case 'i':
			f_inode = 1;
			break;
		case 'k':
			f_kblocks = 1;
			break;
		case 'q':
			f_nonprint = 1;
			break;
		case 'r':
			f_reversesort = 1;
			break;
		case 's':
			f_size = 1;
			break;
		case 'T':
			f_sectime = 1;
			break;
		case 't':
			f_timesort = 1;
			break;
		default:
		case '?':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* -d turns off -R */
	if (f_listdir)
		f_recursive = 0;

	/* if need to stat files */
	f_needstat = f_longform || f_recursive || f_timesort ||
	    f_size || f_type;

	/* select a sort function */
	if (f_reversesort) {
		if (!f_timesort)
			sortfcn = revnamecmp;
		else if (f_accesstime)
			sortfcn = revacccmp;
		else if (f_statustime)
			sortfcn = revstatcmp;
		else /* use modification time */
			sortfcn = revmodcmp;
	} else {
		if (!f_timesort)
			sortfcn = namecmp;
		else if (f_accesstime)
			sortfcn = acccmp;
		else if (f_statustime)
			sortfcn = statcmp;
		else /* use modification time */
			sortfcn = modcmp;
	}

	/* select a print function */
	if (f_singlecol)
		printfcn = printscol;
	else if (f_longform)
		printfcn = printlong;
	else
		printfcn = printcol;

#if _POSIX_SOURCE
	statfcn = stat;
#else
	/* if -l, -d or -F, and not ignoring the link, use lstat() */
	statfcn =
	    (f_longform || f_listdir || f_type) && !f_ignorelink ? lstat : stat;
#endif

	if (!argc) {
		static char dot[] = ".";

		argc = 1;
		argv[0] = dot;
		argv[1] = NULL;
	}
	doargs(argc, argv);

#ifndef _POSIX_SOURCE /* DF_MSS: fclosing not working in POSIX */
	fflush (stderr);
	fflush (stdout);
	fclose (stderr);
	fclose (stdout);
	fclose (stdin);
#endif

#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	return EXIT_SUCCESS;
}

static char path[MAXPATHLEN + 1];
static char *endofpath = path;

void
#if __STDC__
doargs (int argc, char **argv)
#else
doargs(argc, argv)
	int argc;
	char **argv;
#endif
{
	register LS *dstatp, *rstatp;
	register int cnt, dircnt, maxlen, regcnt;
	LS *dstats, *rstats;
	struct stat sb;
	char top[MAXPATHLEN + 1];
	u_long blocks;

	/*
	 * walk through the operands, building separate arrays of LS
	 * structures for directory and non-directory files.
	 */
	dstats = rstats = NULL;
	for (dircnt = regcnt = 0; *argv; ++argv) {
#if BETA
printf("doargs - before stat; *argv: \"%s\"\n", *argv);
#endif
#if _POSIX_SOURCE
		if (statfcn(*argv, &sb)) {
# if BETA
printf("doargs - after non-zero stat\n");
# endif
#else
		if (statfcn(*argv, &sb) &&
		    (statfcn == lstat || lstat(*argv, &sb))) {
#endif
			(void)fprintf(stderr,
			    "ls: %s: %s\n", *argv, strerror(errno));
			if (errno == ENOENT)
				continue;
#if WIN_NT
			if (ppid == (pid_t) 1 && globulation == 0)
				deglobulate();
#endif
			exit(EXIT_FAILURE);
		}
#if BETA
printf("*argv: \"%s\"; S_ISDIR: %d; sb.st_size: %ld\n",
	*argv, S_ISDIR(sb.st_mode), (long) sb.st_size);
#endif
#if DF_POSIX
		if (S_ISDIR(sb.st_mode))
			sb.st_size = (off_t) 0;
#endif
		if (S_ISDIR(sb.st_mode) && !f_listdir) {
			if (!dstats)
				dstatp = dstats = (LS *)emalloc((u_int)argc *
				    (sizeof(LS)));
			dstatp->name = *argv;
			dstatp->lstat = sb;
			++dstatp;
			++dircnt;
		}
		else {
			if (!rstats) {
				rstatp = rstats = (LS *)emalloc((u_int)argc *
				    (sizeof(LS)));
				blocks = 0;
				maxlen = -1;
			}
			rstatp->name = *argv;
			rstatp->lstat = sb;

			/* save name length for -C format */
			rstatp->len = strlen(*argv);

			if (f_nonprint)
				prcopy(*argv, *argv, rstatp->len);

			/* calculate number of blocks if -l/-s formats */
			if (f_longform || f_size)
#ifdef _POSIX_SOURCE /* DF_MSS */
				blocks += (sb.st_size/BLOCK_SIZE) + 1;
#else
				blocks += sb.st_blocks;
#endif

			/* save max length if -C format */
			if (f_column && maxlen < rstatp->len)
				maxlen = rstatp->len;

			++rstatp;
			++regcnt;
		}
	}
	/* display regular files */
	if (regcnt) {
#ifndef _POSIX_SOURCE /* DF_MSS */
		rstats[0].lstat.st_btotal = blocks;
		rstats[0].lstat.st_maxlen = maxlen;
#endif 

		displaydir(rstats, regcnt);
		f_newline = f_dirname = 1;
	}
	/* display directories */
	if (dircnt) {
		register char *p;

		f_total = 1;
		if (dircnt > 1) {
#if _POSIX_SOURCE
			(void)getcwd(top, sizeof top - 1);
#else
			(void)getwd(top);
#endif
#if __STDC__
			qsort((void *)dstats, (size_t)dircnt, sizeof(LS), (int (*)(const void *, const void *))sortfcn);
#else
			qsort((char *)dstats, dircnt, sizeof(LS), sortfcn);
#endif
			f_dirname = 1;
		}
		for (cnt = 0; cnt < dircnt; ++dstats) {
			for (endofpath = path, p = dstats->name;
			    (*endofpath = *p++) != '\0'; ++endofpath);
#if BETA
puts("before subdir(dstats)");
#endif
			subdir(dstats);
			f_newline = 1;
			if (++cnt < dircnt && chdir(top)) {
				(void)fprintf(stderr, "ls: %s: %s\n",
				    top, strerror(errno));
#if WIN_NT
				if (ppid == (pid_t) 1 && globulation == 0)
					deglobulate();
#endif
				exit(EXIT_FAILURE);
			}
		}
	}
}

void
#if __STDC__
displaydir (LS *stats, register int num)
#else
displaydir(stats, num)
	LS *stats;
	register int num;
#endif
{
	register char *p, *savedpath;
	LS *lp;

	if (num > 1 && !f_nosort) {
#ifndef _POSIX_SOURCE /* DF_MSS */
		u_long save1, save2;

		save1 = stats[0].lstat.st_btotal;
		save2 = stats[0].lstat.st_maxlen;
#endif
#if __STDC__
		qsort((void *)stats, (size_t)num, sizeof(LS), (int (*)(const void *, const void *))sortfcn);
#else
		qsort((char *)stats, num, sizeof(LS), sortfcn);
#endif
#ifndef _POSIX_SOURCE /* DF_MSS */
		stats[0].lstat.st_btotal = save1;
		stats[0].lstat.st_maxlen = save2;
#endif
	}
	printfcn(stats, num);

	if (f_recursive) {
		savedpath = endofpath;
		for (lp = stats; num--; ++lp) {
			if (!S_ISDIR(lp->lstat.st_mode))
				continue;
			p = lp->name;
			if (p[0] == '.' && (!p[1] || p[1] == '.' && !p[2]))
				continue;
			if (endofpath != path && endofpath[-1] != '/')
				*endofpath++ = '/';
			for (; (*endofpath = *p++) != '\0'; ++endofpath);
			f_newline = f_dirname = f_total = 1;
#if BETA
puts("before subdir(lp)");
#endif
			subdir(lp);
			*(endofpath = savedpath) = '\0';
		}
	}
}

void
#if __STDC__
subdir (LS *lp)
#else
subdir(lp)
	LS *lp;
#endif
{
	LS *stats;
	int num;
	char *names;

	if (f_newline)
		(void)putchar('\n');
	if (f_dirname)
		(void)printf("%s:\n", path);

	if (chdir(lp->name)) {
		(void)fprintf(stderr, "ls: %s: %s\n", lp->name,
		     strerror(errno));
		return;
	}
	if ((num = tabdir(lp, &stats, &names)) != 0) {
		displaydir(stats, num);
		(void)free((char *)stats);
		(void)free((char *)names);
	}
	if (chdir("..")) {
		(void)fprintf(stderr, "ls: ..: %s\n", strerror(errno));
#if WIN_NT
		if (ppid == (pid_t) 1 && globulation == 0)
			deglobulate();
#endif
		exit(EXIT_FAILURE);
	}
}

int
#if __STDC__
tabdir (LS *lp, LS **s_stats, char **s_names)
#else
tabdir(lp, s_stats, s_names)
	LS *lp, **s_stats;
	char **s_names;
#endif
{
	register DIR *dirp;
	register int cnt, maxentry, maxlen;
	register char *p, *names;
	struct dirent *dp;
	u_long blocks;
	LS *stats;

	if (!(dirp = opendir("."))) {
		(void)fprintf(stderr, "ls: %s: %s\n", lp->name,
		    strerror(errno));
		return(0);
	}
#if BETA
printf("a opendir; f_needstat: %d\n", f_needstat);
#endif
	blocks = maxentry = maxlen = 0;
	stats = NULL;
	for (cnt = 0; (dp = readdir(dirp)) != NULL;) {
		/* this does -A and -a */
		p = dp->d_name;
#if BETA
printf("a readdir; file: \"%s\"\n", p);
#endif
		if (p[0] == '.') {
			if (!f_listdot)
				continue;
			if (!f_listalldot && (!p[1] || p[1] == '.' && !p[2]))
				continue;
		}
#if BETA
puts("after dot check");
#endif
		if (cnt == maxentry) {
			if (!maxentry) {
#if BETA
int err;
#endif
#ifdef _POSIX_SOURCE /* DF_MSS */
				if( lp->lstat.st_size == (off_t) 0 )
					lp->lstat.st_size = (off_t) 7000;
#if BETA
printf("st_size: %u\n", (u_int)lp->lstat.st_size);
#endif
#endif
				*s_names = names =
				    emalloc((u_int)lp->lstat.st_size);
#if BETA
err = errno;
if (names == NULL) {
	printf("errno: %d\n", err);
	nomem();
}
#endif
			}
#define	DEFNUM	256
			maxentry += DEFNUM;
			if (!(*s_stats = stats = (LS *)realloc((char *)stats,
			    (u_int)maxentry * sizeof(LS))))
				nomem();
		}
#if BETA
puts("after maxentry check");
#endif
		if (f_needstat && statfcn(dp->d_name, &stats[cnt].lstat) &&
#if _POSIX_SOURCE
		    statfcn == stat ) {
#else
		    statfcn == stat && lstat(dp->d_name, &stats[cnt].lstat)) {
#endif
			/*
			 * don't exit -- this could be an NFS mount that has
			 * gone away.  Flush stdout so the messages line up.
			 */
			(void)fflush(stdout);
			(void)fprintf(stderr,
			    "ls: %s: %s\n", dp->d_name, strerror(errno));
			continue;
		}
		stats[cnt].name = names;
#if BETA
printf("after f_needstat check; dp->d_name: \"%s\"; names: %p; len: %u\n",
	dp->d_name, names, (unsigned int) strlen(dp->d_name));
#endif

		if (f_nonprint)
#ifdef _POSIX_SOURCE /* DF_MSS */
			prcopy(dp->d_name, names, (int)strlen(dp->d_name));
		else
			bcopy(dp->d_name, names, (int)strlen(dp->d_name));
#if BETA
puts("after nonprint check");
#endif
		names += strlen(dp->d_name);
#else
			prcopy(dp->d_name, names, (int)dp->d_namlen);
		else
			bcopy(dp->d_name, names, (int)dp->d_namlen);
		names += dp->d_namlen;
#endif
		*names++ = '\0';
#if BETA
puts("after names mucking");
#endif

		/*
		 * get the inode from the directory, so the -f flag
		 * works right.
		 */
#ifndef _POSIX_SOURCE /* DF_MSS */
		stats[cnt].lstat.st_ino = dp->d_ino;
#endif

		/* save name length for -C format */
#ifdef _POSIX_SOURCE /* DF_MSS */
                stats[cnt].len = strlen(dp->d_name);
#else
                stats[cnt].len = dp->d_namlen;
#endif

		/* calculate number of blocks if -l/-s formats */
		if (f_longform || f_size)
#ifdef _POSIX_SOURCE /* DF_MSS: POSIX does not support st_blocks defaulting to 10 */
			blocks += (stats[cnt].lstat.st_size/BLOCK_SIZE) + 1;
#else
			blocks += stats[cnt].lstat.st_blocks;
#endif
		/* save max length if -C format */
#ifdef _POSIX_SOURCE /* DF_MSS */
		if (f_column && maxlen < (int)strlen(dp->d_name))
			maxlen = strlen(dp->d_name);
#else
		if (f_column && maxlen < (int)dp->d_namlen)
			maxlen = dp->d_namlen;
#endif
		++cnt;
#if BETA
puts("end-o-loop");
#endif
	}
	(void)closedir(dirp);

	if (cnt) {
#ifndef _POSIX_SOURCE /* DF_MSS */
		stats[0].lstat.st_btotal = blocks;
		stats[0].lstat.st_maxlen = maxlen;
#endif
	} else if (stats) {

		(void)free((char *)stats);
		(void)free((char *)names);
	}
	return(cnt);
}
