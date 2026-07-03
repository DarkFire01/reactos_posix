/*
 * Miscellaneous functions
 */

static char *RCSid = "$Id: misc.c,v 3.2 89/03/27 15:51:44 egisin Exp $";

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#if 1 && XN
#include <stdio.h>
#endif /* Xn 1993-06-17 */
#include "sh.h"
#include "expand.h"

char ctypes [UCHAR_MAX+1];	/* type bits for unsigned char */

/*
 * Fast character classes
 */
void
#if __STDC__
setctypes(register Const char *s, register int t)
#else
setctypes(s, t)
	register Const char *s;
	register int t;
#endif /* Xn 1992-07-17 */
{
	register int i;

	if ((t&C_IFS)) {
		for (i = 0; i < UCHAR_MAX+1; i++)
			ctypes[i] &=~ C_IFS;
		ctypes[0] |= C_IFS; /* include \0 in C_IFS */
	}
	ctypes[(unsigned char) *s++] |= t;	/* allow leading \0 in string */
	while (*s != 0)
		ctypes[(unsigned char) *s++] |= t;
}

void
#if __STDC__
initctypes(void)
#else
initctypes()
#endif /* Xn 1992-07-17 */
{
	register int c;

	for (c = 'a'; c <= 'z'; c++)
		ctypes[c] |= C_ALPHA;
	for (c = 'A'; c <= 'Z'; c++)
		ctypes[c] |= C_ALPHA;
	ctypes['_'] |= C_ALPHA;
	setctypes("0123456789", C_DIGIT);
	setctypes("\0 \t\n|&;<>()", C_LEX1);
	setctypes("*@#!$-?", C_VAR1);
	setctypes("=-+?#%", C_SUBOP);
}

/* convert unsigned long to base N string */

char *
#if __STDC__
ulton(register unsigned long n, int base)
#else
ulton(n, base)
	register unsigned long n;
	int base;
#endif /* Xn 1992-07-17 */
{
	register char *p;
	static char buf [20];

	p = &buf[sizeof(buf)];
	*--p = '\0';
	do {
		*--p = "0123456789ABCDEF"[n%base];
		n /= base;
	} while (n != 0);
	return p;
}

char *
#if __STDC__
strsave(register char *s, Area *ap)
#else
strsave(s, ap)
	register char *s;
	Area *ap;
#endif /* Xn 1992-07-17 */
{
	return strcpy((char*) alloc((size_t)strlen(s)+1, ap), s);
}

static struct option {
	char *name;
	int flag;
} options[] = {
	{"allexport",	FEXPORT},
	{"bgnice",	FBGNICE},
#if defined(EDIT) && defined(EMACS)
	{"emacs",	FEMACS},
#endif
	{"errexit",	FERREXIT},
	{"hashall",	FHASHALL},
	{"ignoreeof",	FIGNEOF},
	{"interactive",	FTALKING},
	{"keyword",	FKEYWORD},
	{"markdirs",	FMARKDIRS},
	{"monitor",	FMONITOR},
	{"noexec",	FNOEXEC},
	{"noglob",	FNOGLOB},
	{"nounset",	FNOUNSET},
	{"privileged",	FPRIVILEGED},
	{"stdin",	FSTDIN},
	{"trackall",	FHASHALL},
	{"verbose",	FVERBOSE},
#if defined(EDIT) && defined(VI)
	{"vi",		FVI},
#endif
	{"xtrace",	FXTRACE},
	{NULL,		0}
};	

/*
 * translate -o option into F* constant
 */
int
#if __STDC__
option(Const char *n)
#else
option(n)
	Const char *n;
#endif /* Xn 1992-07-17 */
{
	register struct option *op;

	for (op = options; op->name != NULL; op++)
		if (strcmp(op->name, n) == 0)
			return op->flag;
	return 0;
}

char *
#if __STDC__
getoptions(void)
#else
getoptions()
#endif /* Xn 1992-07-17 */
{
	register int c;
	char m [26+1];
	register char *cp = m;

	for (c = 'a'; c <= 'z'; c++)
		if (flag[FLAG(c)])
			*cp++ = (char) c;
	*cp = 0;
	return strsave(m, ATEMP);
}

void
#if __STDC__
printoptions(void)
#else
printoptions()
#endif /* Xn 1992-07-17 */
{
	register struct option *op;

	for (op = options; op->name != NULL; op++)
		if (flag[op->flag])
			shellf("%s ", op->name);
	shellf("\n");
}
	
/* atoi with error detection */
int
#if __STDC__
getn(char *as)
#else
getn(as)
	char *as;
