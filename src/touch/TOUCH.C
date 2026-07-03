/*
 * Copyright (c) 1988 Regents of the University of California.
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
char copyright[] =
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)touch.c	4.8 (Berkeley) 6/1/90";
#endif /* not lint */

/*
 * Attempt to set the modify date of a file to the current date.  If the
 * file exists, read and write its first character.  If the file doesn't
 * exist, create it, unless -c option prevents it.  If the file is read-only,
 * -f forces chmod'ing and touch'ing.
 */
#include <sys/types.h>
#ifdef _POSIX_SOURCE
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#else
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <stdio.h>
#if WIN_NT
#include <sys/cdefs.h>
#include <misc.h>
#include <bsdlib.h>

extern int globulate __P((int, int, char **));
extern void deglobulate __P((void));
extern int globulated_argc;
extern char **globulated_argv;
pid_t ppid;
int globulation;
#endif

static int	dontcreate;	/* set if -c option */
static int	force;		/* set if -f option */

int touch __P((char *));
int readwrite __P((char *, off_t));
void usage __P((void));

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
	int ch, retval;


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
	dontcreate = force = retval = 0;
	while ((ch = getopt(argc, argv, "cf")) != EOF)
		switch((char)ch) {
		case 'c':
			dontcreate = 1;
			break;
		case 'f':
			force = 1;
			break;
		case '?':
		default:
			usage();
		}
	if (!*(argv += optind))
		usage();
	do {
		retval |= touch(*argv);
	} while (*++argv);
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	return retval;
}

int
#if __STDC__
touch (char *filename)
#else
touch(filename)
	char *filename;
#endif
{
	struct stat statbuffer;

	if (stat(filename, &statbuffer) == -1) {
		if (!dontcreate)
			return(readwrite(filename, 0L));
/* 
   Marked off for option 'c' to work according
   to BSD man pages
*/
#if 0
		fprintf(stderr, "touch: %s: does not exist\n", filename);
#endif
		return(EXIT_FAILURE);
	}
	if ((statbuffer.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, "touch: %s: can only touch regular files\n",
		    filename);
		return(EXIT_FAILURE);
	}
	if (!access(filename, R_OK | W_OK)) {
		return(readwrite(filename,statbuffer.st_size));
	}

	if (force) {
		int retval;

		if (chmod(filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) {
			fprintf(stderr, "touch: %s: couldn't chmod: ",
			    filename);
			perror((char *)NULL);
			return(EXIT_FAILURE);
		}
		retval = readwrite(filename, statbuffer.st_size);
		if (chmod(filename, statbuffer.st_mode)) {
			fprintf(stderr, "touch: %s: couldn't chmod back: ",
			    filename);
			perror((char *)NULL);
			return(EXIT_FAILURE);
		}
		return(retval);
	}
	fprintf(stderr, "touch: %s: cannot touch\n", filename);
	return(EXIT_FAILURE);
}

int
#if __STDC__
readwrite (char *filename, off_t size)
#else
readwrite(filename, size)
	char *filename;
	off_t size;
#endif
{
	int filedescriptor;
	char first;
#if !_POSIX_SOURCE
	off_t lseek();
#endif

	if (size) {
		filedescriptor = open(filename, O_RDWR, 0);
		if (filedescriptor == -1) {
error:			fprintf(stderr, "touch: %s: ", filename);
			perror((char *)NULL);
			return(EXIT_FAILURE);
		}
		if (read(filedescriptor, &first, 1) != 1)
			goto error;
#ifdef _POSIX_SOURCE
		if (lseek(filedescriptor, (off_t) 0, SEEK_SET) == (off_t) -1)
#else
		if (lseek(filedescriptor, 0L, 0) == -1)
#endif
                            goto error;
		if (write(filedescriptor, &first, 1) != 1)
			goto error;
	} else {
		filedescriptor = creat(filename, 0666);
		if (filedescriptor == -1)
			goto error;
	}
	if (close(filedescriptor) == -1)
		goto error;
	return(EXIT_SUCCESS);
}

void
#if __STDC__
usage (void)
#else
usage()
#endif
{
	fprintf(stderr, "usage: touch [-cf] file ...\n");
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	exit(EXIT_FAILURE);
}
