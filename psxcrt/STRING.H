/* ANSI string handling (missing wide char stuff) */

#if ! _STRING_H
#define _STRING_H 1

#include <stddef.h>		/* define NULL and size_t */

#if __STDC__
#define	ARGS(args)	args
#define	Void	void
#else
#define	ARGS(args)	()
#define	Void	char
#endif

Void   *memcpy ARGS((Void *s1, const Void *s2, size_t));
Void   *memmove ARGS((Void *s1, const Void *s2, size_t));
int	memcmp ARGS((const Void *s1, const Void *s2, size_t));
Void   *memchr ARGS((const Void *s, int c, size_t));
Void   *memset ARGS((Void *s, int c, size_t));
size_t	strlen ARGS((const char *s));
char   *strcpy ARGS((char *s1, const char *s2));
char   *strncpy ARGS((char *s1, const char *s2, size_t));
char   *strcat ARGS((char *s1, const char *s2));
char   *strncat ARGS((char *s1, const char *s2, size_t));
int	strcmp ARGS((const char *s1, const char *s2));
int	strncmp ARGS((const char *s1, const char *s2, size_t));
char   *strchr ARGS((const char *s1, int c));
char   *strrchr ARGS((const char *s1, int c));
size_t	strspn ARGS((const char *s1, const char *s2));
size_t	strcspn ARGS((const char *s1, const char *s2));
char   *strpbrk ARGS((const char *s1, const char *s2));
char   *strstr ARGS((const char *s1, const char *s2));
char   *strtok ARGS((char *s1, const char *s2));
char   *strerror ARGS((int errno));

#endif /* _STRING_H */

