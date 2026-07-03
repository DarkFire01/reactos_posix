#ifndef lint
static char *RCSid = "$Id: var.c,v 3.2 89/03/27 15:52:21 egisin Exp $";
static char *sccs_id = "@(#)var.c	1.3 91/11/09 15:35:17 (sjg)";
#endif

#include <stddef.h>
#if __STDC__
#include <stdlib.h>
#endif /* Xn 1992-07-17 */
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <time.h>
#if WIN_NT
#include <ctype.h>
#endif /* Xn 1992-08-03 */
#if 0 && XN
#include <stdio.h>
#endif /* Xn 1993-06-15 */
#include "sh.h"
#include "table.h"
#include "expand.h"

/*
 * Variables
 *
 * WARNING: unreadable code, needs a rewrite
 *
 * if (flag&INTEGER), val.i contains integer value, and type contains base.
 * otherwise, (val.s + type) contains string value.
 * if (flag&EXPORT), val.s contains "name=value" for E-Z exporting.
 */
char	null []	= "";
static	struct tbl vtemp;
#if 0
static	void getspec(), setspec();
#else
extern	void flushcom ARGS((int)); /* in exec.c */
extern	void mbset ARGS((register char *)); /* in mail.c */
extern	void mpset ARGS((register char *)); /* in mail.c */

static	void getspec ARGS((register struct tbl *)), setspec ARGS((register struct tbl *));
#endif /* Xn 1992-07-31 */
static	void export ARGS((struct tbl *, char *val));
static	int special ARGS ((char *name));

#if WIN_NT
extern char *dos_to_posix (const char *, size_t, int); /* in sh.c */
extern char *posix_to_dos (const char *, size_t, int); /* in sh.c */

#endif /* Xn 1992-08-19 */
/*
 * create a new block for function calls and simple commands
 * assume caller has allocated and set up e.loc
 */
void
#if __STDC__
newblock(void)
#else
newblock()
#endif /* Xn 1992-07-17 */
{
	register struct block *l = e.loc;
	static char *empty[] = {""};

	ainit(&l->area);
	l->argc = 0;
	l->argv = empty;
	l->exit = l->error = NULL;
	tinit(&l->vars, &l->area);
	tinit(&l->funs, &l->area);
}

/*
 * pop a block handling special variables
 */
void
#if __STDC__
popblock(void)
#else
popblock()
#endif /* Xn 1992-07-17 */
{
	register struct block *l = e.loc;
	register struct tbl *vp, **vpp = l->vars.tbls;
	register int i;

	e.loc = l->next;	/* pop block */
	for (i = l->vars.size; --i >= 0; )
		if ((vp = *vpp++) != NULL && (vp->flag&SPECIAL))
			setspec(global(vp->name));
	afreeall(&l->area);
}

/*
 * Search for variable, if not found create globally.
 */
struct tbl *
#if __STDC__
global(register char *n)
#else
global(n)
	register char *n;
#endif /* Xn 1992-07-17 */
{
	register struct block *l = e.loc;
	register struct tbl *vp;
	register int c;
	unsigned h = hash(n);

	c = n[0];
	if (digit(c)) {
		vp = &vtemp;
		lastarea = ATEMP;
		vp->flag = (DEFINED|RDONLY);
		vp->type = 0;
		*vp->name = (char)c;	/* should strncpy - Xn 1992-07-31 */
		for (c = 0; digit(*n) && c < 1000; n++)
			c = c*10 + *n-'0';
		if (c <= l->argc)
			setstr(vp, l->argv[c]);
		return vp;
	} else
	if (!letter(c)) {
		vp = &vtemp;
		lastarea = ATEMP;
		vp->flag = (DEFINED|RDONLY);
		vp->type = 0;
		*vp->name = (char)c; /* Xn 1992-07-31 */
		if (n[1] != '\0')
			return vp;
		vp->flag |= ISSET|INTEGER;
		switch (c) {
		  case '$':
			vp->val.i = (long)kshpid;
			break;
		  case '!':
			vp->val.i = async;
			break;
		  case '?':
			vp->val.i = exstat;
			break;
		  case '#':
			vp->val.i = l->argc;
			break;
		  case '-':
			vp->flag &= ~ INTEGER;
			vp->val.s = getoptions();
			break;
		  default:
			vp->flag &= ~(ISSET|INTEGER);
		}
		return vp;
	}
	for (l = e.loc; l != NULL; l = l->next) {
		vp = tsearch(&l->vars, n, h);
		lastarea = &l->area;
		if (vp != NULL)
			return vp;
		if (l->next == NULL)
			break;
	}
	vp = tenter(&l->vars, n, h);
	vp->flag |= DEFINED;
	if (special(n))
		vp->flag |= SPECIAL;
	return vp;
}

