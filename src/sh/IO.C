/*
 * shell buffered IO and formatted output
 */

static char *RCSid = "$Id: io.c,v 3.4 89/03/27 15:50:52 egisin Exp $";

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#if _POSIX_SOURCE
#include <sys/stat.h>
#endif /* Xn 1992-07-17 */
#if 0 && XN
#include <sys/stat.h>
#endif
#include "sh.h"

#if 1
extern void error ARGS((void)); /* in main.c */

#endif /* Xn 1992-07-20 */
#if 0
/* fputc with ^ escaping */
static void
#if __STDC__
fzotc(register int c, register FILE *f)
#else
fzotc(c, f)
	register int c;
	register FILE *f;
#endif /* Xn 1992-07-17 */
{
	if ((c&0x60) == 0) {		/* C0|C1 */
		putc((c&0x80) ? '$' : '^', f);
		putc((c&0x7F|0x40), f);
	} else if ((c&0x7F) == 0x7F) {	/* DEL */
		putc((c&0x80) ? '$' : '^', f);
		putc('?', f);
	} else
		putc(c, f);
}
#endif

/*
 * formatted output functions
 */

/* shellf(...); error() */
void
#if __STDC__
errorf(Const char *fmt, ...) {
#else
errorf(va_alist) va_dcl
{
	char *fmt;
#endif
	va_list va;

#if __STDC__
	va_start(va, fmt);
#else
	va_start(va);
	fmt = va_arg(va, char *);
#endif
	vfprintf(shlout, fmt, va);
	va_end(va);
	/*fflush(shlout);*/
	error();
}

/* printf to shlout (stderr) */
int
#if __STDC__
shellf(Const char *fmt, ...) {
#else
shellf(va_alist) va_dcl
{
	char *fmt;
#endif
	va_list va;

#if __STDC__
	va_start(va, fmt);
#else
	va_start(va);
	fmt = va_arg(va, char *);
#endif
	vfprintf(shlout, fmt, va);
	va_end(va);
	return 0;
}

/*
 * We have a stdio stream for any open shell file descriptors (0-9)
 */
FILE *	shf [NUFILE];		/* map shell fd to FILE * */

/* open stream for shell fd */
void
#if __STDC__
fopenshf(int fd)
#else
fopenshf(fd)
	int fd;
#endif /* Xn 1992-07-17 */
{
#if 0 && XN
	(void) fprintf(stderr, "fopenshf - fd: %d\n", fd);
	if (fd <= 2)
		(void) fprintf(stderr, "fopenshf - _iob[%d]._flag: %lX\n", fd, (unsigned long) _iob[fd]._flag);
	(void) fflush(stderr);
#endif
	if (shf[fd] != NULL)
		return;
	if (fd <= 2)
		_iob[fd]._flag = 0; /* re-use stdin, stdout, stderr */
	shf[fd] = fdopen(fd, "r+");
#if 0 && XN
	(void) fprintf(stderr, "fopenshf - shf[%d]: %p\n", fd, shf[fd]);
	(void) fflush(stderr);
#endif
	if (shf[fd] == NULL)
		return;
	setvbuf(shf[fd], (char*)NULL, _IOFBF, (size_t)BUFSIZ);
#if 0 && XN
	if (fd <= 2)
		(void) fprintf(stderr, "fopenshf - shf[%d]->_flag: %lX\n", fd, (unsigned long) shf[fd]->_flag);
	(void) fflush(stderr);
#endif
}

/* flush stream assoc with fd */
/* this must invalidate input and output buffers */
void
#if __STDC__
flushshf(int fd)
#else
flushshf(fd)
	int fd;
#endif /* Xn 1992-07-17 */
{
	if (shf[fd] != NULL) {
#if 0 && XN
		int ret;
#if 0 && XN
		struct stat st;
		int err;

#endif
		(void) fprintf(stderr, "flushshf - fd: %d\n", fd);
		if (fd <= 2)
			(void) fprintf(stderr, "flushshf - _iob[%d]._flag: %lX\n", fd, (unsigned long) _iob[fd]._flag);
		(void) fflush(stderr);
#endif
#if __STDC__
#if 0 && XN
		(void) fstat(fileno(shf[fd]), &st);
		(void) fprintf(stderr, "flushshf - shf[%d]'s st_mode: %#lo\n", fd, (unsigned long) st.st_mode);
		(void) fflush(stderr);
		ret = fseek(shf[fd], 0L, SEEK_CUR); /* V7 derived */
		err = errno;
		(void) fprintf(stderr, "flushshf - fseek: %d", ret);
		if (ret == -1)
			(void) fprintf(stderr, "; errno: %d", err);
		(void) fprintf(stderr, "\n");
		(void) fflush(stderr);
#else
		fseek(shf[fd], 0L, SEEK_CUR); /* V7 derived */
#endif
#else
		fseek(shf[fd], 0L, 1); /* V7 derived */
#endif /* Xn 1992-07-31 */
#if 0 && XN
		ret = fflush(shf[fd]);	/* standard C */
		(void) fprintf(stderr, "flushshf - fflush: %d\n", ret);
		(void) fflush(stderr);
#else
		fflush(shf[fd]);	/* standard C */
#endif
	}
}

/*
 * move fd from user space (0<=fd<10) to shell space (fd>=10)
 */
int
#if __STDC__
savefd(int fd)
#else
savefd(fd)
	int fd;
#endif /* Xn 1992-07-17 */
{
	int nfd;

#if 0 && XN
	(void) fprintf(stderr, "savefd - fd: %d\n", fd);
	(void) fflush(stderr);
#endif
	if (fd < FDBASE) {
		flushshf(fd);
		nfd = fcntl(fd, F_DUPFD, FDBASE);
		if (nfd < 0)
#if 0 && XN
		{
			(void) fprintf(stderr, "savefd - nfd: %d\n", nfd);
			(void) fflush(stderr);
#endif
			if (errno == EBADF)
				return -1;
			else
				errorf("too many files open in shell\n");
#if 0 && XN
		}
#endif
#if _POSIX_SOURCE
		(void) fcntl(nfd, F_SETFD, FD_CLOEXEC);
#else
		(void) fcntl(nfd, F_SETFD, FD_CLEXEC);
#endif /* Xn 1992-07-16 */
		close(fd);
	} else
		nfd = fd;
#if 0 && XN
	(void) fprintf(stderr, "savefd - nfd: %d; isatty(nfd): %d\n", nfd, isatty_kludge(nfd));
	(void) fflush(stderr);
#endif
	return nfd;
}

void
#if __STDC__
restfd(int fd, int ofd)
#else
restfd(fd, ofd)
	int fd, ofd;
#endif /* Xn 1992-07-17 */
{
	if (ofd == 0)		/* not saved (e.savefd) */
		return;
#if 0 && XN
	(void) fprintf(stderr, "restfd - fd: %d; ofd: %d\n", fd, ofd);
	(void) fflush(stderr);
#endif
	flushshf(fd);
	close(fd);
	if (ofd < 0)		/* original fd closed */
		return;
#if 0 && XN
{
	int fdflags, ret;

	ret = fcntl(ofd, F_DUPFD, fd);
	(void) fprintf(stderr, "restfd - fcntl(%d, F_DUPFD, %d): %d\n", ofd, fd, ret);
	(void) fflush(stderr);
	fdflags = fcntl(ret, F_GETFD);
	(void) fprintf(stderr, "restfd - fcntl(%d, F_GETFD): %d\n", ret, fdflags);
	(void) fflush(stderr);
}
#else
	(void) fcntl(ofd, F_DUPFD, fd);
#endif
	close(ofd);
}

void
#if __STDC__
openpipe(register int *pv)
#else
openpipe(pv)
	register int *pv;
#endif /* Xn 1992-07-17 */
{
	if (pipe(pv) < 0)
		errorf("can't create pipe - try again\n");
#if 0 && XN
	(void) fprintf(stderr, "openpipe - before savefd - pv: %d %d\n", pv[0], pv[1]);
	(void) fflush(stderr);
#endif
	pv[0] = savefd(pv[0]);
	pv[1] = savefd(pv[1]);
#if 0 && XN
	(void) fprintf(stderr, "openpipe - after savefd - pv: %d %d\n", pv[0], pv[1]);
	(void) fflush(stderr);
#endif
}

void
#if __STDC__
closepipe(register int *pv)
#else
closepipe(pv)
	register int *pv;
#endif /* Xn 1992-07-17 */
{
	close(pv[0]);
	close(pv[1]);
}

/*
 * temporary files
 */

struct temp *
#if __STDC__
maketemp(Area *ap)
#else
maketemp(ap)
	Area *ap;
#endif /* Xn 1992-07-17 */
{
	register struct temp *tp;
	static unsigned int inc = 0;
	char path [PATH];

#if WIN_NT
	sprintf(path, "/tmp/sh%06lu.%03u", (unsigned long)getpid()%1000000ul, inc++%1000u);
#else
	sprintf(path, "/tmp/sh%05u%02u", (unsigned)getpid(), inc++);
#endif /* Xn 1992-09-08 */
#if _POSIX_SOURCE
	close(creat(path, S_IRUSR|S_IWUSR));	/* to get appropriate permissions */
#else
#if defined(_SYSV) || defined(_BSD)
	close(creat(path, 0600));	/* to get appropriate permissions */
#endif
#endif /* Xn 1992-07-17 */
	tp = (struct temp *) alloc(sizeof(struct temp), ap);
	tp->next = NULL;
	tp->name = strsave(path, ap);
	return tp;
}
