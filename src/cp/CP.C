/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * David Hitz of Auspex Systems Inc.
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
"@(#) Copyright (c) 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)cp.c	5.24 (Berkeley) 5/6/91";
#endif /* not lint */

/*
 * cp copies source files to target files.
 * 
 * The global PATH_T structures "to" and "from" always contain paths to the
 * current source and target files, respectively.  Since cp does not change
 * directories, these paths can be either absolute or dot-realative.
 * 
 * The basic algorithm is to initialize "to" and "from", and then call the
 * recursive copy() function to do the actual work.  If "from" is a file,
 * copy copies the data.  If "from" is a directory, copy creates the
 * corresponding "to" directory, and calls itself recursively on all of
 * the entries in the "from" directory.
 */
#ifdef _POSIX_SOURCE
#else
#include <sys/param.h>
#endif
#include <sys/stat.h>
#ifdef _POSIX_SOURCE
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cp.h"

PATH_T from = { from.p_path, "" };
PATH_T to = { to.p_path, "" };

uid_t myuid;
int exit_val;
mode_t myumask;
int iflag, pflag, orflag, rflag;
int (*statfcn) __P((const char *, struct stat *));
char *buf, *progname;

void copy __P((void));
void copy_file __P((struct stat *, int));
void copy_dir __P((void));
void copy_link __P((int));
void copy_fifo __P((struct stat *, int));
void copy_special __P((struct stat *, int));
void setfile __P((register struct stat *, int));
void error __P((const char *));
void usage __P((void));

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
	struct stat to_stat;
	register int c, r;
#if WIN_NT
#else
	int symfollow, lstat(), stat();
#endif
	char *old_to, *p;

	/*
	 * The utility cp(1) is used by mv(1) -- except for usage statements,
	 * print the "called as" program name.
	 */
#if WIN_NT
	ppid = getppid();
	if (ppid == (pid_t) 1) /* if parent is CMD.EXE */
	{
		globulation = globulate(1, argc, argv);
		if (globulation == 0)
		{
			argc = globulated_argc;
			argv = globulated_argv;
# if 0
			for (c = 0; c < argc; ++c)
				(void) printf("[%s] ", argv[c]);
			(void) printf("NULL\n");
# endif
		}
	}
#endif
	progname = ((p = rindex(*argv,'/')) != NULL) ? ++p : *argv;

#if WIN_NT
#else
	symfollow = 0;