/*
 * Search for local variable, if not found create locally.
 */
struct tbl *
#if __STDC__
local(register char *n)
#else
local(n)
	register char *n;
#endif /* Xn 1992-07-17 */
{
	register struct block *l = e.loc;
	register struct tbl *vp;
	unsigned h = hash(n);

	if (!letter(*n)) {
		vp = &vtemp;
		lastarea = ATEMP;
		vp->flag = (DEFINED|RDONLY);
		vp->type = 0;
		return vp;
	}
	vp = tenter(&l->vars, n, h);
	lastarea = &l->area;
	vp->flag |= DEFINED;
	if (special(n))
		vp->flag |= SPECIAL;
	return vp;
}

/* get variable string value */
char *
#if __STDC__
strval(register struct tbl *vp)
#else
strval(vp)
	register struct tbl *vp;
#endif /* Xn 1992-07-17 */
{
	register char *s;
	static char strbuf[40];

#if 0 && XN
	(void) fprintf(stderr, "vp->name: \"%s\"\n", vp->name);
	(void) fflush(stderr);
#endif
	if ((vp->flag&SPECIAL))
		getspec(vp);
	if (!(vp->flag&ISSET))
		return null;	/* special to dollar() */
	if (!(vp->flag&INTEGER))	/* string source */
#if 0 && XN
	{
		(void) fprintf(stderr, "vp->name: \"%s\"; vp->val.s: \"%s\"; vp->type: %d\n", vp->name, vp->val.s, vp->type);
		(void) fflush(stderr);
#endif
		s = vp->val.s + vp->type;
#if 0 && XN
	}
#endif
	else {				/* integer source */
		register unsigned long n;
		register int base;

		s = strbuf + sizeof(strbuf);
		n = (vp->val.i < 0) ? -vp->val.i : vp->val.i;
		base = (vp->type == 0) ? 10 : vp->type;

		*--s = '\0';
		do {
			*--s = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[n%base];
			n /= base;
		} while (n != 0);
		/* todo: should we output base# ? */
		if (vp->val.i < 0)
			*--s = '-';
	}
	return s;
}

/* get variable integer value */
long
#if __STDC__
intval(register struct tbl *vp)
#else
intval(vp)
	register struct tbl *vp;
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vq;

	if ((vp->flag&SPECIAL))
		getspec(vp);
	if ((vp->flag&INTEGER))
		return vp->val.i;
	vq = &vtemp;
	vq->flag = (INTEGER);
	vq->type = 0;
	if (strint(vq, vp) == NULL)
		errorf("%s: bad number\n", vp->val.s);
	return vq->val.i;
}

