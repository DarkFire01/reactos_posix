/*
 * area-based allocation built on malloc/free
 */

static char *RCSid = "$Id";

#include <stddef.h>
#include <stdlib.h>
#include <setjmp.h>
#include "sh.h"

#define	ICELLS	100		/* number of Cells in small Block */

typedef union Cell Cell;
typedef struct Block Block;

/*
 * The Cells in a Block are organized as a set of objects.
 * Each object (pointed to by dp) begins with a size in (dp-1)->size,
 * followed with "size" data Cells.  Free objects are
 * linked together via dp->next.
 */

union Cell {
	size_t	size;
	Cell   *next;
	struct {int _;} junk;	/* alignment */
};

struct Block {
	Block  *next;		/* list of Blocks in Area */
	Cell   *free;		/* object free list */
	Cell   *last;		/* &b.cell[size] */
	Cell	cell [1];	/* [size] Cells for allocation */
};

Block aempty = {&aempty, aempty.cell, aempty.cell};

/* create empty Area */
Area *
#if __STDC__
ainit(register Area *ap)
#else
ainit(ap)
	register Area *ap;
#endif /* Xn 1992-07-16 */
{
	ap->free = &aempty;
	return ap;
}

/* free all object in Area */
void
#if __STDC__
afreeall(register Area *ap)
#else
afreeall(ap)
	register Area *ap;
#endif /* Xn 1992-07-16 */
{
	register Block *bp;
#if WIN_NT
	register Block *apf;
	register Block *bpn;

	if ((apf = ap->free) == NULL || apf == &aempty)
		return;
	for (bp = apf; ; bp = bpn) {
		bpn = bp->next;
		free((Void*)bp);
		if (bpn == apf)
			break;
	}
#else

	if (ap->free == NULL || ap->free == &aempty)
		return;
	for (bp = ap->free; ; bp = bp->next) {
		free((Void*)bp);
		if (bp->next == ap->free)
			break;
	}
#endif /* Xn 1992-09-22 */
	ap->free = &aempty;
}

/* allocate object from Area */
Void *
#if __STDC__
alloc(size_t size, register Area *ap)
#else
alloc(size, ap)
	size_t size;
	register Area *ap;
#endif /* Xn 1992-07-16 */
{
	int cells, split;
	register Block *bp;
	register Cell *dp, *fp, *fpp;

	if (size <= 0) {
		aerror(ap, "allocate bad size");
		return NULL;
	}
	cells = (unsigned)(size - 1) / sizeof(Cell) + 1;

	/* find Cell large enough */
	for (bp = ap->free; ; bp = bp->next) {
		for (fpp = NULL, fp = bp->free;
		     fp != bp->last; fpp = fp, fp = fpp->next)
#if 0
			if ((fp-1)->size >= cells)
#else
			if ((fp-1)->size >= (size_t)cells)
#endif /* Xn 1992-07-17 */
				goto Found;

		/* wrapped around Block list, create new Block */
		if (bp->next == ap->free) {
			bp = (Block*) malloc(offsetof(Block, cell[ICELLS + cells]));
			if (bp == NULL) {
				aerror(ap, "cannot allocate");
				return NULL;
			}
			if (ap->free == &aempty)
				bp->next = bp;
			else {
				bp->next = ap->free->next;
				ap->free->next = bp;
			}
			bp->last = bp->cell + ICELLS + cells;
			fp = bp->free = bp->cell + 1; /* initial free list */
			(fp-1)->size = ICELLS + cells - 1;
			fp->next = bp->last;
			fpp = NULL;
			break;
		}
	}
  Found:
	ap->free = bp;
	dp = fp;		/* allocated object */
	split = (dp-1)->size - cells;
	if (split < 0)
		aerror(ap, "allocated object too small");
	if (--split <= 0) {	/* allocate all */
		fp = fp->next;
	} else {		/* allocate head, free tail */
		(fp-1)->size = cells;
		fp += cells + 1;
		(fp-1)->size = split;
		fp->next = dp->next;
	}
	if (fpp == NULL)
		bp->free = fp;
	else
		fpp->next = fp;
	return (Void*) dp;
}

/* change size of object -- like realloc */
Void *
#if __STDC__
aresize(register Void *ptr, size_t size, Area *ap)
#else
aresize(ptr, size, ap)
	register Void *ptr;
	size_t size;
	Area *ap;
#endif /* Xn 1992-07-16 */
{
	int cells;
	register Cell *dp = (Cell*) ptr;

	if (size <= 0) {
		aerror(ap, "allocate bad size");
		return NULL;
	}
	cells = (unsigned)(size - 1) / sizeof(Cell) + 1;

#if 0
	if (dp == NULL || (dp-1)->size < cells) { /* enlarge object */
#else
	if (dp == NULL || (dp-1)->size < (size_t)cells) { /* enlarge object */
#endif /* Xn 1992-07-17 */
		register Cell *np;
		register int i;
		Void *optr = ptr;

		ptr = alloc(size, ap);
		np = (Cell*) ptr;
		if (dp != NULL)
			for (i = (dp-1)->size; i--; )
				*np++ = *dp++;
		afree(optr, ap);
	} else {		/* shrink object */
		int split;

		split = (dp-1)->size - cells;
		if (--split <= 0) /* cannot split */
			;
		else {		/* shrink head, free tail */
			(dp-1)->size = cells;
			dp += cells + 1;
			(dp-1)->size = split;
			afree((Void*)dp, ap);
		}
	}
	return (Void*) ptr;
}

void
#if __STDC__
afree(Void *ptr, register Area *ap)
#else
afree(ptr, ap)
	Void *ptr;
	register Area *ap;
#endif /* Xn 1992-07-16 */
{
	register Block *bp;
	register Cell *fp, *fpp;
	register Cell *dp = (Cell*)ptr;

	/* find Block containing Cell */
	for (bp = ap->free; ; bp = bp->next) {
		if (bp->cell <= dp && dp < bp->last)
			break;
		if (bp->next == ap->free) {
			aerror(ap, "freeing with invalid area");
			return;
		}
	}

	/* find position in free list */
	for (fpp = NULL, fp = bp->free; fp < dp; fpp = fp, fp = fpp->next)
		;

	if (fp == dp) {
		aerror(ap, "freeing free object");
		return;
	}

	/* join object with next */
	if (dp + (dp-1)->size == fp-1) { /* adjacent */
		(dp-1)->size += (fp-1)->size + 1;
		dp->next = fp->next;
	} else			/* non-adjacent */
		dp->next = fp;

	/* join previous with object */
	if (fpp == NULL)
		bp->free = dp;
	else if (fpp + (fpp-1)->size == dp-1) { /* adjacent */
		(fpp-1)->size += (dp-1)->size + 1;
		fpp->next = dp->next;
	} else			/* non-adjacent */
		fpp->next = dp;
}


#if TEST_ALLOC

Area a;

int
#if __STDC__
main(int argc, char **argv)
#else
main(argc, argv)
	int argc;
	char **argv;
#endif /* Xn 1992-07-16 */
{
	int i;
	char *p [9];

	ainit(&a);
	for (i = 0; i < 9; i++) {
		p[i] = alloc(124, &a);
		printf("alloc: %x\n", p[i]);
	}
	for (i = 1; i < argc; i++)
		afree(p[atoi(argv[i])], &a);
	afreeall(&a);
	return 0;
}

void
#if __STDC__
aerror(Area *ap, Const char *msg)
#else
aerror(ap, msg)
	Area *ap;
	Const char *msg;
#endif /* Xn 1992-07-20 */
{
	abort();
}

#endif

