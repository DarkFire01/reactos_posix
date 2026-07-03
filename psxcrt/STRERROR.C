#include <string.h>

/*
 * strerror - map error number to descriptive string
 *
 * This version is obviously somewhat Unix-specific.
 */
char *
strerror(errno)
int errno;
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	if (errno > 0 && errno < sys_nerr)
		return(sys_errlist[errno]);
	else
		return("unknown error");
}
