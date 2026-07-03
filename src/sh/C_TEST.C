/*
 * test(1); version 7-like  --  author Erik Baalbergen
 * modified by Eric Gisin to be used as built-in.
 * modified by Arnold Robbins to add SVR3 compatibility
 * (-x -c -b -p -u -g -k) plus Korn's -L -nt -ot -ef and new -S (socket).
 */

static char *RCSid = "$Id: c_test.c,v 3.3 89/03/27 15:47:25 egisin Exp $";

#include <stddef.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#if _POSIX_SOURCE
#include <unistd.h>
#endif /* Xn 1992-07-20 */
#if 0 && XN
#include <stdio.h>
#endif /* Xn 1993-06-15 */
#include "sh.h"

/* test(1) accepts the following grammar:
	oexpr	::= aexpr | aexpr "-o" oexpr ;
	aexpr	::= nexpr | nexpr "-a" aexpr ;
	nexpr	::= primary ! "!" primary
	primary	::= unary-operator operand
		| operand binary-operator operand
		| operand
		| "(" oexpr ")"
		;
	unary-operator ::= "-r"|"-w"|"-x"|"-f"|"-d"|"-c"|"-b"|"-p"|
		"-u"|"-g"|"-k"|"-s"|"-t"|"-z"|"-n"|"-o"|"-O"|"-G"|"-L"|"-S";

	binary-operator ::= "="|"!="|"-eq"|"-ne"|"-ge"|"-gt"|"-le"|"-lt"|
			"-nt"|"-ot"|"-ef";
	operand ::= <any legal UNIX file name>
*/

#define EOI	0
#define FILRD	1
#define FILWR	2
#define FILREG	3
#define FILID	4
#define FILGZ	5
#define FILTT	6
#define STZER	7
#define STNZE	8
#define STEQL	9
#define STNEQ	10
#define INTEQ	11
#define INTNE	12
#define INTGE	13
#define INTGT	14
#define INTLE	15
#define INTLT	16
#define UNOT	17
#define BAND	18
#define BOR	19
#define LPAREN	20
#define RPAREN	21
#define OPERAND	22
#define FILEX	23
#define FILCDEV	24
#define FILBDEV	25
#define FILFIFO	26
#define FILSETU	27
#define FILSETG	28
#define FILSTCK	29
#define FILSYM	30
#define FILNT	31
#define FILOT	32
#define FILEQ	33
#define FILSOCK	34
#define	FILUID	35
#define	FILGID	36
#define	OPTION	37

#define UNOP	1
#define BINOP	2
#define BUNOP	3
#define BBINOP	4
#define PAREN	5

struct t_op {
	char *op_text;
	short op_num, op_type;
} Const ops [] = {
	{"-r",	FILRD,	UNOP},
	{"-w",	FILWR,	UNOP},
	{"-x",	FILEX,	UNOP},
	{"-f",	FILREG,	UNOP},
	{"-d",	FILID,	UNOP},
	{"-c",	FILCDEV,UNOP},
	{"-b",	FILBDEV,UNOP},
	{"-p",	FILFIFO,UNOP},
	{"-u",	FILSETU,UNOP},
	{"-g",	FILSETG,UNOP},
	{"-k",	FILSTCK,UNOP},
	{"-s",	FILGZ,	UNOP},
	{"-t",	FILTT,	UNOP},
	{"-z",	STZER,	UNOP},
	{"-n",	STNZE,	UNOP},
#if 0				/* conficts with binary -o */
	{"-o",	OPTION,	UNOP},
#endif
	{"-U",	FILUID,	UNOP},
	{"-G",	FILGID,	UNOP},
	{"-L",	FILSYM,	UNOP},
	{"-S",	FILSOCK,UNOP},
	{"=",	STEQL,	BINOP},
	{"!=",	STNEQ,	BINOP},
	{"-eq",	INTEQ,	BINOP},
	{"-ne",	INTNE,	BINOP},
	{"-ge",	INTGE,	BINOP},
	{"-gt",	INTGT,	BINOP},
	{"-le",	INTLE,	BINOP},
	{"-lt",	INTLT,	BINOP},
	{"-nt",	FILNT,	BINOP},
	{"-ot",	FILOT,	BINOP},
	{"-ef",	FILEQ,	BINOP},
	{"!",	UNOT,	BUNOP},
	{"-a",	BAND,	BBINOP},
	{"-o",	BOR,	BBINOP},
	{"(",	LPAREN,	PAREN},
	{")",	RPAREN,	PAREN},
	{0,	0,	0}
};

char **t_wp;
struct t_op Const *t_wp_op;

