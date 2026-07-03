/* scandir(3) -- opendir/readdir into a sorted array. MIT. */
#include <sys/types.h>
#include <dirent.h>
extern void *malloc(size_t);
extern void *realloc(void *, size_t);
extern void  free(void *);
extern void  qsort(void *, size_t, size_t, int (*)(const void *, const void *));

int scandir(const char *dir, struct dirent ***namelist,
            int (*sel)(struct dirent *),
            int (*cmp)(const void *, const void *))
{
    DIR *d = opendir(dir);
    struct dirent *e, **list = 0, **nl, *copy;
    int n = 0, cap = 0;

    if (d == 0) return -1;
    while ((e = readdir(d)) != 0)
    {
        if (sel && !sel(e)) continue;
        if (n >= cap)
        {
            cap = cap ? cap * 2 : 16;
            nl = (struct dirent **)realloc(list, cap * sizeof(*list));
            if (nl == 0) break;
            list = nl;
        }
        copy = (struct dirent *)malloc(sizeof(struct dirent));
        if (copy == 0) break;
        *copy = *e;
        list[n++] = copy;
    }
    closedir(d);
    if (cmp && n > 0) qsort(list, n, sizeof(*list), cmp);
    *namelist = list;
    return n;
}
