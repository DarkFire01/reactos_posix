/*
 * area-based allocation built on malloc/free
 */

typedef struct Area {
	struct Block *free;	/* free list */
} Area;

#if 0
Area   *ainit ARGS((Area *ap));
void	afreeall ARGS((Area *ap));
void	ashrink ARGS((Area *ap));
void	aerror ARGS((Area *ap, const char *msg));
void	awarn ARGS((Area *ap, const char *msg));

Void   *alloc ARGS((size_t size, Area *ap));
Void   *aresize ARGS((Void *ptr, size_t size, Area *ap));
void	afree ARGS((Void *ptr, Area *ap));
#else
Area   *ainit ARGS((register Area *__ap));
void	afreeall ARGS((register Area *__ap));
Void   *alloc ARGS((size_t __size, register Area *__ap));
Void   *aresize ARGS((register Void *__ptr, size_t __size, Area *__ap));
void	afree ARGS((Void *__ptr, register Area *__ap));
void	aerror ARGS((Area *ap, Const char *msg));
#endif /* Xn 1992-07-20 */

