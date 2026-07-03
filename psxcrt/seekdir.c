#include <sys/types.h>
#include <dirent.h>

void seekdir(DIR *dirp, long loc)
{
	int 
		i,
		offset;

printf("in new seekdir loc X%ldX\n", loc);
	rewinddir(dirp);
	offset = loc / sizeof(struct dirent);

	for(i = 0; i < offset; i++)
		readdir(dirp);	
}
