/* ANSI common definitions */

/* $Header$ */

#ifndef NULL
#if __STDC__
#define	NULL	(void*)0
#else
#define	NULL	0
#endif
#endif

#if ! _STDDEF_H
#define	_STDDEF_H 1

/* doesn't really belong here, but the library function need it */
#if __STDC__
#define	ARGS(args)	args
#define Void	void
#define	Const	const
#else
#define	ARGS(args)	()
#define	Void	char
#define	Const
#endif

#ifdef sparc
# include <sys/stdtypes.h>
#else
typedef unsigned size_t;		/* may need long */
typedef int ptrdiff_t;
#endif /* sparc */

/* 16-bit to match the MSVC compiler's L"..." literals (C mode has no
 * built-in wchar_t); the X11R6 libX11 port relies on this being here. */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#define	offsetof(type,id) ((size_t)&((type*)NULL)->id)

extern	int errno;		/* really belongs in <errno.h> */

#endif

