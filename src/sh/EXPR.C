/*
 * Korn expression evaluation
 */

static char *RCSid = "$Id: expr.c,v 3.2 89/03/27 15:50:20 egisin Exp $";

#include <stddef.h>
#include <errno.h>
#include <setjmp.h>
#if 0 && XN
#include <stdio.h>
#endif /* Xn 1993-06-15 */
#include "sh.h"
#include "table.h"

#define	ef	else if		/* fashion statement */

#define	VAR	0x01
#define	LIT	0x02
#define	LEQ	0x03
#define	LNE	0x04
#define	LLE	0x05
#define	LGE	0x06

#if 0
static void token();		/* read next token */
#else
static void token ARGS((void));		/* read next token */
#endif /* Xn 1992-07-20 */
static Const char *expression;	/* expression being evaluated */
static Const char *tokp;	/* lexical position */
static int tok;			/* token from token() */
static struct tbl *val;		/* value from token() */

#if 0
static struct tbl *tempvar(), *intvar();
static struct tbl *asn(), *e6(), *e5(), *e3(), *e2(), *e0();
#else
static struct tbl *tempvar ARGS((void)), *intvar ARGS((register struct tbl *));
static struct tbl *asn ARGS((void)), *e6 ARGS((void)), *e5 ARGS((void));
static struct tbl *e3 ARGS((void)), *e2 ARGS((void)), *e0 ARGS((void));
#endif /* Xn 1992-07-20 */

/*
 * parse and evalute expression
 */
void
#if __STDC__
evalerr(char *err)
#else
evalerr(err)
	char *err;
#endif /* Xn 1992-07-17 */
{
	errorf("%s: %s\n", expression, err);
}

long
#if __STDC__
evaluate(Const char *expr)
#else
evaluate(expr)
	Const char *expr;
#endif /* Xn 1992-07-17 */
{
	struct tbl *v;

	expression = tokp = expr;
#if 0 && XN
	(void) fprintf(stderr, "expr.c/evaluate - tokp: \"%s\"\n", tokp);
	(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
	token();
	v = intvar(asn());
	if (!(tok == 0))
#if 0 && XN
		evalerr("evaluate: bad expression");
#else
		evalerr("bad expression");
#endif /* Xn 1993-06-15 */
	return v->val.i;
}

static struct tbl *
#if __STDC__
asn(void)
#else
asn()
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vl, *vr;

	vr = vl = e6();
	if ((tok == '=')) {
		Area * olastarea = lastarea;
		token();
		if ((vl->flag&RDONLY)) /* assign to rvalue */
			evalerr("bad assignment");
		vr = intvar(asn());
		lastarea = olastarea;
		setint(vl, vr->val.i);
		if ((vl->flag&INTEGER) && vl->type == 0) /* default base? */
			vl->type = vr->type;
	}
	return vr;
}

static struct tbl *
#if __STDC__
e6(void)
#else
e6()
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vl, *vr;

	vl = e5();
	while ((tok == LEQ) || (tok == LNE)) {
		int op = tok;
		token();
		vl = intvar(vl);
		vr = intvar(e5());
		vl->val.i = vl->val.i == vr->val.i;
		if (op == LNE)
			vl->val.i = ! vl->val.i;
	}
	return vl;
}

static struct tbl *
#if __STDC__
e5(void)
#else
e5()
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vl, *vr;

	vl = e3();
#if 0 && XN
	(void) fprintf(stderr, "expr.c/e5 - LLT: %d; LLE: %d; LGE: %d; LGT: %d; tok: %d\n", '<', LLE, LGE, '>', tok);
	(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
	while ((tok == LLE) || (tok == '<') || (tok == '>') || (tok == LGE)) {
		int op = tok;
		token();
		vl = intvar(vl);
		vr = intvar(e3());
		if (op == LLE)
			vl->val.i = vl->val.i <= vr->val.i;
		ef (op == '<')
			vl->val.i = vl->val.i < vr->val.i;
		ef (op == LGE)
			vl->val.i = vl->val.i >= vr->val.i;
		ef (op == '>')
			vl->val.i = vl->val.i > vr->val.i;
	}
	return vl;
}

static struct tbl *
#if __STDC__
e3(void)
#else
e3()
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vl, *vr;

	vl = e2();
	while ((tok == '+') || (tok == '-')) {
		int op = tok;
		token();
		vl = intvar(vl);
		vr = intvar(e2());
		if (op == '+')
			vl->val.i += vr->val.i;
		ef (op == '-')
			vl->val.i -= vr->val.i;
	}
	return vl;
}

