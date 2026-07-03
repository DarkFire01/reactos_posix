/*
 * <langinfo.h> -- psx-minimal shim.  psxcrt has no nl_langinfo(); Motif's
 * Scale.c calls it only for CODESET/RADIXCHAR-style queries and tolerates the
 * "C" locale answers.  We return fixed C-locale strings.  MIT (ReactOS POSIX).
 */
#ifndef _PSX_LANGINFO_H_
#define _PSX_LANGINFO_H_

#include <nl_types.h>   /* nl_item (compat/nl_types.h) */
#include <locale.h>     /* struct lconv/localeconv -- Scale.c reaches lconv only
                         * via this path on non-Darwin (it skips <locale.h>). */

#define CODESET     1
#define RADIXCHAR   2
#define THOUSEP     3
#define D_T_FMT     4
#define D_FMT       5
#define T_FMT       6
#define AM_STR      7
#define PM_STR      8

static __inline char *nl_langinfo(nl_item __item)
{
    switch (__item) {
    case CODESET:   return (char *)"ISO8859-1";
    case RADIXCHAR: return (char *)".";
    case THOUSEP:   return (char *)"";
    default:        return (char *)"";
    }
}

#endif /* _PSX_LANGINFO_H_ */