#endif /* Xn 1992-07-17 */
{
	register char *s;
	register int n;

	s = as;
	if (*s == '-')
		s++;
	for (n = 0; digit(*s); s++)
		n = (n*10) + (*s-'0');
	if (*s)
		errorf("%s: bad number\n", as);
	return (*as == '-') ? -n : n;
}

/*
 * stripped down strerror for kill and exec
 */
char *
#if __STDC__
strerror(int i)
#else
strerror(i)
	int i;
#endif /* Xn 1992-07-17 */
{
	switch (i) {
	  case EINVAL:
		return "Invalid argument";
	  case EACCES:
		return "Permission denied";
	  case ESRCH:
		return "No such process";
	  case EPERM:
		return "Not owner";
	  case ENOENT:
		return "No such file or directory";
	  case ENOTDIR:
		return "Not a directory";
	  case ENOEXEC:
		return "Exec format error";
	  case ENOMEM:
		return "Not enough memory";
	  case E2BIG:
		return "Argument list too long";
	  default:
		return "Unknown system error";
	}
}

/* -------- gmatch.c -------- */

/*
 * int gmatch(string, pattern)
 * char *string, *pattern;
 *
 * Match a pattern as in sh(1).
 * pattern character are prefixed with MAGIC by expand.
 */

static	char	*cclass ARGS((char *, int c));

int
#if __STDC__
gmatch(register char *s, register char *p)
#else
gmatch(s, p)
	register char *s, *p;
#endif /* Xn 1992-07-17 */
{
	register int sc, pc;

	if (s == NULL || p == NULL)
		return 0;
	while ((pc = *p++) != 0) {
		sc = *s++;
		if (pc ==  MAGIC)
			switch (*p++) {
			  case '[':
				if (sc == 0 || (p = cclass(p, sc)) == NULL)
					return (0);
				break;

			  case '?':
				if (sc == 0)
					return (0);
				break;

			  case '*':
				s--;
				do {
					if (*p == '\0' || gmatch(s, p))
						return (1);
				} while (*s++ != '\0');
				return (0);

			  default:
				if (sc != p[-1])
					return 0;
				break;
			}
		else
			if (sc != pc)
				return 0;
	}
	return (*s == 0);
}

static char *
#if __STDC__
cclass(register char *p, register int sub)
#else
cclass(p, sub)
	register char *p;
	register int sub;
#endif /* Xn 1992-07-17 */
{
	register int c, d, not, found = 0;

	if ((not = (*p == MAGIC && *++p == NOT)) != 0) /* Xn 1992-07-31 */
		p++;
	do {
		if (*p == MAGIC)
			p++;
		if (*p == '\0')
			return NULL;
		c = *p;
		if (p[1] == '-' && p[2] != ']') {
			d = p[2];
			p++;
		} else
			d = c;
		if (c == sub || c <= sub && sub <= d)
			found = 1;
	} while (*++p != ']');

	return (found != not) ? p+1 : NULL;
}

/* -------- qsort.c -------- */

/*
 * quick sort of array of generic pointers to objects.
 */

#if 1
void qsort1 ARGS((Void **, Void **, int (*) ARGS((Void *, Void *))));

#endif /* Xn 1992-07-31 */
void
#if __STDC__
qsortp(Void **base, size_t n, int (*f) ARGS((Void *, Void *)))
#else
qsortp(base, n, f)
	Void **base;		/* base address */
	size_t n;		/* elements */
	int (*f) ARGS((Void *, Void *));		/* compare function */
#endif /* Xn 1992-07-17 */
{
	qsort1(base, base + n, f);
}

#define	swap2(a, b)	{\
	register Void *t; t = *(a); *(a) = *(b); *(b) = t;\
}
#define	swap3(a, b, c)	{\
	register Void *t; t = *(a); *(a) = *(c); *(c) = *(b); *(b) = t;\
}

void
#if __STDC__
qsort1(Void **base, Void **lim, int (*f) ARGS((Void *, Void *)))
#else
qsort1(base, lim, f)
	Void **base, **lim;
	int (*f) ARGS((Void *, Void *));