static struct tbl *
#if __STDC__
e2(void)
#else
e2()
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vl, *vr;

	vl = e0();
	while ((tok == '*') || (tok == '/') || (tok == '%')) {
		int op = tok;
		token();
		vl = intvar(vl);
		vr = intvar(e0());
		if (op != '*' && vr->val.i == 0)
			evalerr("zero divisor");
		if (op == '*')
			vl->val.i *= vr->val.i;
		ef (op == '/')
			vl->val.i /= vr->val.i;
		ef (op == '%')
			vl->val.i %= vr->val.i;
	}
	return vl;
}

static struct tbl *
#if __STDC__
e0(void)
#else
e0()
#endif /* Xn 1992-07-17 */
{
	register struct tbl *v;

	if ((tok == '!') || (tok == '-')) {
		int op = tok;
		token();
		v = intvar(e0());
		if (op == '!')
			v->val.i = !v->val.i;
		ef (op == '-')
			v->val.i = -v->val.i;
	} else
	if ((tok == '(')) {
		token();
		v = asn();
		if (!(tok == ')'))
			evalerr("missing )");
		token();
	} else
	if ((tok == VAR) || (tok == LIT)) {
		v = val;
		token();
	} else
#if 0 && XN
		evalerr("e0: bad expression");
#else
		evalerr("bad expression");
#endif /* Xn 1993-06-15 */
	return v;
}

static void
#if __STDC__
token(void)
#else
token()
#endif /* Xn 1992-07-17 */
{
	register char *cp = (char *) tokp;
	register int c, c2;

#if 0 && XN
	(void) fprintf(stderr, "expr.c/token - tokp: \"%s\"\n", tokp);
	(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
	/* skip white space */
	do c = *cp++;	while (c != '\0' && (c == ' ' || c == '\t'));
	tokp = cp-1;
#if 0 && XN
	(void) fprintf(stderr, "expr.c/token - c: %02X; tokp: \"%s\"\n", c, tokp);
	(void) fflush(stderr);
#endif /* Xn 1993-06-15 */

	if (letter(c)) {
		for (; letnum(c); c = *cp++)
			;
		c = *--cp;
		*cp = 0;
#if 0
		val = global(tokp);
		*cp = c;
#else
		val = global((char *)tokp);
		*cp = (char)c;
#endif /* Xn 1992-07-20 */
		tok = VAR;
	} else
	if (digit(c)) {
		for (; letnum(c) || c == '#'; c = *cp++)
			;
		c = *--cp;
		*cp = 0;
		val = tempvar();
#if 0
		setstr(val, tokp);
		val->flag |= RDONLY;
		*cp = c;
#else
		setstr(val, (char *)tokp);
		val->flag |= RDONLY;
		*cp = (char)c;
#endif /* Xn 1992-07-20 */
		tok = LIT;
	} else {
		c2 = *cp++;
#if 0 && XN
		(void) fprintf(stderr, "expr.c/token - c: ");
		if (isprint(c))
		{
			(void) fprintf(stderr, "'%c'; c2: ", c);
			if (isprint(c2))
			{
				(void) fprintf(stderr, "'%c'\n", c2);
			}
			else
			{
				(void) fprintf(stderr, "%02X\n", c2);
			}
		}
		else
		{
			(void) fprintf(stderr, "%02X\n", c);
		}
		(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
		if (c == '=' && c2 == '=')
			c = LEQ;
		ef (c == '!' && c2 == '=')
			c = LNE;
		ef (c == '<' && c2 == '=')
				c = LLE;
		ef (c == '>' && c2 == '=')
				c = LGE;
		else
			cp--;
		tok = c;
	}
	tokp = cp;
#if 0 && XN
	(void) fprintf(stderr, "expr.c/token - returning - ");
	if (c != '\0')
	{
		(void) fprintf(stderr, "tokp: \"%s\"; ", tokp);
	}
	(void) fprintf(stderr, "tok: %d\n", tok);
	(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
}

static struct tbl *
#if __STDC__
tempvar(void)
#else
tempvar()
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vp;

	vp = (struct tbl*) alloc(sizeof(struct tbl), ATEMP);
	lastarea = ATEMP;
	vp->flag = ISSET|INTEGER;
	vp->type = 0;
	vp->name[0] = '\0';
	return vp;
}

/* cast (string) variable to temporary integer variable */
static struct tbl *
#if __STDC__
intvar(register struct tbl *vp)
#else
intvar(vp)
	register struct tbl *vp;
#endif /* Xn 1992-07-17 */
{
	register struct tbl *vq;

	vq = tempvar();
	vq->type = 10;
	if (strint(vq, vp) == NULL) {
		if ((vp->flag&ISSET) && vp->val.s && *(vp->val.s)) {
			evalerr("bad number");
		} else {
			vq->flag |= (ISSET|INTEGER);
			vq->type = 10;
			vq->val.i = 0;
		}
	}
	return vq;
}

