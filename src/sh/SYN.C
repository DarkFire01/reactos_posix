/*
 * shell parser (C version)
 */

static char *RCSid = "$Id: syn.c,v 3.3 89/03/27 15:51:51 egisin Exp $";

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include "sh.h"
#include "lex.h"
#include "tree.h"
#include "table.h"
#include "expand.h"

#if 0
static	void	zzerr();
static	struct	op *block(), *newtp();
static	struct	op *pipeline(), *andor(), *command();
static	struct	op *nested(), *c_list();
static	struct	op *dogroup(), *thenpart(), *casepart(), *caselist();
static	struct	op *elsepart();
static	char  **wordlist();
static	void	musthave();
static	struct ioword *synio(), *io();
#else
extern	void	unwind ARGS((void)); /* in main.c */

static	void	zzerr ARGS((void));
static	struct	op *block ARGS((int, struct op *, struct op *, char **)), *newtp ARGS((int));
static	struct	op *pipeline ARGS((int)), *andor ARGS((void)), *command ARGS((int));
static	struct	op *nested ARGS((int, int)), *c_list ARGS((void));
static	struct	op *dogroup ARGS((int)), *thenpart ARGS((void)), *casepart ARGS((void)), *caselist ARGS((void));
static	struct	op *elsepart ARGS((void));
static	char  **wordlist ARGS((void));
static	void	musthave ARGS((int, int));
static	struct ioword *synio ARGS((int));
#endif /* Xn 1992-07-31 */

static	struct	op	*outtree; /* yyparse output */

static	int	reject;		/* token(cf) gets symbol again */
static	int	symbol;		/* yylex value */

#define	REJECT	(reject = 1)
#define	ACCEPT	(reject = 0)
#define	token(cf) \
	((reject) ? (ACCEPT, symbol) : (symbol = yylex(cf)))
#define	tpeek(cf) \
	((reject) ? (symbol) : (REJECT, symbol = yylex(cf)))

