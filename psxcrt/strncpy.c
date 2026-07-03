#include <string.h>

/*
 * strncpy - copy at most n characters of string src to dst
 */
char *				/* dst */
strncpy(dst, src, n)
char *dst;
Const char *src;
size_t n;
{
	register char *dscan;
	register Const char *sscan;
	register size_t count;

	dscan = dst;
	sscan = src;
	count = n;
	/* NB: count is size_t (unsigned), so the classic `--count >= 0` never ends --
	 * it wraps at zero and runs off the end. Guard on count > 0 instead. */
	while (count > 0) {
		count--;
		if ((*dscan++ = *sscan++) == '\0')
			break;
	}
	while (count > 0) {
		count--;
		*dscan++ = '\0';
	}
	return(dst);
}
