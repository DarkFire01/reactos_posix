/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kevin Fall.
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


#ifdef DF_POSIX
#include <misc.h>
#include <bsdlib.h>
#endif
#include <sys/cdefs.h>

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)cat.c	5.15 (Berkeley) 5/23/91";
#endif /* not lint */

#ifdef _POSIX_SOURCE
#else
#include <sys/param.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int bflag, eflag, nflag, sflag, tflag, vflag;
int rval;
char *filename;

void cook_args __P((char **));
void cook_buf __P((register FILE *));
void raw_args __P((char **));
void raw_cat __P((register int));
void err __P((int, const char *, ...));

#if WIN_NT
extern int globulate __P((int, int, char **));
extern void deglobulate __P((void));
extern int globulated_argc;
extern char **globulated_argv;
pid_t ppid;
int globulation;
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
	extern int optind;
	int ch;
#if 0
ssize_t n;
int saved_errno;
static const char text[] = "moo goo gi pan";
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
#if 0
n = write(1, text, sizeof text - 1);
saved_errno = errno;
fprintf(stderr, "n: %d; errno: %d\n", n, saved_errno);
fflush(stderr);
#endif
	while ((ch = getopt(argc, argv, "benstuv")) != EOF)
		switch (ch) {
		case 'b':
			bflag = nflag = 1;	/* -b implies -n */
			break;
		case 'e':
			eflag = vflag = 1;	/* -e implies -v */
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 't':
			tflag = vflag = 1;	/* -t implies -v */
			break;
		case 'u':
			setbuf(stdout, (char *)NULL);
			break;
		case 'v':
			vflag = 1;
			break;
		case '?':
			(void)fprintf(stderr,
			    "usage: cat [-benstuv] [-] [file ...]\n");
#if WIN_NT
			if (ppid == (pid_t) 1 && globulation == 0)
				deglobulate();
#endif
			exit(EXIT_FAILURE);
		}
	argv += optind;

	if (bflag || eflag || nflag || sflag || tflag || vflag)
		cook_args(argv);
	else
		raw_args(argv);
#ifdef DF_POSIX /* DF_MSS */
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	return rval;
#else
        if (ch = fclose(stdout)) {
		fprintf(stdout, "ch X%dX\n", ch);
		fflush(stdout);
		err(EXIT_FAILURE, "stdout: %s", strerror(errno));
	}
	exit(rval);
#endif
}

void
#if __STDC__
cook_args (char **argv)
#else
cook_args(argv)
	char **argv;
#endif
{
	register FILE *fp;

	fp = stdin;
	filename = "stdin";
	do {
		if (*argv) {
			if (!strcmp(*argv, "-"))
				fp = stdin;
			else if (!(fp = fopen(*argv, "r"))) {
#if 0
fprintf(stderr, "err #1\n");
fflush(stderr);
#endif
				err(0, "%s: %s", *argv, strerror(errno));
				++argv;
				continue;
			}
			filename = *argv++;
		}
		cook_buf(fp);
		if (fp != stdin)
			(void)fclose(fp);
	} while (*argv);
}

void
#if __STDC__
cook_buf (register FILE *fp)
#else
cook_buf(fp)
	register FILE *fp;
