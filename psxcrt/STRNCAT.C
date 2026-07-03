#include <string.h>

/*
 * strncat - append at most n characters of string src to dst
 */
char *				/* dst */
strncat(dst, src, n)
char *dst;
Const char *src;
size_t n;
{
	register char *dscan;
	register Const char *sscan;
	register size_t count;

	for (dscan = dst; *dscan != '\0'; dscan++)
		continue;
	sscan = src;
	count = n;
	/* NB: count is size_t (unsigned) -- `--count >= 0` is always true, so the
	 * append ignored n and could overrun dst. Unsigned-safe countdown. */
	while (*sscan != '\0' && count > 0) {
		*dscan++ = *sscan++;
		count--;
	}
	*dscan++ = '\0';
	return(dst);
}