#if WIN_NT
static int needs_posixizing (const struct tbl *vp)
{
	int ret;
	Const char **i;
	extern Const char *initcoms[];

	if (vp == NULL)
		ret = 0;
	else if (kshpid == (pid_t) -1) /* see main.c */
	{
		ret = 0;
		for (i = initcoms; *i != NULL || i[1] != NULL; ++i)
			if (*i == NULL || i[1] == NULL)
				continue;
			else if (strcmp(*i, "typeset") == 0 && strcmp(i[1], "-p") == 0)
				break;
		if (*i != NULL)
			for (++i, ++i; *i != NULL; ++i)
				if (strcmp(*i, vp->name) == 0)
				{
					ret = 1;
					break;
				}
	}
	else
		ret = (vp->flag & POSIXIZE) ? 1 : 0;
	return ret;
}

static int needs_dosifying (const struct tbl *vp)
{
	int ret;

	if (vp == NULL)
		ret = 0;
	else
		ret = (vp->flag & DOSIFY) ? 1 : 0;
	return ret;
}

#endif /* Xn 1992-08-19 */
/* set variable to string value */
void
#if __STDC__
setstr(register struct tbl *vq, char *s)
#else
setstr(vq, s)
	register struct tbl *vq;
	char *s;
#endif /* Xn 1992-07-17 */
{
#if 0 && XN
	(void) fprintf(stderr, "setstr - ");
	if (vq != NULL)
		(void) fprintf(stderr, "vq->name: \"%s\"; ", vq->name);
	(void) fprintf(stderr, "s: %p [", s);
	if (s == NULL)
		(void) fprintf(stderr, "NULL");
	else
		(void) fprintf(stderr, "\"%s\"", s);
	(void) fprintf(stderr, "]");
	if (vq != NULL)
		(void) fprintf(stderr, "; ALLOC: %d; EXPORT: %d", (vq->flag & ALLOC) ? 1 : 0, (vq->flag & EXPORT) ? 1 : 0);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
#endif
	if (!(vq->flag&INTEGER)) { /* string dest */
		if ((vq->flag&ALLOC))
			afree((Void*)vq->val.s, lastarea);
		vq->flag &= ~ (ISSET|ALLOC);
		vq->type = 0;
#if WIN_NT
		if (needs_posixizing(vq))
		{
			char *dos_name, *p;
	
			dos_name = s;
			p = strchr(dos_name, '=');
			if (p == NULL)
				p = dos_name;
			else
				++p;
			s = strsave(dos_to_posix(p, 0, 0), lastarea); /* last 0 == preserve case */
#if 0 && XN
			(void) fprintf(stderr, "dos_name: \"%s\"; s: \"%s\"\n", dos_name, s);
			(void) fflush(stderr);
#endif
		}
		else if (needs_dosifying(vq))
		{
			char *posix_name, *p;
	
			posix_name = s;
			p = strchr(posix_name, '=');
			if (p == NULL)
				p = posix_name;
			else
				++p;
			s = strsave(posix_to_dos(p, 0, 0), lastarea); /* last 0 == preserve case */
		}
#endif /* Xn 1992-08-19 */
		if ((vq->flag&EXPORT))
			export(vq, s);
		else
			vq->val.s = strsave(s, lastarea);
		vq->flag |= ALLOC;
#if 0 && XN
		if (vq != NULL)
			(void) fprintf(stderr, "vq->name: \"%s\"; ", vq->name);
		(void) fprintf(stderr, "vq->val.s: %p [", vq->val.s);
		if (vq->val.s == NULL)
			(void) fprintf(stderr, "NULL");
		else
			(void) fprintf(stderr, "\"%s\"", vq->val.s);
		(void) fprintf(stderr, "]\n");
		(void) fflush(stderr);
#endif
	} else {		/* integer dest */
		register struct tbl *vp = &vtemp;	
		vp->flag = (DEFINED|ISSET);
		vp->type = 0;
		vp->val.s = s;
		if (strint(vq, vp) == NULL)
			errorf("%s: bad number\n", s);
	}
	vq->flag |= ISSET;
	if ((vq->flag&SPECIAL))
		setspec(vq);
}
	
