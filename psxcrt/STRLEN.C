#include <string.h>

/*
 * strlen - length of string (not including NUL)
 */
size_t
strlen(s)
Const char *s;
{
	register Const char *scan;
	register size_t count;

	count = 0;
	scan = s;
	while (*scan++ != '\0')
		count++;
	return(count);
}
