/*
 * <strings.h> BSD-compat shim for the X11R1 libX11 port. The 1987 Xlib sources use
 * the 4.2BSD byte/string primitives. We declare them here (prototypes, not macros, so
 * they can't collide with variables named `index`) and implement them in
 * compat/xpsxcompat.c. MIT/X license (see copyright.h).
 */
#ifndef _XPSX_STRINGS_H_
#define _XPSX_STRINGS_H_

void  bcopy(const void *src, void *dst, int n);
void  bzero(void *dst, int n);
int   bcmp(const void *a, const void *b, int n);
char *index(const char *s, int c);
char *rindex(const char *s, int c);

#endif /* _XPSX_STRINGS_H_ */
