#include <string.h>

/*
 * strncmp - compare at most n characters of string s1 to s2
 */

int				/* <0 for <, 0 for ==, >0 for > */
strncmp(s1, s2, n)
Const char *s1;
Const char *s2;
size_t n;
{
	register Const char *scan1;
	register Const char *scan2;
	register size_t count;

	scan1 = s1;
	scan2 = s2;
	count = n;
	/* NB: count is size_t (unsigned), so the classic `--count >= 0` never ends
	 * and `count < 0` never fires -- n was ignored and every prefix compare
	 * ran off the end (this broke the Xlib locale parser's "END <category>"
	 * match, among others). Count down with an unsigned-safe guard instead. */
	while (count > 0 && *scan1 != '\0' && *scan1 == *scan2) {
		scan1++;
		scan2++;
		count--;
	}
	if (count == 0)
		return(0);

	/*
	 * The following case analysis is necessary so that characters
	 * which look negative collate low against normal characters but
	 * high against the end-of-string NUL.
	 */
	if (*scan1 == '\0' && *scan2 == '\0')
		return(0);
	else if (*scan1 == '\0')
		return(-1);
	else if (*scan2 == '\0')
		return(1);
	else
		return(*scan1 - *scan2);
}
