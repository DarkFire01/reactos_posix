/* $Header$ */

#include <string.h>

Void *
memchr(ap, c, n)
	Const Void *ap;
	register int c;
	register size_t n;
{
	register char *p = ap;

	if (n++ > 0)
		while (--n > 0)
			if (*p++ == c)
				return --p;
	return NULL;
}