#endif
	while ((c = getopt(argc, argv, "Rfipr")) != EOF) {
	switch ((char)c) {
		case 'f':
			iflag = 0;
			break;
#if WIN_NT
#else
		case 'h':
			symfollow = 1;
			break;
#endif
		case 'i':
			iflag = isatty(fileno(stdin));
			break;
		case 'p':
			pflag = 1;
			break;
		case 'R':
			rflag = 1;
			break;
		case 'r':
			orflag = 1;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	if (rflag && orflag) {
		(void)fprintf(stderr,
		    "cp: the -R and -r options are mutually exclusive.\n");
#if WIN_NT
		if (ppid == (pid_t) 1 && globulation == 0)
			deglobulate();
#endif
		exit(EXIT_FAILURE);
	}

	buf = (char *)malloc(MAXBSIZE);
	if (!buf) {
		(void)fprintf(stderr, "%s: out of space.\n", progname);
#if WIN_NT
		if (ppid == (pid_t) 1 && globulation == 0)
			deglobulate();
#endif
		exit(EXIT_FAILURE);
	}

	myuid = getuid();

	/* copy the umask for explicit mode setting */
	myumask = umask(0);
	(void)umask(myumask);

	/* consume last argument first. */
	if (!path_set(&to, argv[--argc])) {
#if WIN_NT
		if (ppid == (pid_t) 1 && globulation == 0)
			deglobulate();
#endif
		exit(EXIT_FAILURE);
	}

#if WIN_NT
	statfcn = stat;
#else
	statfcn = symfollow || !rflag ? stat : lstat;
#endif

	/*
	 * Cp has two distinct cases:
	 *
	 * % cp [-rip] source target
	 * % cp [-rip] source1 ... directory
	 *
	 * In both cases, source can be either a file or a directory.
	 *
	 * In (1), the target becomes a copy of the source. That is, if the
	 * source is a file, the target will be a file, and likewise for
	 * directories.
	 *
	 * In (2), the real target is not directory, but "directory/source".
	 */

	r = stat(to.p_path, &to_stat);
	if (r == -1 && errno != ENOENT) {
#if 0
(void) fprintf(stderr, "#1\n");
#endif
		error(to.p_path);
#if WIN_NT
		if (ppid == (pid_t) 1 && globulation == 0)
			deglobulate();
#endif
		exit(EXIT_FAILURE);
	}
	if (r == -1 || !S_ISDIR(to_stat.st_mode)) {
		/*
		 * Case (1).  Target is not a directory.
		 */
		if (argc > 1)
			usage();
		if (!path_set(&from, *argv)) {
#if WIN_NT
			if (ppid == (pid_t) 1 && globulation == 0)
				deglobulate();
#endif
			exit(EXIT_FAILURE);
		}
		copy();
	}
	else {
		/*
		 * Case (2).  Target is a directory.
		 */
		for (;; ++argv) {
			if (!path_set(&from, *argv)) {
				exit_val = EXIT_FAILURE;
				continue;
			}
			old_to = path_append(&to, path_basename(&from), -1);
			if (!old_to) {
				exit_val = EXIT_FAILURE;
				continue;
			}
			copy();
			if (!--argc)
				break;
			path_restore(&to, old_to);
		}
	}
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	return exit_val;
}

/* copy file or directory at "from" to "to". */
void
#if __STDC__
copy (void)
#else
copy()
#endif
{
	struct stat from_stat, to_stat;
	int dne, statval;

	statval = statfcn(from.p_path, &from_stat);
	if (statval == -1) {
#if 0
(void) fprintf(stderr, "#2\n");
#endif
		error(from.p_path);
		return;
	}

	/* not an error, but need to remember it happened */
	if (stat(to.p_path, &to_stat) == -1)
		dne = 1;
	else {
		if (to_stat.st_dev == from_stat.st_dev &&
		    to_stat.st_ino == from_stat.st_ino) {
			(void)fprintf(stderr,
			    "%s: %s and %s are identical (not copied).\n",
			    progname, to.p_path, from.p_path);
			exit_val = EXIT_FAILURE;
			return;
		}
		dne = 0;
	}

	switch(from_stat.st_mode & S_IFMT) {
#ifndef _POSIX_SOURCE /* DF_MSS */
	case S_IFLNK:
		copy_link(!dne);
		return;
#endif
	case S_IFDIR:
		if (!rflag && !orflag) {
			(void)fprintf(stderr,
			    "%s: %s is a directory (not copied).\n",
			    progname, from.p_path);
			exit_val = EXIT_FAILURE;
			return;
		}
		if (dne) {
			/*
			 * If the directory doesn't exist, create the new
			 * one with the from file mode plus owner RWX bits,
			 * modified by the umask.  Trade-off between being
			 * able to write the directory (if from directory is
			 * 555) and not causing a permissions race.  If the
			 * umask blocks owner writes cp fails.
			 */
			if (mkdir(to.p_path, from_stat.st_mode|S_IRWXU) < 0) {
#if 0
(void) fprintf(stderr, "#3\n");
#endif
				error(to.p_path);
				return;
			}
		}
		else if (!S_ISDIR(to_stat.st_mode) != S_IFDIR) {
			(void)fprintf(stderr, "%s: %s: not a directory.\n",
			    progname, to.p_path);
			return;
		}
		copy_dir();
		/*
		 * If not -p and directory didn't exist, set it to be the
		 * same as the from directory, umodified by the umask;
		 * arguably wrong, but it's been that way forever.
		 */
		if (pflag)
			setfile(&from_stat, 0);
		else if (dne)
			(void)chmod(to.p_path, from_stat.st_mode);
		return;
	case S_IFCHR:
	case S_IFBLK:
		if (rflag) {
			copy_special(&from_stat, !dne);
			return;
		}
		break;
	case S_IFIFO:
		if (rflag) {
			copy_fifo(&from_stat, !dne);
			return;
		}
		break;
	}
	copy_file(&from_stat, dne);
}

void
#if __STDC__
copy_file (struct stat *fs, int dne)
#else
copy_file(fs, dne)
	struct stat *fs;
	int dne;
#endif
{
	register int from_fd, to_fd, rcount, wcount;
	struct stat to_stat;

	if ((from_fd = open(from.p_path, O_RDONLY, 0)) == -1) {
#if 0
(void) fprintf(stderr, "#4\n");
#endif
		error(from.p_path);
		return;
	}

	/*
	 * If the file exists and we're interactive, verify with the user.
	 * If the file DNE, set the mode to be the from file, minus setuid
	 * bits, modified by the umask; arguably wrong, but it makes copying
	 * executables work right and it's been that way forever.  (The
	 * other choice is 666 or'ed with the execute bits on the from file
	 * modified by the umask.)
	 */
#if 0
(void) fprintf(stderr, "dne: %d; to.p_path: \"%s\"; fs->st_mode: %lo\n",
	dne, to.p_path, (unsigned long) (fs->st_mode & ~(S_ISUID|S_ISGID)));
#endif
	if (!dne) {
		if (iflag) {
			int checkch, ch;

			(void)fprintf(stderr, "overwrite %s? ", to.p_path);
			checkch = ch = getchar();
			while (ch != '\n' && ch != EOF)
				ch = getchar();
			if (checkch != 'y') {
				(void)close(from_fd);
				return;
			}
		}
		to_fd = open(to.p_path, O_WRONLY|O_TRUNC, 0);
	} else
		to_fd = open(to.p_path, O_WRONLY|O_CREAT|O_TRUNC,
		    fs->st_mode & ~(S_ISUID|S_ISGID));

	if (to_fd == -1) {
#if 0
int saved_errno;

saved_errno = errno;
(void) fprintf(stderr, "#5; errno: %d\n", saved_errno);
#endif
		error(to.p_path);
		(void)close(from_fd);
		return;
	}

	while ((rcount = read(from_fd, buf, MAXBSIZE)) > 0) {
		wcount = write(to_fd, buf, rcount);
		if (rcount != wcount || wcount == -1) {
#if 0
(void) fprintf(stderr, "#6\n");
#endif
			error(to.p_path);
			break;
		}
	}
	if (rcount < 0) {
#if 0
(void) fprintf(stderr, "#7\n");
#endif
		error(from.p_path);
	}
	if (pflag)
		setfile(fs, to_fd);
	/*
	 * If the source was setuid or setgid, lose the bits unless the
	 * copy is owned by the same user and group.
	 */
	else if (fs->st_mode & (S_ISUID|S_ISGID) && fs->st_uid == myuid)
		if (fstat(to_fd, &to_stat)) {
#if 0
(void) fprintf(stderr, "#8\n");
#endif
			error(to.p_path);
		}
#ifdef _POSIX_SOURCE /* DF_MSS */
#define	RETAINBITS	(S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO)
#else
#define	RETAINBITS	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
#endif
#ifdef _POSIX_SOURCE /* DF_MSS */
		else if (fs->st_gid == to_stat.st_gid && chmod(to.p_path,
		    fs->st_mode & RETAINBITS & ~myumask)) {
#else
		else if (fs->st_gid == to_stat.st_gid && fchmod(to_fd,
		    fs->st_mode & RETAINBITS & ~myumask)) {
#endif
#if 0
(void) fprintf(stderr, "#9\n");
#endif
			error(to.p_path);
		}
	(void)close(from_fd);
	if (close(to_fd)) {
#if 0
(void) fprintf(stderr, "#10\n");
#endif
		error(to.p_path);
	}
}

void
#if __STDC__
copy_dir (void)
#else
copy_dir()
#endif
{
	struct stat from_stat;
	struct dirent *dp, **dir_list;
	register int dir_cnt, i;
	char *old_from, *old_to;

	dir_cnt = scandir(from.p_path, &dir_list, NULL, NULL);
	if (dir_cnt == -1) {
		(void)fprintf(stderr, "%s: can't read directory %s.\n",
		    progname, from.p_path);
		exit_val = EXIT_FAILURE;
	}

	/*
	 * Instead of handling directory entries in the order they appear
	 * on disk, do non-directory files before directory files.
	 * There are two reasons to do directories last.  The first is
	 * efficiency.  Files tend to be in the same cylinder group as
	 * their parent, whereas directories tend not to be.  Copying files
	 * all at once reduces seeking.  Second, deeply nested tree's
	 * could use up all the file descriptors if we didn't close one
	 * directory before recursivly starting on the next.
	 */
	/* copy files */
	for (i = 0; i < dir_cnt; ++i) {
		dp = dir_list[i];

#ifdef _POSIX_SOURCE /* DF_MSS */
                if (strlen(dp->d_name) <= 2 && dp->d_name[0] == '.'
#else
                if (dp->d_namlen <= 2 && dp->d_name[0] == '.'
#endif
		    && (dp->d_name[1] == '\0' || dp->d_name[1] == '.'))
			goto done;
#ifdef _POSIX_SOURCE /* DF_MSS */
		old_from = path_append(&from, dp->d_name, (int)strlen(dp->d_name));
#else
		old_from = path_append(&from, dp->d_name, (int)dp->d_namlen);
#endif
		if (!old_from) {
			exit_val = EXIT_FAILURE;
			goto done;
		}

		if (statfcn(from.p_path, &from_stat) < 0) {
#if 0
(void) fprintf(stderr, "#11\n");
#endif
			error(dp->d_name);
			path_restore(&from, old_from);
			goto done;
		}
		if (S_ISDIR(from_stat.st_mode)) {
			path_restore(&from, old_from);
			continue;
		}
		old_to = path_append(&to, dp->d_name, (int)strlen(dp->d_name));
		if (old_to) {
			copy();
			path_restore(&to, old_to);
		} else
			exit_val = EXIT_FAILURE;
		path_restore(&from, old_from);
done:		dir_list[i] = NULL;
		(void)free((void *)dp);
	}

	/* copy directories */
	for (i = 0; i < dir_cnt; ++i) {
		dp = dir_list[i];
		if (!dp)
			continue;
		old_from = path_append(&from, dp->d_name, (int) strlen(dp->d_name));
		if (!old_from) {
			exit_val = EXIT_FAILURE;
			(void)free((void *)dp);
			continue;
		}
		old_to = path_append(&to, dp->d_name, (int) strlen(dp->d_name));
		if (!old_to) {
			exit_val = EXIT_FAILURE;
			(void)free((void *)dp);
			path_restore(&from, old_from);
			continue;
		}
		copy();
		free((void *)dp);
		path_restore(&from, old_from);
		path_restore(&to, old_to);
	}
	free((void *)dir_list);
}

void
#if __STDC__
copy_link (int exists)
#else
copy_link(exists)
	int exists;
#endif
{
#ifdef _POSIX_SOURCE  /* DF_MSS */
	if (exists)
		;
#else
	int len;
	char link[MAXPATHLEN];

	if ((len = readlink(from.p_path, link, sizeof(link))) == -1) {
#if 0
(void) fprintf(stderr, "#12\n");
#endif
		error(from.p_path);
		return;
	}
	link[len] = '\0';
	if (exists && unlink(to.p_path)) {
#if 0
(void) fprintf(stderr, "#13\n");
#endif
		error(to.p_path);
		return;
	}
	if (symlink(link, to.p_path)) {
#if 0
(void) fprintf(stderr, "#14\n");
#endif
		error(link);
		return;
	}
#endif
}

void
#if __STDC__
copy_fifo (struct stat *from_stat, int exists)
#else
copy_fifo(from_stat, exists)
	struct stat *from_stat;
	int exists;
#endif
{
	if (exists && unlink(to.p_path)) {
#if 0
(void) fprintf(stderr, "#15\n");
#endif
		error(to.p_path);
		return;
	}
	if (mkfifo(to.p_path, from_stat->st_mode)) {
#if 0
(void) fprintf(stderr, "#16\n");
#endif
		error(to.p_path);
		return;
	}
	if (pflag)
		setfile(from_stat, 0);
}

void
#if __STDC__
copy_special (struct stat *from_stat, int exists)
#else
copy_special(from_stat, exists)
	struct stat *from_stat;
	int exists;
#endif
{
	if (exists && unlink(to.p_path)) {
#if 0
(void) fprintf(stderr, "#17\n");
#endif
		error(to.p_path);
		return;
	}
#ifdef _POSIX_SOURCE /* DF_MSS */
	if (mknod(to.p_path, from_stat->st_mode, 0)) {
#else
	if (mknod(to.p_path, from_stat->st_mode, from_stat->st_rdev)) {
#endif
#if 0
(void) fprintf(stderr, "#18\n");
#endif
		error(to.p_path);
		return;
	}
	if (pflag)
		setfile(from_stat, 0);
}

void
#if __STDC__
setfile (register struct stat *fs, int fd)
#else
setfile(fs, fd)
	register struct stat *fs;
	int fd;
#endif
{
	static struct timeval tv[2];
	char path[100];

#ifdef _POSIX_SOURCE /* DF_MSS */
	if (fd)
		;
#endif
	fs->st_mode &= S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO;

	tv[0].tv_sec = fs->st_atime;
	tv[1].tv_sec = fs->st_mtime;
	if (utimes(to.p_path, tv)) {
		(void)snprintf(path, sizeof(path), "utimes: %s", to.p_path);
#if 0
(void) fprintf(stderr, "#19\n");
#endif
		error(path);
	}
	/*
	 * Changing the ownership probably won't succeed, unless we're root
	 * or POSIX_CHOWN_RESTRICTED is not set.  Set uid/gid before setting
	 * the mode; current BSD behavior is to remove all setuid bits on
	 * chown.  If chown fails, lose setuid/setgid bits.
	 */
#ifdef _POSIX_SOURCE /* DF_MSS */
	if (chown(to.p_path, fs->st_uid, fs->st_gid)) {
#else
	if (fd ? fchown(fd, fs->st_uid, fs->st_gid) :
	    chown(to.p_path, fs->st_uid, fs->st_gid)) {
#endif
		if (errno != EPERM) {
			(void)snprintf(path, sizeof(path),
			    "chown: %s", to.p_path);
#if 0
(void) fprintf(stderr, "#20\n");
#endif
			error(path);
		}
		fs->st_mode &= ~(S_ISUID|S_ISGID);
	}
#ifdef _POSIX_SOURCE /* DF_MSS */
	if (chmod(to.p_path, fs->st_mode)) {
#else
	if (fd ? fchmod(fd, fs->st_mode) : chmod(to.p_path, fs->st_mode)) {
#endif
		(void)snprintf(path, sizeof(path), "chown: %s", to.p_path);
#if 0
(void) fprintf(stderr, "#21\n");
#endif
		error(path);
	}
}

void
#if __STDC__
error (const char *s)
#else
error(s)
	char *s;
#endif
{
	exit_val = EXIT_FAILURE;
	(void)fprintf(stderr, "%s: %s: %s\n", progname, s, strerror(errno));
}

void
#if __STDC__
usage (void)
#else
usage()
#endif
{
	(void)fprintf(stderr,
#if WIN_NT
"usage: cp [-Rfip] src target;\n   or: cp [-Rfip] src1 ... srcN directory\n");
#else
"usage: cp [-Rfip] src target;\n   or: cp [-Rfhip] src1 ... srcN directory\n");
#endif
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	exit(EXIT_FAILURE);
}