#if 0
static void syntax ();
#else
extern int getn ARGS((char *)); /* in misc.c */
int oexpr ARGS((int));
int t_lex ARGS((register char *));
int aexpr ARGS((int));
int nexpr ARGS((int));
int primary ARGS((int));
int filstat ARGS((char *, int));
int newerf ARGS((char *, char *));
int olderf ARGS((char *, char *));
int equalf ARGS((char *, char *));
static void syntax ARGS((char *, char *));
#endif /* Xn 1992-07-20 */

int
#if __STDC__
c_test(char **wp)
#else
c_test(wp)
	char **wp;
#endif /* Xn 1992-07-16 */
{
	int	res;

	t_wp = wp+1;
	if (strcmp(wp[0], "[") == 0) {
		while (*wp != NULL)
			wp++;
		if (strcmp(*--wp, "]") != 0)
			errorf("[: missing ]\n");
		*wp = NULL;
	}
	res = *t_wp == NULL || !oexpr(t_lex(*t_wp));

	if (*t_wp != NULL && *++t_wp != NULL)
		syntax(*t_wp, "unknown operand");

	return res;
}

static void
#if __STDC__
syntax(char *op, char *msg)
#else
syntax(op, msg)
	char	*op;
	char	*msg;
#endif /* Xn 1992-07-16 */
{
	if (op && *op)
		errorf("test: %s: %s\n", op, msg);
	else
		errorf("test: %s\n", msg);
}

int
#if __STDC__
oexpr(int n)
#else
oexpr(n)
	int n;
#endif /* Xn 1992-07-16 */
{
	int res;

	res = aexpr(n);
	if (t_lex(*++t_wp) == BOR)
		return oexpr(t_lex(*++t_wp)) || res;
	t_wp--;
	return res;
}

int
#if __STDC__
aexpr(int n)
#else
aexpr(n)
	int n;
#endif /* Xn 1992-07-16 */
{
	int res;

	res = nexpr(n);
	if (t_lex(*++t_wp) == BAND)
		return aexpr(t_lex(*++t_wp)) && res;
	t_wp--;
	return res;
}

int
#if __STDC__
nexpr(int n)
#else
nexpr(n)
	int n;			/* token */
#endif /* Xn 1992-07-16 */
{
	if (n == UNOT)
		return !nexpr(t_lex(*++t_wp));
	return primary(n);
}

int
#if __STDC__
primary(int n)
#else
primary(n)
	int n;			/* token */