/* convert variable to integer variable */
struct tbl *
#if __STDC__
strint(register struct tbl *vq, register struct tbl *vp)
#else
strint(vq, vp)
	register struct tbl *vq, *vp;
#endif /* Xn 1992-07-17 */
{
	register char *s = vp->val.s + vp->type;
	register int c;
	int base, neg = 0;
	
	vq->flag |= INTEGER;
	if (!(vp->flag&ISSET) || (s == NULL && !(vp->flag&INTEGER))) {
		vq->flag &= ~ ISSET;
		return NULL;
	}
	if ((vp->flag&INTEGER)) {
		vq->val.i = vp->val.i;
		return vq;
	}
	vq->val.i = 0;
	base = 10;
	for (c = *s++; c ; c = *s++)
		if (c == '-') {
			neg++;
		} else if (c == '#') {
			base = vq->type = (int)vq->val.i; /* Xn 1992-07-31 */
			vq->val.i = 0;
		} else if (letnum(c)) {
			if ('0' <= c && c <= '9')
				c -= '0';
			else if ('a' <= c && c <= 'z') /* fuck EBCDIC */
				c -= 'a'-10;
			else if ('A' <= c && c <= 'Z')
				c -= 'A'-10;
			if (c < 0 || c >= base) {
				vq->flag &= ~ ISSET;
				return NULL;
			}
			vq->val.i = (vq->val.i*base) + c;
		} else
			break;
	if (neg)
		vq->val.i = -vq->val.i;
	if (vq->type < 2 || vq->type > 36)
		vq->type = 0;	/* default base (10) */
	return vq;
}

/* set variable to integer */
void
#if __STDC__
setint(register struct tbl *vq, long n)
#else
setint(vq, n)
	register struct tbl *vq;
	long n;
#endif /* Xn 1992-07-17 */
{
	if (!(vq->flag&INTEGER)) {
		register struct tbl *vp = &vtemp;
		vp->flag = (ISSET|INTEGER);
		vp->type = 0;
		vp->val.i = n;
		setstr(vq, strval(vp));	/* ? */
	} else
		vq->val.i = n;
	vq->flag |= ISSET;
	if ((vq->flag&SPECIAL))
		setspec(vq);
}

/* set variable from enviroment */
int
#if __STDC__
import(char *thing)
#else
import(thing)
	char *thing;
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vp;
	register char *val;
#if WIN_NT
	register char *var;
#endif /* Xn 1992-08-03 */

#if 0 && XN
	(void) fprintf(stderr, "thing: ");
	if (thing == NULL)
		(void) fprintf(stderr, "NULL");
	else
		(void) fprintf(stderr, "\"%s\"", thing);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
#endif
#if WIN_NT
	if (*thing == '=' && isupper(thing[1]) && thing[2] == ':' && thing[3] == '=')
		return 0; /* don't import special drive environment variables */
#endif /* Xn 1992-08-18 */
	val = strchr(thing, '=');
	if (val == NULL)
		return 0;
	*val = '\0';
	vp = local(thing);
	*val++ = '=';
	vp->flag |= DEFINED|ISSET|EXPORT;
#if WIN_NT
	if (needs_posixizing(vp))
	{
		char *posix_name, *xp, *cp;

		posix_name = dos_to_posix(val, 0, 0); /* last 0 == preserve case */
		xp = (char *)alloc(strlen(vp->name) + strlen(posix_name) + 2, lastarea);
		vp->flag |= ALLOC;
		vp->val.s = xp;
		for (cp = vp->name; (*xp = *cp) != '\0'; ++cp)
			++xp;
		*xp++ = '=';
		vp->type = xp - vp->val.s; /* offset to value */
		for (cp = posix_name; (*xp = *cp) != '\0'; ++cp)
			++xp;
	}
	else
	{
#if 0
		char *xp, *cp;

		xp = (char *)alloc(strlen(thing) + 1, lastarea);
		vp->val.s = xp;
		vp->flag |= ALLOC;
		for (cp = thing; (*xp = *cp) != '\0'; ++cp)
			++xp;
		vp->type = val - thing;
#else /* this works well enough */
		vp->val.s = thing;
		vp->type = val - thing;
#endif /* Xn 1992-09-17 */
	}
