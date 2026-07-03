/*
 * command tree climbing
 */

static char *RCSid = "$Id: tree.c,v 3.2 89/03/27 15:52:13 egisin Exp $";

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <setjmp.h>
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif /* Xn 1992-07-16 */
#include "sh.h"
#include "tree.h"

#define	FSTRING	(FILE*)NULL

#if 1
void	pioact ARGS((register FILE *f, register struct ioword *iop));
int	fptreef ARGS((register FILE *f, char *fmt, ...));
int	snptreef ARGS((char *s, int n, char *fmt, ...));
void	vfptreef ARGS((register FILE *f, register char *fmt, register va_list va));

#endif /* Xn 1992-07-16 */
static	int	tputc ARGS((int c, FILE *f));
static	void	tputC ARGS((int c, FILE *f));
static	void	tputS ARGS((char *wp, FILE *f));

/*
 * print a command tree
 */

void
#if __STDC__
ptree(register struct op *t, register FILE *f)
#else
ptree(t, f)
	register struct op *t;
	register FILE *f;
#endif /* Xn 1992-07-17 */
{
	register char **w;
	struct ioword **ioact;
	struct op *t1;

 Chain:
	if (t == NULL)
		return;
	switch (t->type) {
	  case TCOM:
		for (w = t->vars; *w != NULL; )
			fptreef(f, "%S ", *w++);
		for (w = t->args; *w != NULL; )
			fptreef(f, "%S ", *w++);
		break;
	  case TEXEC:
		t = t->left;
		goto Chain;
	  case TPAREN:
		fptreef(f, "(%T)", t->left);
		break;
	  case TPIPE:
		fptreef(f, "%T| ", t->left);
		t = t->right;
		goto Chain;
	  case TLIST:
		fptreef(f, "%T", t->left);
		fptreef(f, "%;");
		t = t->right;
		goto Chain;
	  case TOR:
	  case TAND:
		fptreef(f, "%T", t->left);
		fptreef(f, "%s %T", (t->type==TOR) ? "||" : "&&", t->right);
		break;
	  case TFOR:
		fptreef(f, "for %s ", t->str);
		if (t->vars != NULL) {
			fptreef(f, "in ");
			for (w = t->vars; *w; )
				fptreef(f, "%S ", *w++);
			fptreef(f, "%;");
		}
		fptreef(f, "do %T", t->left);
		fptreef(f, "%;done ");
		break;
	  case TCASE:
		fptreef(f, "case %S in%;", t->str);
		for (t1 = t->left; t1 != NULL; t1 = t1->right) {
			fptreef(f, "(");
			for (w = t1->vars; *w != NULL; w++) {
				fptreef(f, "%S", *w);
				fptreef(f, "%c", (w[1] != NULL) ? '|' : ')');
			}
			fptreef(f, " %T;;%;", t1->left);
		}
		fptreef(f, "esac ");
		break;
	  case TIF:
		fptreef(f, "if %T", t->left);
		fptreef(f, "%;");
		t = t->right;
		if (t->left != NULL) {
			fptreef(f, "then %T", t->left);
			fptreef(f, "%;");
		}
		if (t->right != NULL) {
			fptreef(f, "else %T", t->right);
			fptreef(f, "%;");
		}
		fptreef(f, "fi ");
		break;
	  case TWHILE:
	  case TUNTIL:
		fptreef(f, "%s %T",
			(t->type==TWHILE) ? "while" : "until",
			t->left);
		fptreef(f, "%;do %T", t->right);
		fptreef(f, "%;done ");
		break;
	  case TBRACE:
		fptreef(f, "{%;%T", t->left);
		fptreef(f, "%;} ");
		break;
	  case TASYNC:
		fptreef(f, "%T&", t->left);
		break;
	  case TFUNCT:
		fptreef(f, "function %s %T", t->str, t->left);
		break;
	  case TTIME:
		fptreef(f, "time %T", t->left);
		break;
	  default:
		fptreef(f, "<botch>");
		break;
	}
	if ((ioact = t->ioact) != NULL)
		while (*ioact != NULL)
			pioact(f, *ioact++);
}

void
#if __STDC__
pioact(register FILE *f, register struct ioword *iop)
#else
pioact(f, iop)
	register FILE *f;
	register struct ioword *iop;
#endif /* Xn 1992-07-17 */
{
	register int flag = iop->flag;
	if (iop->unit > 1)
		fptreef(f, "%c", '0' + iop->unit );

	switch(flag&IOTYPE) {
	case IOREAD:
		fptreef(f, "< ");
		break;
	case IOHERE:
		if (flag&IOSKIP)
			fptreef(f, "<<- ");
		else
			fptreef(f, "<< ");
		if (!(flag&IOEVAL))
			fptreef(f, "'");
		break;
	case IOCAT:
		fptreef(f, ">> ");
		break;
	case IOWRITE:
		if (flag&IOCLOB)
			fptreef(f, ">! ");
		else
			fptreef(f, "> ");
		break;
	case IODUP:			/* Needs help */
		if (iop->unit == 0)
			fptreef(f, "<&");
		else
			fptreef(f, ">&");
		break;
	}

	if ((flag&IOTYPE) == IOHERE) {
		if (flag&IOEVAL)
			fptreef(f, "%s ", iop->name);
		else
			fptreef(f, "%s' ", iop->name);
	} else {
		fptreef(f, "%S ", iop->name);
	}
}


