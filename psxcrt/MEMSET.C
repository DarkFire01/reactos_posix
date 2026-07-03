/* $Header$ */

#include <string.h>

Void *
memset(ap, c, n)
	Void *ap;
	register int c;
	register size_t n;
{
	register char *p = ap;

	if (n++ > 0)
		while (--n > 0)
			*p++ = c;
	return ap;
}

