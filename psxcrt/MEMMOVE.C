/* $Header$ */

#include <string.h>

Void *
memmove(dap, sap, n)
	Void *dap;
	Const Void *sap;
	register size_t n;
{
	register char *dp = dap, *sp = (Void*) sap;

	if (n <= 0)
		;
	else if (dp < sp)
		do *dp++ = *sp++; while (--n > 0);
	else if (dp > sp) {
		dp += n;
		sp += n;
		do *--dp = *--sp; while (--n > 0);
	}
	return dap;
}