/*
 * variants of fputc, fputs for ptreef and snptreef
 */

static	char   *snpf_s;		/* snptreef string */
static	int	snpf_n;		/* snptreef length */

static int
#if __STDC__
tputc(int c, register FILE *f)
#else
tputc(c, f)
	int c;
	register FILE *f;
#endif /* Xn 1992-07-17 */
{
	if (f != NULL)
		putc(c, f);
	else
		if (--snpf_n >= 0)
			*snpf_s++ = (char)c; /* Xn 1992-07-31 */
	return c;
}

static void
#if __STDC__
tputC(register int c, register FILE *f)
#else
tputC(c, f)
	register int c;
	register FILE *f;
#endif /* Xn 1992-07-17 */
{
	if ((c&0x60) == 0) {		/* C0|C1 */
		tputc((c&0x80) ? '$' : '^', f);
		tputc((c&0x7F|0x40), f);
	} else if ((c&0x7F) == 0x7F) {	/* DEL */
		tputc((c&0x80) ? '$' : '^', f);
		tputc('?', f);
	} else
		tputc(c, f);
}

static void
#if __STDC__
tputS(register char *wp, register FILE *f)
#else
tputS(wp, f)
	register char *wp;
	register FILE *f;
#endif /* Xn 1992-07-17 */
{
	register int c, quoted=0;

#if 0
	while (1)
#else
	for (;;)
#endif /* Xn 1992-07-31 */
		switch ((c = *wp++)) {
		  case EOS:
			return;
		  case CHAR:
			tputC(*wp++, f);
			break;
		  case QCHAR:
			if (!quoted)
				tputc('\\', f);
			tputC(*wp++, f);
			break;
		  case OQUOTE:
		  	quoted = 1;
			tputc('"', f);
			break;
		  case CQUOTE:
			quoted = 0;
			tputc('"', f);
			break;
		  case OSUBST:
			tputc('$', f);
			tputc('{', f);
			while ((c = *wp++) != 0)
				tputc(c, f);
			if (*wp != CSUBST)
				tputC(*wp++, f);
			break;
		  case CSUBST:
			tputc('}', f);
			break;
		  case COMSUB:
			tputc('$', f);
			tputc('(', f);
			while (*wp != 0)
				tputC(*wp++, f);
			tputc(')', f);
			break;
		}
}

/* TODO: use varargs properly */

