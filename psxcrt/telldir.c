#include <sys/types.h>
#include <dirent.h>

//Added by TimC
#include <unistd.h>

long telldir(DIR *dirp)
{
	char
		origDirName[NAME_MAX+1];
	long
		direntSize,
		offset;

	struct dirent 
		*origDirEntry,	
		*varDirEntry;

puts("in telldir");
	origDirEntry = readdir(dirp);
	strcpy(origDirName, origDirEntry->d_name);
	rewinddir(dirp);
	
	varDirEntry = readdir(dirp);
	direntSize = sizeof(struct dirent);	 
	offset = 0;

	if(origDirEntry == NULL) { 
puts("in origDirEntry");
		while(varDirEntry != NULL){ //End of directory
			varDirEntry = readdir(dirp);
			offset += direntSize;
		}
	}					
	else {
//puts("in else origDirEntry");
//printf("orig X%sX var X%sX\n", origDirName, varDirEntry->d_name);
		while(strcmp(origDirName, varDirEntry->d_name) &&
			varDirEntry != NULL) {
//printf("orig X%sX var X%sX\n", origDirName, varDirEntry->d_name);
			varDirEntry = readdir(dirp);
			offset += direntSize;
		}
	}
	
	if(offset > 0) //Do if telldir points to current dir obj.
		offset -= direntSize;
printf("offset = X%dX\n", offset);

	seekdir(dirp, offset);	
	return(offset);
}
