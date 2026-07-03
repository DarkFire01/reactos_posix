static char *RCSid = "$Id: table.c,v 3.2 89/03/27 15:51:58 egisin Exp $";

/*
 * dynamic hashed associative table for commands and variables
 */

#include <stddef.h>
#include <errno.h>
#include <setjmp.h>
#if 0 && XN
#include <stdio.h>
#endif
#include "sh.h"
#include "table.h"

#define	INIT_TBLS	8	/* initial table size (power of 2) */

static struct tstate {
	int left;
	struct tbl **next;
} tstate;

#if 0
static void texpand();
#else
static void texpand ARGS((register struct table *, int));
#endif /* Xn 1992-07-17 */

unsigned int
#if __STDC__
hash(register char *n)
#else
hash(n)
	register char *n;
#endif /* Xn 1992-07-17 */
{
	register unsigned int h = 0;

	while (*n != '\0')
		h = 2*h + *n++;
	return h * 32821;	/* scatter bits */
}

#if 0
void
#if __STDC__
phash(char *s)
#else
phash(s)
	char *s;
#endif /* Xn 1992-07-17 */
{
	printf("%2d: %s\n", hash(s)%32, s);
}
#endif

void
#if __STDC__
tinit(register struct table *tp, register Area *ap)
#else
tinit(tp, ap)
	register struct table *tp;
	register Area *ap;
#endif /* Xn 1992-07-17 */
{
	tp->areap = ap;
	tp->size = tp->free = 0;
	tp->tbls = NULL;
}

static void
#if __STDC__
texpand(register struct table *tp, int nsize)
#else
texpand(tp, nsize)
	register struct table *tp;
	int nsize;
#endif /* Xn 1992-07-17 */
{
	register int i;
	register struct tbl *tblp, **p;
	register struct tbl **ntblp, **otblp = tp->tbls;
	int osize = tp->size;

	ntblp = (struct tbl**) alloc(sizeofN(struct tbl *, nsize), tp->areap);
	for (i = 0; i < nsize; i++)
		ntblp[i] = NULL;
	tp->size = nsize;
	tp->free = 8*nsize/10;	/* table can get 80% full */
	tp->tbls = ntblp;
	if (otblp == NULL)
		return;
	for (i = 0; i < osize; i++)
		if ((tblp = otblp[i]) != NULL)
			if ((tblp->flag&DEFINED)) {
				for (p = &ntblp[hash(tblp->name) & tp->size-1];
				     *p != NULL; p--)
					if (p == ntblp) /* wrap */
						p += tp->size;
				*p = tblp;
				tp->free--;
			} else {
				afree((Void*)tblp, tp->areap);
			}
	afree((Void*)otblp, tp->areap);
}

struct tbl *
#if __STDC__
tsearch(register struct table *tp, register char *n, unsigned int h)
#else
tsearch(tp, n, h)
	register struct table *tp;	/* table */
	register char *n;		/* name to enter */
	unsigned int h;			/* hash(n) */
#endif /* Xn 1992-07-17 */
{
	register struct tbl **pp, *p;

#if 0 && XN
	(void) fprintf(stderr, "entering tsearch\n");
	(void) fflush(stderr);
#endif
	if (tp->size == 0)
#if 0 && XN
	{
		(void) fprintf(stderr, "exiting tsearch (NULL) #1\n");
		(void) fflush(stderr);
#endif
		return NULL;
#if 0 && XN
	}
#endif

	/* search for name in hashed table */
	for (pp = &tp->tbls[h & tp->size-1]; (p = *pp) != NULL; pp--) {
		if (*p->name == *n && strcmp(p->name, n) == 0
		    && (p->flag&DEFINED))
#if 0 && XN
		{
			(void) fprintf(stderr, "exiting tsearch\n");
			(void) fflush(stderr);
#endif
			return p;
#if 0 && XN
		}
#endif
		if (pp == tp->tbls) /* wrap */
			pp += tp->size;
	}

#if 0 && XN
	(void) fprintf(stderr, "exiting tsearch (NULL) #2\n");
	(void) fflush(stderr);
#endif
	return NULL;
}

struct tbl *
#if __STDC__
tenter(register struct table *tp, register char *n, unsigned int h)
#else
tenter(tp, n, h)
	register struct table *tp;	/* table */
	register char *n;		/* name to enter */
	unsigned int h;			/* hash(n) */
#endif /* Xn 1992-07-17 */
{
	register struct tbl **pp, *p;
	register char *cp;

	if (tp->size == 0)
		texpand(tp, INIT_TBLS);
  Search:
	/* search for name in hashed table */
	for (pp = &tp->tbls[h & tp->size-1]; (p = *pp) != NULL; pp--) {
		if (*p->name == *n && strcmp(p->name, n) == 0)
			return p; 	/* found */
		if (pp == tp->tbls) /* wrap */
			pp += tp->size;
	}

	if (tp->free <= 0) {	/* too full */
		texpand(tp, 2*tp->size);
		goto Search;
	}

	/* create new tbl entry */
	for (cp = n; *cp != '\0'; cp++)
		;
	p = (struct tbl *) alloc(offsetof(struct tbl, name[(cp-n)+1]), tp->areap);
	p->flag = 0;
	p->type = 0;
#if WIN_NT
	p->val.i = 0L;
#endif /* 1992-09-22 */
	for (cp = p->name; *n != '\0';)
		*cp++ = *n++;
	*cp = '\0';

	/* enter in tp->tbls */
	tp->free--;
	*pp = p;
	return p;
}

void
#if __STDC__
tdelete(register struct tbl *p)
#else
tdelete(p)
	register struct tbl *p;
#endif /* Xn 1992-07-17 */
{
	p->flag = 0;
}

void
#if __STDC__
twalk(register struct table *tp)
#else
twalk(tp)
	register struct table *tp;
#endif /* Xn 1992-07-17 */
{
	tstate.left = tp->size;
	tstate.next = tp->tbls;
}

struct tbl *
#if __STDC__
tnext(void)
#else
tnext()
#endif /* Xn 1992-07-17 */
{
	while (--tstate.left >= 0) {
		struct tbl *p = *tstate.next++;
		if (p != NULL && (p->flag&DEFINED))
			return p;
	}
	return NULL;
}

static int
#if __STDC__
tnamecmp(Void *p1, Void *p2)
#else
tnamecmp(p1, p2)
	Void *p1, *p2;
#endif /* Xn 1992-07-17 */
{
	return strcmp(((struct tbl *)p1)->name, ((struct tbl *)p2)->name);
}

struct tbl **
#if __STDC__
tsort(register struct table *tp)
#else
tsort(tp)
	register struct table *tp;
#endif /* Xn 1992-07-17 */
{
	register int i;
	register struct tbl **p, **sp, **dp;

	p = (struct tbl **)alloc(sizeofN(struct tbl *, tp->size+1), ATEMP);
	sp = tp->tbls;		/* source */
	dp = p;			/* dest */
	for (i = 0; i < tp->size; i++)
		if ((*dp = *sp++) != NULL && ((*dp)->flag&DEFINED))
			dp++;
	i = dp - p;
#if 0
	qsortp((Void**)p, (size_t)i, tnamecmp);
#else
	qsortp((Void**)p, (size_t)i, (int (*) ARGS((Void *, Void *)))tnamecmp);
#endif /* Xn 1992-07-16 */
	p[i] = NULL;
	return p;
}