#if __STDC__
int
fptreef(register FILE *f, char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
#else
/* VARARGS */ int
fptreef(f, va_alist) va_dcl
	register FILE *f;
{
	va_list va;
	char *fmt;

	va_start(va);
	fmt = va_arg(va, char *);
#endif /* Xn 1992-07-16 */
	vfptreef(f, fmt, va);
	va_end(va);
	return 0;
}

#if __STDC__
int
snptreef(char *s, int n, char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	snpf_s = s;
	snpf_n = n;
#else
/* VARARGS */ int
snptreef(s, n, va_alist) va_dcl
	char *s;
	int n;
{
	va_list va;
	char *fmt;

	snpf_s = s;
	snpf_n = n;
	va_start(va);
	fmt = va_arg(va, char *);
#endif /* Xn 1992-07-16 */
	vfptreef(FSTRING, fmt, va);
	tputc('\0', FSTRING);
	va_end(va);
	return 0;
}

void
#if __STDC__
vfptreef(register FILE *f, register char *fmt, register va_list va)
#else
vfptreef(f, fmt, va)
	register FILE *f;
	register char *fmt;
	register va_list va;
#endif /* Xn 1992-07-17 */
{
	register int c;

	while ((c = *fmt++) != 0) /* Xn 1992-07-31 */
	    if (c == '%') {
		register long n;
		register char *p;
		int neg;

		switch ((c = *fmt++)) {
		  case 'c':
			tputc(va_arg(va, int), f);
			break;
		  case 's':
			p = va_arg(va, char *);
			while (*p)
				tputc(*p++, f);
			break;
		  case 'S':	/* word */
			p = va_arg(va, char *);
			tputS(p, f);
			break;
		  case 'd': case 'u': /* decimal */
			n = (c == 'd') ? va_arg(va, int) : va_arg(va, unsigned int);
			neg = c=='d' && n<0;
			p = ulton((neg) ? -n : n, 10);
			if (neg)
				*--p = '-';
			while (*p)
				tputc(*p++, f);
			break;
		  case 'T':	/* format tree */
			ptree(va_arg(va, struct op *), f);
			break;
		  case ';':	/* newline or ; */
			p = (f == FSTRING) ? "; " : "\n";
			while (*p)
				tputc(*p++, f);
			break;
		  default:
			tputc(c, f);
			break;
		}
	    } else
		tputc(c, f);
}

/*
 * copy tree (for function definition)
 */

#if 0
static	struct ioword **iocopy();
#else
static struct ioword **iocopy ARGS((register struct ioword **, Area *));
#endif /* Xn 1992-07-17 */

struct op *
#if __STDC__
tcopy(register struct op *t, Area *ap)
#else
tcopy(t, ap)
	register struct op *t;
	Area *ap;
#endif /* Xn 1992-07-17 */
{
	register struct op *r;
	register char **tw, **rw;

	if (t == NULL)
		return NULL;

	r = (struct op *) alloc(sizeof(struct op), ap);

	r->type = t->type;

	/* this will copy function and for identifiers quite accidently */
	r->str = (t->str == NULL) ? NULL : wdcopy(t->str, ap);

	if (t->vars == NULL)
		r->vars = NULL;
	else {
		for (tw = t->vars; *tw++ != NULL; )
			;
		rw = r->vars = (char **)
			alloc((int)(tw - t->vars) * sizeof(*tw), ap);
		for (tw = t->vars; *tw != NULL; )
			*rw++ = wdcopy(*tw++, ap);
		*rw = NULL;
	}

	if (t->args == NULL)
		r->args = NULL;
	else {
		for (tw = t->args; *tw++ != NULL; )
			;
		rw = r->args = (char **)
			alloc((int)(tw - t->args) * sizeof(*tw), ap);
		for (tw = t->args; *tw != NULL; )
			*rw++ = wdcopy(*tw++, ap);
		*rw = NULL;
	}

	r->ioact = (t->ioact == NULL) ? NULL : iocopy(t->ioact, ap);

	r->left = tcopy(t->left, ap);
	r->right = tcopy(t->right, ap);

	return r;
}

char *
#if __STDC__
wdcopy(char *wp, Area *ap)
#else
wdcopy(wp, ap)
	char *wp;
	Area *ap;
#endif /* Xn 1992-07-17 */
{
	size_t len = wdscan(wp, EOS) - wp;
	return memcpy(alloc(len, ap), wp, len);
}

/* return the position of prefix c in wp plus 1 */
char *
#if __STDC__
wdscan(register char *wp, register int c)
#else
wdscan(wp, c)
	register char *wp;
	register int c;
#endif /* Xn 1992-07-17 */
{
	register int nest = 0;

#if 0
	while (1)
#else
	for (;;)
#endif /* Xn 192-07-31 */
		switch (*wp++) {
		  case EOS:
			return wp;
		  case CHAR:
		  case QCHAR:
			wp++;
			break;
		  case OQUOTE:
		  case CQUOTE:
			break;
		  case OSUBST:
			nest++;
			while (*wp++ != 0)
				;
			if (*wp != CSUBST)
				wp++;
			break;
		  case CSUBST:
			if (c == CSUBST && nest == 0)
				return wp;
			nest--;
			break;
		  case COMSUB:
			while (*wp++ != 0)
				;
			break;
		}
}

static struct ioword **
#if __STDC__
iocopy(register struct ioword **iow, Area *ap)
#else
iocopy(iow, ap)
	register struct ioword **iow;
	Area *ap;
#endif /* Xn 1992-07-17 */
{
	register struct ioword **ior;
	register int i;

	for (ior = iow; *ior++ != NULL; )
		;
	ior = (struct ioword **) alloc((int)(ior - iow) * sizeof(*ior), ap);

	for (i = 0; iow[i] != NULL; i++) {
		register struct ioword *p, *q;

		p = iow[i];
		q = (struct ioword *) alloc(sizeof(*p), ap);
		ior[i] = q;
		*q = *p;
		if (p->name != NULL)
			q->name = wdcopy(p->name, ap);
	}
	ior[i] = NULL;

	return ior;
}

/*
 * free tree (for function definition)
 */

#if 0
static	void iofree();
#else
static void iofree ARGS((struct ioword **, Area *));
#endif /* Xn 1992-07-17 */

void
#if __STDC__
tfree(register struct op *t, Area *ap)
#else
tfree(t, ap)
	register struct op *t;
	Area *ap;
#endif /* Xn 1992-07-17 */
{
	register char **w;

	if (t == NULL)
		return;

	if (t->str != NULL)
		afree((Void*)t->str, ap);

	if (t->vars != NULL) {
		for (w = t->vars; *w != NULL; w++)
			afree((Void*)*w, ap);
		afree((Void*)t->vars, ap);
	}

	if (t->args != NULL) {
		for (w = t->args; *w != NULL; w++)
			afree((Void*)*w, ap);
		afree((Void*)t->args, ap);
	}

	if (t->ioact != NULL)
		iofree(t->ioact, ap);

	tfree(t->left, ap);
	tfree(t->right, ap);

	afree((Void*)t, ap);
}

static void
#if __STDC__
iofree(struct ioword **iow, Area *ap)
#else
iofree(iow, ap)
	struct ioword **iow;
	Area *ap;
#endif /* Xn 1992-07-17 */
{
	register struct ioword **iop;
	register struct ioword *p;

	for (iop = iow; (p = *iop++) != NULL; ) {
		if (p->name != NULL)
			afree((Void*)p->name, ap);
		afree((Void*)p, ap);
	}
}

