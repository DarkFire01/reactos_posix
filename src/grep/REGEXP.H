/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
#define NSUBEXP  10
typedef struct regexp {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;

#if __STDC__
extern regexp *regcomp (char *);
extern int regexec (register regexp *, register char *);
extern void regsub (regexp *, char *, char *);
extern void regerror (const char *);
#else
extern regexp *regcomp();
extern int regexec();
extern void regsub();
extern void regerror();
#endif