#else
	vp->val.s = thing;
	vp->type = val - thing;
#endif /* convert Windows NT variables with pathnames to POSIX format - Xn 1992-08-17 */
#if WIN_NT
	for (var = vp->name; *var != '\0'; ++var)
		if (islower(*var))
			*var = (char)toupper(*var);
#endif /* Windows NT has case-insensitive environment variable names - Xn 1992-08-03 */
	if ((vp->flag&SPECIAL))
		setspec(vp);
	return 1;
}

/*
 * make vp->val.s be "name=value" for quick exporting.
 */
static void
#if __STDC__
export(register struct tbl *vp, char *val)
#else
export(vp, val)
	register struct tbl *vp;
	char *val;
#endif /* Xn 1992-07-17 */
{
	register char *cp, *xp;
	char *op = (vp->flag&ALLOC) ? vp->val.s : NULL;

	xp = (char*)alloc(strlen(vp->name) + strlen(val) + 2, lastarea);
	vp->flag |= ALLOC;
	vp->val.s = xp;
	for (cp = vp->name; (*xp = *cp++) != '\0'; xp++)
		;
	*xp++ = '=';
	vp->type = xp - vp->val.s; /* offset to value */
	for (cp = val; (*xp++ = *cp++) != '\0'; )
		;
	if (op != NULL)
		afree((Void*)op, lastarea);
}

/*
 * lookup variable (according to (set&LOCAL)),
#if WIN_NT
 * set its attributes (INTEGER, POSIXIZE, RDONLY, EXPORT, TRACE),
#else
 * set its attributes (INTEGER, RDONLY, EXPORT, TRACE),
#endif
 * and optionally set its value if an assignment.
 */
struct tbl *
#if __STDC__
typeset(register char *var, int set, int clr)
#else
typeset(var, set, clr)
	register char *var;
	int set, clr;
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vp;
	register char *val;
#if WIN_NT
	int old_flag;
#endif /* Xn 1992-08-17 */

#if 0 && XN
	(void) fprintf(stderr, "typeset - var: ");
	if (var == NULL)
		(void) fprintf(stderr, "NULL");
	else
		(void) fprintf(stderr, "\"%s\"", var);
	(void) fprintf(stderr, "; set: %08lX; clr: %08lX\n", (unsigned long) set, (unsigned long) clr);
	(void) fflush(stderr);
#endif
	/* check for valid variable name, search for value */
	val = var;
	if (!letter(*val))
		return NULL;
	for (val++; *val != '\0'; val++)
		if (*val == '=')
			break;
		else if (letnum(*val))
			;
		else
			return NULL;
	if (*val == '=')
		*val = '\0';
	else
		val = NULL;
	vp = (set&LOCAL) ? local(var) : global(var);
	set &= ~ LOCAL;
	if (val != NULL)
		*val++ = '=';

#if WIN_NT
	old_flag = (vp == NULL) ? ~0 : vp->flag;
#endif /* Xn 1992-08-17 */
	if (!(vp->flag&ISSET))
		vp->flag = vp->flag & ~clr | set;
	else
	    if (!(vp->flag&INTEGER) && (set&INTEGER)) {
		/* string to integer */
		vtemp.flag = (ISSET);
		vtemp.type = 0;
		vtemp.val.s = vp->val.s + vp->type;
		if ((vp->flag&ALLOC))
			afree((Void*)vp->val.s, lastarea); /* dangerous, used later */
		vp->flag &= ~ ALLOC;
		vp->flag |= INTEGER;
		vp->type = 0;
		if (val == NULL && strint(vp, &vtemp) == NULL) {
			vp->flag &= ~ ISSET;
			errorf("%s: bad number\n", vtemp.val.s);
		}
	    } else
	    if ((clr&INTEGER) && (vp->flag&INTEGER)) {
		/* integer to string */
		vtemp.val.s = strval(vp);
		vp->flag &= ~ INTEGER;
		setstr(vp, vtemp.val.s);
	    }

	vp->flag = vp->flag & ~clr | set;
