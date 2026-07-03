/* $Id: table.h,v 3.2 89/03/27 15:46:52 egisin Exp $ */

/*
 * generic hashed associative table for commands and variables.
 */

struct table {
	Area   *areap;		/* area to allocate entries */
	int	size, free;	/* hash size (always 2^^n), free entries */
	struct	tbl **tbls;	/* hashed table items */
};

struct tbl {			/* table item */
	int	flag;		/* flags */
	int	type;		/* command type or base, see below */
	union {
		char *s;	/* string */
		long i;		/* integer */
		int (*f) ARGS((char **)); /* int function - Xn 1992-07-29 */
		struct op *t;	/* "function" tree */
	} val;			/* value */
	char	name[4];	/* name -- variable length */
};

/* flag bits */
#define	ALLOC	BIT(0)		/* val.s has been allocated */
#define	DEFINED	BIT(1)		/* is defined in block */
#define	ISSET	BIT(2)		/* has value, vp->val.[si] */
#define	SPECIAL	BIT(3)		/* PATH, IFS, SECONDS, etc */
#define	INTEGER	BIT(4)		/* val.i contains integer value */
#if WIN_NT
#define DOSIFY	BIT(6)		/* unPOSIXize variable (only used temporarily) */
#define POSIXIZE BIT(7)		/* POSIXized variable */
#endif /* Xn 1992-08-17 */
#define	RDONLY	BIT(8)		/* read-only variable */
#define	EXPORT	BIT(9)		/* exported variable */
#define	LOCAL	BIT(10)		/* for local typeset() */
#define	TRACE	BIT(11)		/* trace (-t) */
#define	FUNCT	BIT(12)		/* function */
#define	EXPALIAS BIT(13)	/* expanding this alias */

/* command types */
#define	CNONE	0		/* undefined */
#define	CSHELL	1		/* built-in */
#define	CFUNC	2		/* function */
#define	CEXEC	4		/* executable command */
#define	CALIAS	5		/* alias */
#define	CKEYWD	6		/* keyword */

#if 0
void tinit ARGS((struct table *, Area *)); /* initialize table */
unsigned int hash();		/* name hash function */
struct tbl *tsearch();		/* table lookup primative */
struct tbl *tenter();		/* table lookup/enter primative */
void tdelete();			/* mark tbl entry for deletion */
void twalk();			/* initialize walk of table */
struct tbl *tnext();		/* walk table returning table time */
struct tbl **tsort();		/* sort table entries by name */
#else
void tinit ARGS((register struct table *__tp, register Area *__ap)); /* initialize table */
unsigned int hash ARGS((register char *__n));		/* name hash function */
struct tbl *tsearch ARGS((register struct table *__tp, register char *__n, unsigned int __h));		/* table lookup primitive */
struct tbl *tenter ARGS((register struct table *__tp, register char *__n, unsigned int __h));		/* table lookup/enter primitive */
void tdelete ARGS((register struct tbl *__p));			/* mark tbl entry for deletion */
void twalk ARGS((register struct table *__tp));			/* initialize walk of table */
struct tbl *tnext ARGS((void));		/* walk table returning table time */
struct tbl **tsort ARGS((register struct table *__tp));		/* sort table entries by name */
#endif /* Xn 1992-07-20 */

/*
 * activation record for function blocks
 */
struct block {
	Area	area;		/* area to allocate things */
	int	argc;		/* current $# */
	char **	argv;		/* current $* */
	struct	table vars;	/* local variables */
	struct	table funs;	/* local functions */
#if 1
	char *	error;		/* error handler */
	char *	exit;		/* exit handler */
#else
	struct	trap error, exit;
#endif
	struct	block *next;	/* enclosing block */
};

Extern	struct block globals;	/* global variables and functions */
Extern	struct table commands;	/* hashed commands */
Extern	struct table builtins;	/* built-in commands */
Extern	struct table lexicals;	/* keywords and aliases */
Extern	struct table homedirs;	/* homedir() cache */

struct builtin {
	char   *name;
	int  (*func) ARGS((char**)); /* Xn 1992-07-29 */
};
Extern Const struct builtin shbuiltins [], kshbuiltins [];

/* var spec values */
#define	V_NONE	0
#define	V_PATH	1
#define	V_IFS	2
#define	V_SECONDS 3
#define	V_OPTIND 4
#define	V_MAIL	5
#define	V_MAILPATH 6
#define	V_RANDOM 7

Extern	Area   *lastarea;	/* area of last variable/function looked up */
Extern	char   *path;		/* PATH value */
Extern	char   *prompt;		/* PS1 or PS2 */

#if 0
void	newblock();
void	popblock();
struct tbl *global(/* char *s */);
struct tbl *local(/* char *s */);
struct tbl *typeset(/* char *var; int set, clr */);
struct tbl *setvar(/* struct tbl *vdst, *vsrc */);
struct tbl *strint(/* struct tbl *vdst, *vsrc */);
long	intval(/* struct tbl *vp */);
void	setint(/* struct tbl *vp; long n */);
char   *strval(/* struct tbl *vp */);
void	setstr(/* struct tbl *vp; char *s */);
void	unset(/* struct tbl *vp */);
int	import(/* char *s */);
char  **makenv();
int	isassign(/* char *s */);
#else
void	newblock ARGS((void));
void	popblock ARGS((void));
struct tbl *global ARGS((register char *__n));
struct tbl *local ARGS((register char *__n));
struct tbl *typeset ARGS((register char *__var, int __set, int __clr));
struct tbl *strint ARGS((register struct tbl *__vq, register struct tbl *__vp));
long	intval ARGS((register struct tbl *__vp));
void	setint ARGS((register struct tbl *__vq, long __n));
char   *strval ARGS((register struct tbl *__vp));
void	setstr ARGS((register struct tbl *__vq, char *__s));
void	unset ARGS((register struct tbl *__vp));
int	import ARGS((char *__thing));
char  **makenv ARGS((void));
int	isassign ARGS((register char *__s));
#endif /* Xn 1992-07-20 */

