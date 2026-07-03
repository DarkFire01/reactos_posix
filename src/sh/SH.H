/*
 * Public Domain Bourne/Korn shell
 */

/* $Id: sh.h,v 3.3 89/03/27 15:46:34 egisin Exp $ */

#include "config.h"

/* allow for non-Unix linkers. main.c has a "#define Extern " */
#ifndef Extern
#define	Extern	extern
#endif

#ifndef SHELL
#define	SHELL	"/bin/sh"	/* shell to exec scripts */
#endif

/* some people object to this use of __STDC__ */
#if __STDC__
#define	ARGS(args)	args	/* prototype declaration */
#define	Void	void		/* generic pointer */
#define	Const	const		/* constant data */
#define	Volatile volatile	/* you know */
#else
#define	ARGS(args)	()	/* K&R declaration */
#define	Void	char
#define	Const	
#define	Volatile 
#endif

#ifdef WIN_NT
#undef leave
#endif /* Xn 1993-05-13 */

#ifdef _ULTRIX			/* Ultrix 2.x gets dup2 wrong */
int dup2 ARGS ((int, int));
/* assumes we don't want dup2 return value */
#define	dup2(ofd, nfd) \
		(void) ((dup2)(ofd, nfd), fcntl(nfd, F_SETFD, 0))
#endif

#if defined(EMACS) || defined(VI)
#define	EDIT
#endif

typedef int bool_t;
#define	FALSE	0
#define	TRUE	1

#define	sizeofN(type, n) (sizeof(type) * (n))
#define	BIT(i)	(1<<(i))	/* define bit in flag */

#define	NUFILE	10		/* Number of user-accessible files */
#define	FDBASE	10		/* First file usable by Shell */

/* you're not going to run setuid shell scripts, are you? */
#define	eaccess(path, mode)	access(path, mode)

#define	MAGIC	(char)0x80	/* prefix for ~*?[ during expand */
#define	NOT	'!'	/* might use ^ */

#define	LINE	256		/* input line size */
#define	PATH	256		/* pathname size */

Extern	pid_t	kshpid;		/* $$, shell pid - Xn 1992-07-29 */
Extern	int	exstat;		/* exit status */
Extern	int	async;		/* $!, last &'d pid */

/*
 * Area-based allocation built on malloc/free
 */

#if 0
typedef struct Area {
	struct Block *free;	/* free list */
} Area;
#else
#include "alloc.h"
#endif /* Xn 1992-07-20 */

extern	Area	aperm;		/* permanent object space */
#define	APERM	&aperm
#define	ATEMP	&e.area
#if 0

Area   *ainit ARGS((Area *ap));		/* initialize Area */
void	afreeall ARGS((Area *ap));	/* free Area's contents */
void	ashrink ARGS((Area *ap));	/* unimplimented */
void	aerror ARGS((Area *ap, const char *msg)); /* error handler */

Void   *alloc ARGS((size_t size, Area *ap)); /* alloc object from Area */
Void   *aresize ARGS((Void *ptr, size_t size, Area *ap)); /* shrink object */
void	afree ARGS((Void *ptr, Area *ap)); /* free ojbect in Area */
#endif /* Xn 1992-07-20 */

/*
 * parsing & execution environment
 */
Extern	struct	env {
	int	type;			/* enviroment type - see below */
	Area	area;			/* temporary allocation area */
	struct	block *loc;		/* local variables and functions */
	short  *savefd;			/* original redirected fd's */
	struct	env *oenv;		/* link to previous enviroment */
	jmp_buf	jbuf;			/* long jump back to env creator */
	int	interactive;		/* fd's 0,1,2 are tty */
	struct temp *temps;		/* temp files */
} e;

#define	E_NONE	0		/* dummy enviroment */
#define	E_PARSE	1		/* parsing command # */
#define	E_EXEC	2		/* executing command tree */
#define	E_LOOP	3		/* executing for/while # */
#define	E_TCOM	5		/* executing simple command */
#define	E_FUNC	6		/* executing function */
#define	E_ERRH	7		/* general error handler # */
/* # indicates env has valid jbuf */

