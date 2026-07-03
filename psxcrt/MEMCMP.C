/* $Header$ */

#include <string.h>

int
memcmp(dap, sap, n)
	Const Void *dap;
	Const Void *sap;
	register size_t n;
{
	register Const unsigned char *dp = (unsigned char Const *) dap;
	register Const unsigned char *sp = (unsigned char Const *) sap;

	if (n++ > 0)
		while (--n > 0)
			if (*dp++ != *sp++)
				return *--dp - *--sp; /* (int)? */
	return 0;
}