#endif /* Xn 1992-07-16 */
{
	register char *opnd1, *opnd2;
	int res;

	if (n == EOI)
		syntax(NULL, "argument expected");
	if (n == LPAREN) {
		res = oexpr(t_lex(*++t_wp));
		if (t_lex(*++t_wp) != RPAREN)
			syntax(NULL, "closing paren expected");
		return res;
	}
	if (t_wp_op && t_wp_op->op_type == UNOP) {
		/* unary expression */
		if (*++t_wp == NULL && n != FILTT)
			syntax(t_wp_op->op_text, "argument expected");
		switch (n) {
		  case OPTION:
			return flag[option(*t_wp)];
		  case STZER:
			return strlen(*t_wp) == 0;
		  case STNZE:
			return strlen(*t_wp) != 0;
		  case FILTT:
			if (!digit(**t_wp))
				return filstat("0", n);
		  default:	/* all other FIL* */
			return filstat(*t_wp, n);
		}
	}
	opnd1 = *t_wp;
	(void) t_lex(*++t_wp);
	if (t_wp_op && t_wp_op->op_type == BINOP) {
		struct t_op Const *op = t_wp_op;

		if ((opnd2 = *++t_wp) == (char *)0)
			syntax(op->op_text, "argument expected");
		
#if 0 && XN
		(void) fprintf(stderr, "c_test.c/primary - opnd1: \"%s\"; opnd2: \"%s\"\n", opnd1, opnd2);
		(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
		switch (op->op_num) {
		case STEQL:
			return strcmp(opnd1, opnd2) == 0;
		case STNEQ:
			return strcmp(opnd1, opnd2) != 0;
		case INTEQ:
			return evaluate(opnd1) == evaluate(opnd2);
		case INTNE:
			return evaluate(opnd1) != evaluate(opnd2);
		case INTGE:
			return evaluate(opnd1) >= evaluate(opnd2);
		case INTGT:
			return evaluate(opnd1) > evaluate(opnd2);
		case INTLE:
			return evaluate(opnd1) <= evaluate(opnd2);
		case INTLT:
			return evaluate(opnd1) < evaluate(opnd2);
		case FILNT:
			return newerf (opnd1, opnd2);
		case FILOT:
			return olderf (opnd1, opnd2);
		case FILEQ:
			return equalf (opnd1, opnd2);
		}
	}
	t_wp--;
	return strlen(opnd1) > 0;
}

int
#if __STDC__
filstat(char *nm, int mode)
#else
filstat(nm, mode)
	char *nm;
	int mode;
#endif /* Xn 1992-07-16 */
{
	struct stat s;
	
	switch (mode) {
	case FILRD:
#if _POSIX_SOURCE
		return eaccess(nm, R_OK) == 0;
#else
		return eaccess(nm, 4) == 0;
#endif /* Xn 1992-07-31 */
	case FILWR:
#if _POSIX_SOURCE
		return eaccess(nm, W_OK) == 0;
#else
		return eaccess(nm, 2) == 0;
#endif /* Xn 1992-07-31 */
	case FILEX:
#if _POSIX_SOURCE
		return eaccess(nm, X_OK) == 0;
#else
		return eaccess(nm, 1) == 0;
#endif /* Xn 1992-07-31 */
	case FILREG:
		return stat(nm, &s) == 0 && (s.st_mode & S_IFMT) == S_IFREG;
	case FILID:
		return stat(nm, &s) == 0 && (s.st_mode & S_IFMT) == S_IFDIR;
	case FILCDEV:
		return stat(nm, &s) == 0 && (s.st_mode & S_IFMT) == S_IFCHR;
	case FILBDEV:
		return stat(nm, &s) == 0 && (s.st_mode & S_IFMT) == S_IFBLK;
	case FILFIFO:
#ifdef S_IFIFO
		return stat(nm, &s) == 0 && (s.st_mode & S_IFMT) == S_IFIFO;
#else
		return 0;
#endif
	case FILSETU:
		return stat(nm, &s) == 0 && (s.st_mode & S_ISUID) == S_ISUID;
	case FILSETG:
		return stat(nm, &s) == 0 && (s.st_mode & S_ISGID) == S_ISGID;
	case FILSTCK:
#ifdef S_ISVTX
		return stat(nm, &s) == 0 && (s.st_mode & S_ISVTX) == S_ISVTX;
#else
		return 0;
#endif /* Xn 1992-07-16 */
	case FILGZ:
		return stat(nm, &s) == 0 && s.st_size > 0L;
	case FILTT:
		return isatty(getn(nm));
	  case FILUID:
		return stat(nm, &s) == 0 && s.st_uid == geteuid();
	  case FILGID:
		return stat(nm, &s) == 0 && s.st_gid == getegid();
#ifdef S_IFLNK
	case FILSYM:
		return lstat(nm, &s) == 0 && (s.st_mode & S_IFMT) == S_IFLNK;
#endif
#ifdef S_IFSOCK
	case FILSOCK:
		return stat(nm, &s) == 0 && (s.st_mode & S_IFMT) == S_IFSOCK;
#endif
	  default:
		return 1;
	}
}

int
#if __STDC__
t_lex(register char *s)
#else
t_lex(s)
	register char *s;
#endif /* Xn 1992-07-16 */
{
	register struct t_op Const *op = ops;

	if (s == 0) {
		t_wp_op = (struct t_op *)0;
		return EOI;
	}
	while (op->op_text) {
		if (strcmp(s, op->op_text) == 0) {
			t_wp_op = op;
			return op->op_num;
		}
		op++;
	}
	t_wp_op = (struct t_op *)0;
	return OPERAND;
}

int
#if __STDC__
newerf(char *f1, char *f2)
#else
newerf (f1, f2)
	char *f1, *f2;
#endif /* Xn 1992-07-16 */
{
	struct stat b1, b2;

	return (stat (f1, &b1) == 0 &&
		stat (f2, &b2) == 0 &&
		b1.st_mtime > b2.st_mtime);
}

int
#if __STDC__
olderf(char *f1, char *f2)
#else
olderf (f1, f2)
	char *f1, *f2;
#endif /* Xn 1992-07-16 */
{
	struct stat b1, b2;

	return (stat (f1, &b1) == 0 &&
		stat (f2, &b2) == 0 &&
		b1.st_mtime < b2.st_mtime);
}

int
#if __STDC__
equalf (char *f1, char *f2)
#else
equalf (f1, f2)
	char *f1, *f2;
#endif /* Xn 1992-07-16 */
{
	struct stat b1, b2;

	return (stat (f1, &b1) == 0 &&
		stat (f2, &b2) == 0 &&
		b1.st_dev == b2.st_dev &&
		b1.st_ino == b2.st_ino);
}