/*
 * flags
 */
#define	FEXPORT	FLAG('a')	/* -a: allexport */
#define	FERREXIT FLAG('e')	/* -e: errexit (quit on error) */
#define	FBGNICE	29		/* bgnice */
#define	FEMACS 30		/* emacs command editing */
#define	FIGNEOF	27		/* ignoreeof (eof does not exit) */
#define	FHASHALL FLAG('h')	/* -h: trackall, hashall */
#define	FTALKING FLAG('i')	/* -i: interactive (talking type wireless) */
#define	FKEYWORD FLAG('k')	/* -k: keyword (name=value anywhere) */
#define	FMARKDIRS 28		/* markdirs */
#define	FMONITOR FLAG('m')	/* -m: monitor */
#define	FNOEXEC	FLAG('n')	/* -n: noexec */
#define	FNOGLOB	FLAG('f')	/* -f: noglob */
#define	FPRIVILEGED FLAG('p')	/* -p: privileged */
#define	FSTDIN	FLAG('s')	/* -s (invocation): parse stdin */
#define	FNOUNSET FLAG('u')	/* -u: nounset (unset vars is error) */
#define	FVERBOSE FLAG('v')	/* -v: verbose (echo input) */
#define	FVI 31			/* vi command editing */
#define	FXTRACE	FLAG('x')	/* -x: (execute) xtrace */

#define	FLAG(c)	(1 + c - 'a')	/* map char to flags index */
#define	FLAGS	32
Extern	char flag [FLAGS];
#if 0
int	option ARGS((Const char *name));
#else
int	option ARGS((Const char *__n));
#endif /* Xn 1992-07-20 */
char   *getoptions ARGS((void));
void	printoptions ARGS((void));

extern	char	null [];	/* null value for variable */

/*
 * other functions
 */
#if 0
char   *search();
struct tbl *findcom();
char   *strsave ARGS((char *, Area *));
char   *ulton ARGS((unsigned long n, int base));
int	xstrcmp();
void	qsortp ARGS((void **base, size_t n, int (*compare)(void *, void *)));
long	evaluate ARGS((Const char *expr));
void	resetopts();
void	histsave();
void	histlist();
#else
char   *search ARGS((char *__name, char *__path, int __mode));
struct tbl *findcom ARGS((char *__name, int __insert));
char   *strsave ARGS((register char *__s, Area *__ap));
char   *ulton ARGS((register unsigned long __n, int __base));
int	xstrcmp ARGS((Void *__p1, Void *__p2));
void	qsortp ARGS((Void **__base, size_t __n, int (*__compare)(Void *, Void *)));
long	evaluate ARGS((Const char *__expr));
void	resetopts ARGS((void));
void	histsave ARGS((char *__cmd));
#endif /* Xn 1992-07-20 */

void	j_init ARGS((void));
void	j_exit ARGS((void));
void	j_notify ARGS((void));
#if 0
void	j_kill ARGS((int job, int sig));
#else
void	j_kill ARGS((int __job, int __sig));
#endif /* Xn 1992-07-20 */
#ifdef JOBS
void	j_change ARGS((void));
#if 0
int	j_resume ARGS((int job, int bg));
#else
int	j_resume ARGS((int __job, int __bg));
#endif /* Xn 1992-07-20 */
#endif

/*
 * error handling
 */
#if 0
void	leave();	/* abort shell (or fail in subshell) */
#else
void	leave ARGS((int __rv));	/* abort shell (or fail in subshell) */
#endif /* Xn 1992-07-20 */

/*
 * library functions
 */
#if 0
typedef	void (*handler_t)();	/* signal handler */
#else
typedef	void (*handler_t) ARGS((int));	/* signal handler */
#endif /* Xn 1992-07-16 */

/* temp/here files. the file is removed when the struct is freed */
struct	temp {
	struct	temp * next;
	char   *name;
};
#if 0
struct temp *maketemp ARGS((Area *ap));
#else
struct temp *maketemp ARGS((Area *__ap));
#endif /* Xn 1992-07-20 */

