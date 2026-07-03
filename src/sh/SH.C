/*
	sh.c

	This file contains a motley group of functions used to let sh work in the POSIX subsystem on Windows NT.
*/

#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4214)

#include <windef.h>

#pragma warning(default:4214)
#pragma warning(default:4201)
#pragma warning(default:4001)

#define UL_GETFSIZE 1
#define UL_SETFSIZE 2

int remove (const char *filename)
{
	return unlink(filename);
}

int tcsetpgrp (int fildes, pid_t pgrp_id)
{
	if (fildes < 0 || fildes >= OPEN_MAX)
		errno = EBADF;
	else if (pgrp_id < (pid_t) 0)
		errno = EINVAL;
	else
		errno = ENOSYS;
	return -1;
}

long ulimit (int cmd, ...)
{
	static long limit = LONG_MAX;
	va_list va;
	long ret;

	va_start(va, cmd);
	if (cmd == UL_GETFSIZE)
		ret = limit;
	else if (cmd == UL_SETFSIZE)
	{
		ret = va_arg(va, long);
		if (ret <= limit || geteuid() == (uid_t) 0)
			limit = ret;
		else
		{
			errno = EPERM;
			ret = -1L;
		}
	}
	else
	{
		errno = EINVAL;
		ret = -1L;
	}
	va_end(va);
	return ret;
}

static int miniscule (int ch)
{
	return tolower(ch);
}

static int identity (int ch)
{
	return ch;
}

static int majuscule (int ch)
{
	return toupper(ch);
}

char *dos_to_posix (const char *dos_env_str, size_t offset, int mapping) /* mapping: <0 == tolower; 0 == none; >0 == toupper */
{
	static const char semicolon[] = ";";
	static char posix_pathname[PATH_MAX+1];
	char *copy, *p;
	const char *d;
	int (*map)(int);
	size_t s;

#if 0 && XN
	(void) fprintf(stderr, "dos_env_str: \"%s\"; offset: %lu; mapping: %d\n", dos_env_str, (unsigned long) offset, mapping);
	(void) fflush(stderr);
#endif
	if (dos_env_str == NULL || (copy = malloc(strlen(dos_env_str) + 1)) == NULL)
		p = NULL;
	else if (strcpy(copy, dos_env_str) == NULL)
	{
		free(copy);
		p = NULL;
	}
	else
	{
		p = posix_pathname;
		if (mapping < 0)
			map = miniscule;
		else if (mapping == 0)
			map = identity;
		else
			map = majuscule;
		for (s = 0; s < offset; ++s)
			*p++ = copy[s];
		for (d = strtok(copy + offset, semicolon); d != NULL; d = strtok(NULL, semicolon))
		{
			if (isalpha(*d) && d[1] == ':')
			{
				*p++ = '/';
				*p++ = '/';
				*p++ = (char) toupper(*d++);
				++d;
			}
			for (; *d != '\0'; ++d)
				*p++ = (char) ((*d == '\\') ? '/' : map(*d));
			*p++ = ':';
		}
/*
		Since the cwd is always in a DOS pathlist, it's assumed that
		if more than one DOS pathname is in the string, then it's a
		pathlist, and the cwd will be placed last on the list of POSIX
		pathnames (despite the fact that the cwd is always searched
		first in DOSland).
*/
		if (strchr(dos_env_str, ';') == NULL)
			--p;
		*p = '\0';
		free(copy);
		p = posix_pathname;
	}
#if 0 && XN
	(void) fprintf(stderr, "p: ");
	if (p == NULL)
		(void) fprintf(stderr, "NULL");
	else
		(void) fprintf(stderr, "\"%s\"", p);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
#endif
	return p;
}

char *posix_to_dos (const char *posix_env_str, size_t offset, int mapping) /* mapping: <0 == tolower; 0 == none; >0 == toupper */
{
	static const char colon[] = ":";
	static char dos_pathname[PATH_MAX+1];
	char *copy, *d;
	const char *p;
	int (*map)(int);
	size_t s;

#if 0 && XN
	(void) fprintf(stderr, "posix_env_str: \"%s\"; offset: %lu; mapping: %d\n",
		posix_env_str, (unsigned long) offset, mapping);
	(void) fflush(stderr);
#endif
	if (posix_env_str == NULL || (copy = malloc(strlen(posix_env_str) + 1)) == NULL)
		d = NULL;
	else if (strcpy(copy, posix_env_str) == NULL)
	{
		free(copy);
		d = NULL;
	}
	else
	{
		d = dos_pathname;
		if (mapping < 0)
			map = miniscule;
		else if (mapping == 0)
			map = identity;
		else
			map = majuscule;
		for (s = 0; s < offset; ++s)
			*d++ = copy[s];
		for (p = strtok(copy + offset, colon); p != NULL; p = strtok(NULL, colon))
		{
			if (*p == '/' && p[1] == '/' && isupper(p[2]))
			{
				++p;
				++p;
				*d++ = (char) map(*p++);
				*d++ = ':';
			}
			for (; *p != '\0'; ++p)
				*d++ = (char) ((*p == '/') ? '\\' : map(*p));
			*d++ = ';';
		}
		*--d = '\0';
		free(copy);
		d = dos_pathname;
	}
#if 0 && XN
	(void) fprintf(stderr, "d: ");
	if (d == NULL)
		(void) fprintf(stderr, "NULL");
	else
		(void) fprintf(stderr, "\"%s\"", d);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
#endif
	return d;
}

signed int isposix (const char *pathname)
{
	signed int ret, fd;
	IMAGE_DOS_HEADER dos_header;
	IMAGE_NT_HEADERS nt_headers;

	if (pathname == NULL || (fd = open(pathname, O_RDONLY)) == -1)
	{
		ret = 0;
	}
	else
	{
		if (read(fd, &dos_header, sizeof dos_header) != (ssize_t) sizeof dos_header
		|| dos_header.e_magic != IMAGE_DOS_SIGNATURE
		|| lseek(fd, (off_t) dos_header.e_lfanew, SEEK_SET) == (off_t) -1
		|| read(fd, &nt_headers, sizeof nt_headers) != (ssize_t) sizeof nt_headers
		|| nt_headers.FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER)
		{
			ret = 0;
		}
		else
		{
			ret = (nt_headers.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_POSIX_CUI);
		}
		(void) close(fd);
	}
	return ret;
}
#if 0

int tcdrain (int fildes)
{
	int ret;

	if (fildes < 0 || fildes >= OPEN_MAX)
	{
		ret = -1;
		errno = EBADF;
	}
	else if (!isatty(fildes))
	{
		ret = -1;
		errno = ENOTTY;
	}
	else
		ret = 0;
	return ret;
}
#endif /* Xn 1992-09-09 */
