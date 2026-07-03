/* BSD <sys/cdefs.h> compat: the __P prototype macro + decl guards. MIT. */
#pragma once
#if defined(__STDC__) || defined(__cplusplus)
#define __P(protos)     protos
#define __CONCAT(x,y)   x ## y
#define __STRING(x)     #x
#else
#define __P(protos)     ()
#define __CONCAT(x,y)   x/**/y
#define __STRING(x)     "x"
#define const
#define volatile
#endif
#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#ifndef __dead
#define __dead
#endif
#ifndef __pure
#define __pure
#endif