#endif
{
	register int ch, gobble, line, prev;
#if WIN_NT
	register int c;
#endif

	line = gobble = 0;
	for (prev = '\n'; (ch = getc(fp)) != EOF; prev = ch) {
#if WIN_NT
		if ( ch == '\r' ) 
                	if (( c = getc(fp)) == '\n' )
			{
				if ( !gobble )
					putchar(ch);
				ch = c;
			}
			else
			{
				if ( c == EOF )
				{
					putchar(ch);
					break;
				}
				ungetc(c,fp);
			}
#endif
		if (prev == '\n') {
			if (ch == '\n') {
				if (sflag) {
					if (!gobble && putchar(ch) == EOF)
						break;
					gobble = 1;
					continue;
				}
				if (nflag && !bflag) {
					(void)fprintf(stdout, "%6d\t", ++line);
					if (ferror(stdout))
						break;
				}
			} else if (nflag) {
				(void)fprintf(stdout, "%6d\t", ++line);
				if (ferror(stdout))
					break;
			}
		}
		gobble = 0;
		if (ch == '\n') {
			if (eflag)
				if (putchar('$') == EOF)
					break;
		} else if (ch == '\t') {
			if (tflag) {
				if (putchar('^') == EOF || putchar('I') == EOF)
					break;
				continue;
			}
		} else if (vflag) {
			if (!isascii(ch)) {
				if (putchar('M') == EOF || putchar('-') == EOF)
					break;
				ch = toascii(ch);
			}
			if (iscntrl(ch)) {
				if (putchar('^') == EOF ||
				    putchar(ch == '\177' ? '?' :
				    ch | 0100) == EOF)
					break;
				continue;
			}
		}
		if (putchar(ch) == EOF)
			break;
	}
	if (ferror(fp)) {
#if 0
fprintf(stderr, "err #2\n");
fflush(stderr);
#endif
		err(0, "cat: %s", strerror(errno));
		clearerr(fp);
	}
	if (ferror(stdout)) {
#if 0
fprintf(stderr, "err #3\n");
fflush(stderr);
#endif
		err(EXIT_FAILURE, "stdout: %s", strerror(errno));
	}
}

void
#if __STDC__
raw_args (char **argv)
#else
raw_args(argv)
	char **argv;
#endif
{
	register int fd;

	fd = fileno(stdin);
	filename = "stdin";
	do {
		if (*argv) {
			if (!strcmp(*argv, "-"))
				fd = fileno(stdin);
			else if ((fd = open(*argv, O_RDONLY, 0)) < 0) {
#if 0
fprintf(stderr, "err #4\n");
fflush(stderr);
#endif
				err(0, "%s: %s", *argv, strerror(errno));
				++argv;
				continue;
			}
			filename = *argv++;
		}
		raw_cat(fd);
		if (fd != fileno(stdin))
			(void)close(fd);
	} while (*argv);
}

void
#if __STDC__
raw_cat (register int rfd)
#else
raw_cat(rfd)
	register int rfd;
#endif
{
	register int nr, nw, off, wfd;
	static int bsize;
	static char *buf;
	struct stat sbuf;

	wfd = fileno(stdout);
	if (!buf) {
		if (fstat(wfd, &sbuf)) {
#if 0
fprintf(stderr, "err #5\n");
fflush(stderr);
#endif
			err(EXIT_FAILURE, "%s: %s", filename, strerror(errno));
		}
#ifdef DF_POSIX  /* DF_MSS */
		bsize = __max(/*1024*/ 1024, 1024);
#else
		bsize = 1024;
#endif

		if (!(buf = malloc((u_int)bsize))) {
#if 0
fprintf(stderr, "err #6\n");
fflush(stderr);
#endif
			err(EXIT_FAILURE, "%s", strerror(errno));
		}
	}
	while ((nr = read(rfd, buf, bsize)) > 0) {
#if 0
fprintf(stderr, "read %d bytes: \"%.*s\"\n", nr, nr, buf);
fflush(stderr);
#endif
		for (off = 0; off < nr; nr -= nw, off += nw)
			if ((nw = write(wfd, buf + off, nr)) < 0) {
#if 0
int saved_errno;
saved_errno = errno;
fprintf(stderr, "err #7 - errno: %d; wfd: %d; isatty(wfd): %d\n", saved_errno, wfd, isatty(wfd));
fflush(stderr);
#endif
				err(EXIT_FAILURE, "stdout");
			}
	}
	if (nr < 0) {
#if 0
fprintf(stderr, "err #8\n");
fflush(stderr);
#endif
		err(0, "%s: %s", filename, strerror(errno));
	}
}

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
err (int ex, const char *fmt, ...)
#else
err(ex, fmt, va_alist)
	int ex;
	char *fmt;
        va_dcl
#endif
{
	va_list ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
#if 0
fprintf(stderr, "cat - err\n");
fflush(stderr);
#endif
	(void)fprintf(stderr, "cat: ");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	if (ex) {
#if WIN_NT
		if (ppid == (pid_t) 1 && globulation == 0)
			deglobulate();
#endif
		exit(EXIT_FAILURE);
	}
	rval = 1;
}