#if WIN_NT
	if (kshpid != (pid_t) -1 && val == NULL && vp->val.s != NULL)
		if (!(old_flag & POSIXIZE) && (vp->flag & POSIXIZE)) /* see main.c */
		{
#if 0 && XN
			(void) fprintf(stderr, "before - vp->val.s: \"%s\"\n", vp->val.s);
			(void) fflush(stderr);
#endif
			vtemp.val.s = (char *)alloc(strlen(vp->val.s) + 1, lastarea);
			(void) strcpy(vtemp.val.s, vp->val.s);
#if 0 && XN
			(void) fprintf(stderr, "during - vtemp.val.s: \"%s\"\n", vtemp.val.s);
			(void) fflush(stderr);
#endif
			setstr(vp, vtemp.val.s);
#if 0
			afree(vp->val.s, lastarea);
#endif
#if 0 && XN
			(void) fprintf(stderr, "after - vp->val.s: \"%s\"\n", vp->val.s);
			(void) fflush(stderr);
#endif
		}
		else if ((old_flag & POSIXIZE) && !(vp->flag & POSIXIZE))
		{
			vtemp.val.s = (char *)alloc(strlen(vp->val.s) + 1, lastarea);
			(void) strcpy(vtemp.val.s, vp->val.s);
			vp->flag |= DOSIFY;
			setstr(vp, vtemp.val.s);
			vp->flag &= ~DOSIFY;
		}
#endif /* Xn 1992-08-19 */

	if (val != NULL) {
		if ((vp->flag&RDONLY))
			errorf("cannot set readonly %s\n", var);
		if ((vp->flag&INTEGER))
#if 0 && XN
		{
			(void) fprintf(stderr, "var.c/typeset - var: \"%s\"\n", var);
			(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
			/* setstr should be able to handle this */
			(void)evaluate(var);
#if 0 && XN
		}
#endif /* Xn 1993-06-15 */
		else
#if 0 && XN
		{
			(void) fprintf(stderr, "var: \"%s\"; POSIXIZE: %d\n", var, (vp->flag & POSIXIZE) ? 1 : 0);
			(void) fflush(stderr);
#endif
			setstr(vp, val);
#if 0 && XN
		}
#endif
	}

	if ((vp->flag&EXPORT) && !(vp->flag&INTEGER) && vp->type == 0)
		export(vp, (vp->flag&ISSET) ? vp->val.s : null);

	return vp;
}

void
#if __STDC__
unset(register struct tbl *vp)
#else
unset(vp)
	register struct tbl *vp;
#endif /* Xn 1992-07-17 */
{
	if ((vp->flag&ALLOC))
		afree((Void*)vp->val.s, lastarea);
	vp->flag &= SPECIAL;	/* Should ``unspecial'' some vars */
}

int
#if __STDC__
isassign(register char *s)
#else
isassign(s)
	register char *s;
#endif /* Xn 1992-07-17 */
{
	if (!letter(*s))
		return (0);
	for (s++; *s != '='; s++)
		if (*s == 0 || !letnum(*s))
			return (0);
	return (1);
}

/*
 * Make the exported environment from the exported names in the dictionary.
 */
