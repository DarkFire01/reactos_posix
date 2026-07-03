/* lstat -> stat (psxdll exports no lstat; the fs has no POSIX symlinks). MIT. */
#include <sys/types.h>
#include <sys/stat.h>
extern int stat(const char *, struct stat *);
int lstat(const char *path, struct stat *buf) { return stat(path, buf); }