#endif /* Xn 1992-07-17 */
{
	register Void **i, **j;
	register Void **lptr, **hptr;
	size_t n;
	int c;

  top:
	n = (lim - base) / 2;
	if (n == 0)
		return;
	hptr = lptr = base+n;
	i = base;
	j = lim - 1;

	for (;;) {
		if (i < lptr) {
			if ((c = (*f)(*i, *lptr)) == 0) {
				lptr --;
				swap2(i, lptr);
				continue;
			}
			if (c < 0) {
				i += 1;
				continue;
			}
		}

	  begin:
		if (j > hptr) {
			if ((c = (*f)(*hptr, *j)) == 0) {
				hptr ++;
				swap2(hptr, j);
				goto begin;
			}
			if (c > 0) {
				if (i == lptr) {
					hptr ++;
					swap3(i, hptr, j);
					i = lptr += 1;
					goto begin;
				}
				swap2(i, j);
				j -= 1;
				i += 1;
				continue;
			}
			j -= 1;
			goto begin;
		}

		if (i == lptr) {
			if (lptr-base >= lim-hptr) {
				qsort1(hptr+1, lim, f);
				lim = lptr;
			} else {
				qsort1(base, lptr, f);
				base = hptr+1;
			}
			goto top;
		}

		lptr -= 1;
		swap3(j, lptr, i);
		j = hptr -= 1;
	}
}

int
#if __STDC__
xstrcmp(Void *p1, Void *p2)
#else
xstrcmp(p1, p2)
	Void *p1, *p2;
#endif /* Xn 1992-07-17 */
{
	return (strcmp((char *)p1, (char *)p2));
}

void
#if __STDC__
cleanpath(char *pwd, char *dir, char *clean)
#else
cleanpath(pwd, dir, clean)
	char *pwd, *dir, *clean;
#endif /* Xn 1992-07-17 */
{
	register char  *s, *d, *p;
	char *slash = "/";
	register int inslash = 0;
#if _POSIX_SOURCE
	char *root;
#endif /* Xn 1993-06-17 */

#if 0 && XN
	(void) fprintf(stderr, "misc.c/cleanpath - pwd: \"%s\"; dir: \"%s\"\n", pwd, dir);
	(void) fflush(stderr);
#endif /* Xn 1993-06-17 */
	d = clean;
	if (*dir != '/') {
		s = pwd;
		while ((*d++ = *s++) != '\0') /* Xn 1992-07-31 */
			;
#if _POSIX_SOURCE
		if (*pwd == '/' && pwd[1] == '\0')
			--d;
		else
			*(d - 1) = '/';
#else
		*(d - 1) = '/';
#endif /* Xn 1992-08-03 */
	}

	s = dir;
#if _POSIX_SOURCE
	if (*dir == '/' && dir[1] == '/' && dir[2] != '/') {
		*d++ = *s++;
		*d++ = *s++;
		while ((*d++ = *s++) != '/')
			;
		root = d - 1;
	} else if (*dir != '/') {
		root = (*clean == '/' && clean[1] == '/' && clean[2] != '/') ? clean + 3 : clean;
	} else {
#if WIN_NT && CHDIR_KLUDGE
		if (*clean == '/' && clean[1] == '/' && clean[2] != '/') {
			p = pwd;
			*d++ = *p++;
			*d++ = *p++;
			while ((*d++ = *p++) != '/')
				;
			if (*++s == '\0') /* degenerate case where dir is "/" */
			{
				*d = '\0';
				return;
			}
			root = d - 1;
		}
#endif /* Xn 1993-06-17 */
		root = clean;
	}
#endif /* Xn 1993-06-17 */
#if 0 && XN
	(void) fprintf(stderr, "misc.c/cleanpath - clean: \"%*s\"\n", root - clean, clean);
	(void) fflush(stderr);
#endif /* Xn 1992-08-03 */
	while (*s) {
#if _POSIX_SOURCE
		if ((*d++ = *s++) == '/' && d > root + 1) {
			if (*(p = d - 2) == '/') {
				--d;
			} else if (*p == '.') {
				if (*--p == '/') {
					d -= 2;
				} else if (*p == '.' && *--p == '/') {
					while (p > root && *--p != '/')
						;
					d = p + 1;
				}
			}
		}
#else
		if ((*d++ = *s++) == '/' && d > clean + 1) {
			if (*(p = d - 2) == '/') {
				--d;
			} else if (*p == '.') {
				if (*--p == '/') {
					d -= 2;
				} else if (*p == '.' && *--p == '/') {
					while (p > clean && *--p != '/')
						;
					d = p + 1;
				}
			}
		}
#endif /* Xn 1993-06-17 */
		if (!*s && !inslash && *(s - 1) != '/') {
			inslash = 1;
			s = slash;
		}
	}

#if _POSIX_SOURCE
	if (*(d - 1) == '/' && (d - 1) > root)
		d--;
#else
	if (*(d - 1) == '/' && (d - 1) > clean)
		d--;
#endif /* Xn 1993-06-17 */
	*d = '\0';
}