/*
 * stdio and our IO routines
 */

#ifdef	BUFSIZ			/* <stdio.h> included? */
extern	FILE *	shf [NUFILE];	/* map shell fd to FILE */
#endif
#if 0
void	fopenshf();
void	flushshf();
#else
void	fopenshf ARGS((int fd));
void	flushshf ARGS((int fd));
#endif /* Xn 1992-07-20 */

#undef	stdin
#undef	stdout

#define	stdin	shf[0]		/* standard input */
#define	stdout	shf[1]		/* standard output */
#define	shlout	shf[2]		/* shell output */

#if 0
int	shellf ARGS((Const char *fmt, ...)); /* fprintf(shlout, ); */
void	errorf ARGS((Const char *fmt, ...)); /* fprintf(shlout, ); error(); */
#else
int	shellf ARGS((Const char *__fmt, ...)); /* fprintf(shlout, ); */
void	errorf ARGS((Const char *__fmt, ...)); /* fprintf(shlout, ); error(); */
#endif /* Xn 1992-07-20 */

/*
 * IO control
 */
extern	int ttyfd;		/* tty fd (original fd 0) */

#if 0
int	savefd ARGS((int fd));	/* save user fd */
void	restfd ARGS((int fd, int ofd));
void	openpipe ARGS((int [2]));
void	closepipe ARGS((int [2]));
#else
int	savefd ARGS((int __fd));	/* save user fd */
void	restfd ARGS((int __fd, int __ofd));
void	openpipe ARGS((register int *__pv));
void	closepipe ARGS((register int *__pv));
#endif /* Xn 1992-07-20 */

/*
 * trap handlers
 */
typedef struct trap {
	int	signal;		/* signal number */
	char   *name;		/* short name */
	char   *mess;		/* descriptive name */
	char   *trap;		/* trap command */
#if 0
	int	Volatile set;	/* trap pending */
#else
	Volatile int	set;	/* trap pending */
#endif /* Xn 1992-08-13 */
	int	ourtrap;	/* not ignored (?) */
	int	sig_dfl;	/* originally SIG_DFL */
} Trap;

#ifndef	SIGKILL
#include <signal.h>
#endif	/* SIGKILL */
#ifdef	NSIG
#define	SIGNALS	NSIG
#else
#define	SIGNALS	32
#endif	/* NSIG */

#if 0
Extern	int Volatile trap;	/* traps pending? */
#else
Extern	Volatile int trap;	/* traps pending? */
#endif /* Xn 1992-08-13 */
extern	Trap	sigtraps[SIGNALS];
#if 0
Trap    *gettrap ARGS((char *)); /* search for struct trap by number or name */
void	trapsig ARGS((int sig)); /* trap signal handler */
#else
Trap    *gettrap ARGS((char *__name)); /* search for struct trap by number or name */
void	trapsig ARGS((int __i)); /* trap signal handler */
#endif /* Xn 1992-07-20 */

/*
 * fast character classes
 */
#define	C_ALPHA	0x01		/* a-z_A-Z */
#define	C_DIGIT	0x02		/* 0-9 */
#define	C_LEX1	0x04		/* \0 \t\n|&;<>() */
#define	C_VAR1	0x08		/* *@#!$-? */
#define	C_SUBOP	0x40		/* "=-+?#%" */
#define	C_IFS	0x80		/* $IFS */

extern	char ctypes [];
void	initctypes ARGS((void));
#if 0
void	setctypes ARGS((Const char*, int type));
#else
void	setctypes ARGS((register Const char *__s, register int __t));
#endif /* Xn 1992-07-20 */

#define	ctype(c, t)	!!(ctypes[(unsigned char)(c)]&(t))
#define	letter(c)	ctype(c, C_ALPHA)
#define	digit(c)	ctype(c, C_DIGIT)
#define	letnum(c)	ctype(c, C_ALPHA|C_DIGIT)

/*
 * 91-07-06 <sjg@sun0>
 * use my simple debug tracing... 
 */
#include "trace.h"