char **
#if __STDC__
makenv(void)
#else
makenv()
#endif /* Xn 1992-07-17 */
{
	struct block *l = e.loc;
	XPtrV env;
	register struct tbl *vp, **vpp;
	register int i;

	XPinit(env, 64);
	for (l = e.loc; l != NULL; l = l->next)
		for (vpp = l->vars.tbls, i = l->vars.size; --i >= 0; )
			if ((vp = *vpp++) != NULL
			    && (vp->flag&(ISSET|EXPORT)) == (ISSET|EXPORT)) {
				register struct block *l2;
				register struct tbl *vp2;
				unsigned h = hash(vp->name);

				lastarea = &l->area;

				/* unexport any redefined instances */
				for (l2 = l->next; l2 != NULL; l2 = l2->next) {
					vp2 = tsearch(&l2->vars, vp->name, h);
					if (vp2 != NULL)
						vp2->flag &= ~ EXPORT;
				}
				if ((vp->flag&INTEGER)) {
					/* integer to string */
					char *val;
					val = strval(vp);
					vp->flag &= ~ INTEGER;
					setstr(vp, val);
				}
				XPput(env, vp->val.s);
			}
	XPput(env, NULL);
	return (char **) XPclose(env);
}

/*
 * handle special variables with side effects - PATH, SECONDS.
 */

static int
#if __STDC__
special(register char *name)
#else
special(name)
	register char * name;
#endif /* Xn 1992-07-17 */
{
	if (strcmp("PATH", name) == 0)
		return V_PATH;
	if (strcmp("IFS", name) == 0)
		return V_IFS;
	if (strcmp("SECONDS", name) == 0)
		return V_SECONDS;
	if (strcmp("OPTIND", name) == 0)
		return V_OPTIND;
	if (strcmp("MAIL", name) == 0)
		return V_MAIL;
	if (strcmp("MAILPATH", name) == 0)
		return V_MAILPATH;
	if (strcmp("RANDOM", name) == 0)
		return V_RANDOM;
	return V_NONE;
}

#if 0
extern	time_t time();
#endif /* Xn 1992-07-17 */
static	time_t	seconds;		/* time SECONDS last set */
#if !__STDC__
extern	int	rand();
extern	void	srand();
#endif /* Xn 1992-07-17 */

static void
#if __STDC__
getspec(register struct tbl *vp)
#else
getspec(vp)
	register struct tbl *vp;
#endif /* Xn 1992-07-17 */
{
	switch (special(vp->name)) {
	case V_SECONDS:
		vp->flag &= ~ SPECIAL;
		setint(vp, time((time_t *)0) - seconds);
		vp->flag |= SPECIAL;
		break;
	case V_RANDOM:
		vp->flag &= ~ SPECIAL;
		setint(vp, (rand() & 0x7fff));
		vp->flag |= SPECIAL;
		break;
	}
}

static void
#if __STDC__
setspec(register struct tbl *vp)
#else
setspec(vp)
	register struct tbl *vp;
#endif /* Xn 1992-07-17 */
{
#if 0
	extern void	mbset(), mpset();
#endif /* Xn 1992-07-31 */

	switch (special(vp->name)) {
	  case V_PATH:
		path = strval(vp);
#if 0 && XN
		(void) fprintf(stderr, "path: ");
		if (path == NULL)
			(void) fprintf(stderr, "NULL");
		else
			(void) fprintf(stderr, "\"%s\"", path);
		(void) fprintf(stderr, "; vp->name: \"%s\"\n", vp->name);
		(void) fflush(stderr);
#endif
		flushcom(1);	/* clear tracked aliases */
		break;
	  case V_IFS:
		setctypes(strval(vp), C_IFS);
		break;
	  case V_SECONDS:
		seconds = time((time_t *)0);
		break;
	  case V_OPTIND:
		if (intval(vp) == 1)
			resetopts();
		break;
	  case V_MAIL:
		mbset(strval(vp));
		break;
	  case V_MAILPATH:
		mpset(strval(vp));
		break;
	  case V_RANDOM:
		vp->flag &= ~ SPECIAL;
		srand((unsigned int)intval(vp));
		vp->flag |= SPECIAL;
		break;
	}
}

