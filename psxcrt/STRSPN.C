#include <string.h>

/*
 * strspn - find length of initial segment of s consisting entirely
 * of characters from accept
 */

size_t
strspn(s, accept)
Const char *s;
Const char *accept;
{
	register Const char *sscan;
	register Const char *ascan;
	register size_t count;

	count = 0;
	for (sscan = s; *sscan != '\0'; sscan++) {
		for (ascan = accept; *ascan != '\0'; ascan++)
			if (*sscan == *ascan)
				break;
		if (*ascan == '\0')
			return(count);
		count++;
	}
	return(count);
}
