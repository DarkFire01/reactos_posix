#if 0
#include <stdio.h>
#else
#include <stddef.h>
#endif

void
#if __STDC__
regerror (const char *s)
#else
regerror(s)
char *s;
#endif
{
#ifdef ERRAVAIL
	error("regexp: %s", s);
#else
# if 0
	fprintf(stderr, "regexp(3): %s\n", s);
	exit(1);
# else
	if (s == NULL)
		;
# endif
#endif
	return;	  /* let std. egrep handle errors */
}
