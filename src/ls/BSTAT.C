#include <sys/types.h>
#include <sys/stat.h>

/*
 * Lstat: Posix Implementation DF_AJ
 */

int bstat (const char *path, struct stat *buf)
{
	register int result;

#if 0
puts("in lstat");
#endif
	result = stat(path, buf);
#if 0
printf("mode %lo\n", (unsigned long) buf->st_mode);
#endif
	if ( S_ISDIR(buf->st_mode) ) {
#if 0
puts ("is dir");
#endif
		if ( !buf->st_size ) 
			buf->st_size = 16000;
	}
	return result;
}
