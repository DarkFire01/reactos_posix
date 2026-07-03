/*
 * nl_types.h -- psx shim: no message catalogs on the subsystem yet.
 * catgets() returns the caller's default string, the standard graceful
 * fallback, so CDE programs show their built-in (English) messages.
 * Implementations in compat/xpsxcompat.c.  MIT/X license.
 */
#ifndef _PSX_NL_TYPES_H_
#define _PSX_NL_TYPES_H_

typedef void *nl_catd;
typedef int nl_item;

#define NL_SETD		1
#define NL_CAT_LOCALE	1

extern nl_catd catopen(const char *name, int oflag);
extern char *catgets(nl_catd catd, int set_id, int msg_id, const char *s);
extern int catclose(nl_catd catd);

#endif /* _PSX_NL_TYPES_H_ */
