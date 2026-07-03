/* $Header$ */

#include <string.h>

Void *
memcpy(dap, sap, n)
	Void *dap;
	Const Void *sap;
	register size_t n;
{
	register char *dp = dap, *sp = (Void*) sap;

	if (n++ > 0)
		while (--n > 0)
			*dp++ = *sp++;
	return dap;
}