int
#if __STDC__
yyparse(void)
#else
yyparse()
#endif /* Xn 1992-07-17 */
{
#if 1
	int i;

#endif /* Xn 1992-08-04 */
	ACCEPT;
	yynerrs = 0;
#if 0 && XN
	(void) fprintf(stderr, "before yyparse/tpeek\n");
	(void) fflush(stderr);
#endif
#if 1
	if (reject)
		i = symbol;
	else {
		REJECT;
		i = symbol = yylex(KEYWORD|ALIAS);
	}
	if (i == 0) { /* EOF */
#else
	if ((tpeek(KEYWORD|ALIAS)) == 0) { /* EOF */
#endif /* Xn 1992-08-04 */
#if 0 && XN
		(void) fprintf(stderr, "after yyparse/tpeek - i: %d\nbefore yyparse/newtp\n", i);
		(void) fflush(stderr);
#endif
		outtree = newtp(TEOF);
#if 0 && XN
		(void) fprintf(stderr, "after yyparse/newtp\n");
		(void) fflush(stderr);
#endif
		return 0;
	}
#if 0 && XN
	(void) fprintf(stderr, "after yyparse/tpeek - i: %d\nbefore yyparse/c_list\n", i);
	(void) fflush(stderr);
#endif
	outtree = c_list();
#if 0 && XN
	(void) fprintf(stderr, "after yyparse/c_list\n");
	(void) fflush(stderr);
#endif
	musthave('\n', 0);
	return (yynerrs != 0);
}

static struct op *
#if __STDC__
pipeline(int cf)
#else
pipeline(cf)
	int cf;
#endif /* Xn 1992-07-17 */
{
	register struct op *t, *p, *tl = NULL;
	register int c;

	t = command(cf);
	if (t != NULL) {
		while ((c = token(0)) == '|') {
			if ((p = command(CONTIN)) == NULL)
				SYNTAXERR;
			if (tl == NULL)
				t = tl = block(TPIPE, t, p, NOWORDS);
			else
				tl = tl->right = block(TPIPE, tl->right, p, NOWORDS);
			/*t = block(TPIPE, t, p, NOWORDS);*/
		}
		REJECT;
	}
	return (t);
}

static struct op *
#if __STDC__
andor(void)
#else
andor()
#endif /* Xn 1992-07-17 */
{
	register struct op *t, *p;
	register int c;

	t = pipeline(0);
	if (t != NULL) {
		while ((c = token(0)) == LOGAND || c == LOGOR) {
			if ((p = pipeline(CONTIN)) == NULL)
				SYNTAXERR;
			t = block(c == LOGAND? TAND: TOR, t, p, NOWORDS);
		}
		REJECT;
	}
	return (t);
}

static struct op *
#if __STDC__
c_list(void)
#else
c_list()
#endif /* Xn 1992-07-17 */
{
	register struct op *t, *p, *tl = NULL;
	register int c;

	t = andor();
	if (t != NULL) {
		while ((c = token(0)) == ';' || c == '&' ||
		       (multiline || source->type == SSTRING
		        || source->type == SALIAS) && c == '\n') {
			if (c == '&') {
				if (tl)
					tl->right = block(TASYNC, tl->right, NOBLOCK, NOWORDS);
				else
					t = block(TASYNC, t, NOBLOCK, NOWORDS);
			}
			if ((p = andor()) == NULL)
				return (t);
			if (tl == NULL)
				t = tl = block(TLIST, t, p, NOWORDS);
			else
				tl = tl->right = block(TLIST, tl->right, p, NOWORDS);
		}
		REJECT;
	}
	return (t);
}

static struct ioword *
#if __STDC__
synio(int cf)
#else
synio(cf)
	int cf;
#endif /* Xn 1992-07-17 */
{
	register struct ioword *iop;

	if (tpeek(cf) != REDIR)
		return NULL;
	ACCEPT;
	iop = yylval.iop;
	musthave(LWORD, 0);
	iop->name = yylval.cp;
	if ((iop->flag&IOTYPE) == IOHERE) {
		if (*ident != 0) /* unquoted */
			iop->flag |= IOEVAL;
		if (herep >= &heres[HERES])
			errorf("too many <<'s\n");
		*herep++ = iop;
	}
	return iop;
}

static void
#if __STDC__
musthave(int c, int cf)
#else
musthave(c, cf)
	int c, cf;
#endif /* Xn 1992-07-17 */
{
	if ((token(cf)) != c)
		SYNTAXERR;
}

static struct op *
#if __STDC__
nested(int type, int mark)
#else
nested(type, mark)
	int type, mark;
#endif /* Xn 1992-07-17 */
{
	register struct op *t;

	multiline++;
	t = c_list();
	musthave(mark, KEYWORD);
	multiline--;
	return (block(type, t, NOBLOCK, NOWORDS));
}

static struct op *
#if __STDC__
command(int cf)
#else
command(cf)
	int cf;
#endif /* Xn 1992-07-17 */
{
	register struct op *t;
	register int c, iopn = 0;
	struct ioword *iop, **iops;
	XPtrV args, vars;

	iops = (struct ioword **) alloc(sizeofN(struct ioword *, NUFILE+1), ATEMP);
	XPinit(args, 16);
	XPinit(vars, 16);

	if (multiline)
		cf = CONTIN;
	cf |= KEYWORD|ALIAS;

#if 0 && XN
	(void) fprintf(stderr, "command - before synio loop #1\n");
	(void) fflush(stderr);
#endif
	while ((iop = synio(cf)) != NULL) {
		if (iopn >= NUFILE)
			yyerror("too many redirections");
		iops[iopn++] = iop;
		cf &=~ CONTIN;
	}
#if 0 && XN
	(void) fprintf(stderr, "command - after synio loop #1\n");
	(void) fflush(stderr);
#endif

	switch (c = token(cf)) {
	  case 0:
		yyerror("unexpected EOF");
		return NULL;

	  default:
		REJECT;
		if (iopn == 0)
			return NULL; /* empty line */
		t = newtp(TCOM);
		break;

	  case LWORD:
	  case MDPAREN:
		REJECT;
		t = newtp(TCOM);
		if (c == MDPAREN) {
			ACCEPT;
			XPput(args,"let");
			musthave(LWORD,LETEXPR);
#if 0 && XN
			(void) fprintf(stderr, "syn.c/command - yylval.cp: \"%s\"\n", yylval.cp);
			(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
			XPput(args,yylval.cp);
		}
#if 0
		while (1)
#else
		for (;;)
#endif /* Xn 1992-07-31 */
			switch (tpeek(0)) {
			  case REDIR:
				if (iopn >= NUFILE)
					yyerror("too many redirections");
				iops[iopn++] = synio(0);
#if 0 && XN
				(void) fprintf(stderr, "command - REDIR\n");
				(void) fflush(stderr);
#endif
				break;

			  case LWORD:
				ACCEPT;
				if ((XPsize(args) == 0 || flag[FKEYWORD])
				    && strchr(ident+1, '='))
					{XPput(vars, yylval.cp);}
				else
					{XPput(args, yylval.cp);}
				break;

			  case MPAREN:
				ACCEPT;
				if (XPsize(args) != 1)
					SYNTAXERR;
				if (*ident == 0)
					yyerror("invalid function name\n");
				t = newtp(TFUNCT);
				t->str = strsave(ident, ATEMP);
				musthave('{', CONTIN|KEYWORD);
				t->left = nested(TBRACE, '}');
				return t;

			  default:
				goto Leave;
			}
	  Leave:
		break;

	  case '(':
		t = nested(TPAREN, ')');
		break;

	  case '{':
		t = nested(TBRACE, '}');
		break;

	  case FOR:
		t = newtp(TFOR);
		musthave(LWORD, 0);
		t->str = strsave(ident, ATEMP);
		multiline++;
		t->vars = wordlist();
		t->left = dogroup(0);
		multiline--;
		break;

	  case WHILE:
	  case UNTIL:
		multiline++;
		t = newtp((c == WHILE) ? TWHILE: TUNTIL);
		t->left = c_list();
		t->right = dogroup(1);
		multiline--;
		break;

	  case CASE:
		t = newtp(TCASE);
		musthave(LWORD, 0);
		t->str = yylval.cp;
		multiline++;
		musthave(IN, KEYWORD|CONTIN);
		t->left = caselist();
		musthave(ESAC, KEYWORD);
		multiline--;
		break;

	  case IF:
		multiline++;
		t = newtp(TIF);
		t->left = c_list();
		t->right = thenpart();
		musthave(FI, KEYWORD);
		multiline--;
		break;

	  case TIME:
		t = pipeline(CONTIN);
		t = block(TTIME, t, NOBLOCK, NOWORDS);
		break;

	  case FUNCTION:
		t = newtp(TFUNCT);
		musthave(LWORD, 0);
		t->str = strsave(ident, ATEMP);
		musthave('{', CONTIN|KEYWORD);
		t->left = nested(TBRACE, '}');
		break;

#if 0
	  case MDPAREN:
		t = newtp(TCOM);
		XPput(args, "let");
		musthave(LWORD, LETEXPR);
		XPput(args, yylval.cp);
		while (tpeek(0) == REDIR) {
			if (iopn >= NUFILE)
				yyerror("too many redirections");
			iops[iopn++] = synio(0);
		}
		break;
#endif
	}

#if 0 && XN
	(void) fprintf(stderr, "command - before synio loop #2\n");
	(void) fflush(stderr);
#endif
	while ((iop = synio(0)) != NULL) {
		if (iopn >= NUFILE)
			yyerror("too many redirections");
		iops[iopn++] = iop;
	}
#if 0 && XN
	(void) fprintf(stderr, "command - after synio loop #2\n");
	(void) fflush(stderr);
#endif

	if (iopn == 0) {
		afree((Void*) iops, ATEMP);
		t->ioact = NULL;
	} else {
		iops[iopn++] = NULL;
		aresize((Void*) iops, sizeofN(struct ioword *, iopn), ATEMP);
		t->ioact = iops;
	}

	if (t->type == TCOM) {
		XPput(args, NULL);
		t->args = (char **) XPclose(args);
		XPput(vars, NULL);
		t->vars = (char **) XPclose(vars);
	} else {
		XPfree(args);
		XPfree(vars);
	}

	return t;
}

static struct op *
#if __STDC__
dogroup(int onlydone)
#else
dogroup(onlydone)
	int onlydone;
#endif /* Xn 1992-07-17 */
{
	register int c;
	register struct op *list;

	c = token(CONTIN|KEYWORD);
	if (c == DONE && onlydone)
		return NULL;
	if (c != DO)
		SYNTAXERR;
	list = c_list();
	musthave(DONE, KEYWORD);
	return list;
}

static struct op *
#if __STDC__
thenpart(void)
#else
thenpart()
#endif /* Xn 1992-07-17 */
{
	register int c;
	register struct op *t;

	if ((c = token(0)) != THEN) {
		REJECT;
		return NULL;
	}
	t = newtp(0);
	t->left = c_list();
	if (t->left == NULL)
		SYNTAXERR;
	t->right = elsepart();
	return (t);
}

static struct op *
#if __STDC__
elsepart(void)
#else
elsepart()
#endif /* Xn 1992-07-17 */
{
	register int c;
	register struct op *t;

	switch (c = token(0)) {
	  case ELSE:
		if ((t = c_list()) == NULL)
			SYNTAXERR;
		return (t);

	  case ELIF:
		t = newtp(TELIF);
		t->left = c_list();
		t->right = thenpart();
		return (t);

	  default:
		REJECT;
		return NULL;
	}
}

static struct op *
#if __STDC__
caselist(void)
#else
caselist()
#endif /* Xn 1992-07-17 */
{
	register struct op *t, *tl;

	t = tl = NULL;
	while ((tpeek(CONTIN|KEYWORD)) != ESAC) {
		struct op *tc = casepart();
		if (tl == NULL)
			t = tl = tc, tl->right = NULL;
		else
			tl->right = tc, tl = tc;
	}
	return (t);
}

static struct op *
#if __STDC__
casepart(void)
#else
casepart()
#endif /* Xn 1992-07-17 */
{
	register struct op *t;
	register int c, cf;
	XPtrV ptns;

	XPinit(ptns, 16);
	t = newtp(TPAT);
	cf = CONTIN|KEYWORD;
	c = token(cf);
	if (c != '(')
		REJECT;
	else
		cf = 0;
	do {
		musthave(LWORD, cf);
		XPput(ptns, yylval.cp);
		cf = 0;
	} while ((c = token(0)) == '|');
	REJECT;
	XPput(ptns, NULL);
	t->vars = (char **) XPclose(ptns);
	musthave(')', 0);

	t->left = c_list();
	if ((tpeek(CONTIN|KEYWORD)) != ESAC)
		musthave(BREAK, CONTIN|KEYWORD);
	return (t);
}

static char **
#if __STDC__
wordlist(void)
#else
wordlist()
#endif /* Xn 1992-07-17 */
{
	register int c;
	XPtrV args;

	XPinit(args, 16);
	if ((c = token(CONTIN|KEYWORD)) != IN) {
		REJECT;
		return NULL;
	}
	while ((c = token(0)) == LWORD)
		XPput(args, yylval.cp);
	if (c != '\n' && c != ';')
		SYNTAXERR;
	if (XPsize(args) == 0) {
		XPfree(args);
		return NULL;
	} else {
		XPput(args, NULL);
		return (char **) XPclose(args);
	}
}

/*
 * supporting functions
 */

static struct op *
#if __STDC__
block(int type, struct op *t1, struct op *t2, char **wp)
#else
block(type, t1, t2, wp)
	struct op *t1, *t2;
	char **wp;
#endif /* Xn 1992-07-17 */
{
	register struct op *t;

	t = newtp(type);
	t->left = t1;
	t->right = t2;
	t->vars = wp;
	return (t);
}

Const	struct res {
	char	*name;
	int	val;
} restab[] = {
	"for",		FOR,
	"case",		CASE,
	"esac",		ESAC,
	"while",	WHILE,
	"do",		DO,
	"done",		DONE,
	"if",		IF,
	"in",		IN,
	"then",		THEN,
	"else",		ELSE,
	"elif",		ELIF,
	"until",	UNTIL,
	"fi",		FI,
	"function",	FUNCTION,
	"time",		TIME,
	"{",		'{',
	"}",		'}',
	0
};

void
#if __STDC__
keywords(void)
#else
keywords()
#endif /* Xn 1992-07-17 */
{
	register struct res Const *rp;
	register struct tbl *p;

	for (rp = restab; rp->name; rp++) {
		p = tenter(&lexicals, rp->name, hash(rp->name));
		p->flag |= DEFINED|ISSET;
		p->type = CKEYWD;
		p->val.i = rp->val;
	}
}

static struct op *
#if __STDC__
newtp(int type)
#else
newtp(type)
	int type;
#endif /* Xn 1992-07-17 */
{
	register struct op *t;

	t = (struct op *) alloc(sizeof(*t), ATEMP);
	t->type = type;
	t->args = t->vars = NULL;
	t->ioact = NULL;
	t->left = t->right = NULL;
	t->str = NULL;
	return (t);
}

static void
#if __STDC__
zzerr(void)
#else
zzerr()
#endif /* Xn 1992-07-17 */
{
	yyerror("syntax error");
}

struct op *
#if __STDC__
compile(Source *s)
#else
compile(s)
	Source *s;
#endif /* Xn 1992-07-17 */
{
	yynerrs = 0;
	multiline = 0;
	herep = heres;
	source = s;
#if 0 && XN
	(void) fprintf(stderr, "before compile/yyparse\n");
	(void) fflush(stderr);
#endif
	if (yyparse())
		unwind();
#if 0 && XN
	(void) fprintf(stderr, "after compile/yyparse - outtree: %p\n", outtree);
	(void) fflush(stderr);
#endif
	if (s->type == STTY || s->type == SFILE || s->type == SHIST)
		s->str = null;	/* line is not preserved */
	return outtree;
}

