/*
 * lexical analysis and source input
 */

#ifndef lint
static char *RCSid = "$Id: lex.c,v 3.6 89/03/27 15:51:20 egisin Exp $";
static char *sccs_id = "@(#)lex.c	1.3 91/11/09 15:35:19 (sjg)";
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <unistd.h>
#include <assert.h>
#if 0 && XN
#include <termios.h>
#endif /* Xn 1993-05-26 */
#include "sh.h"
#include "lex.h"
#include "tree.h"
#include "table.h"
#include "expand.h"

	int	ttyfd = -1;		/* tty fd for edit and jobs */
	char   *history[HISTORY];	/* saved commands */
	char  **histptr = history - 1;	/* last history item */
	int	histpush;		/* number of pushed fc commands */

/* we set s->str to NULLSTR instead of "", so that ungetsc() works */
static	char	nullstr [] = {0, 0};
#define	NULLSTR	(nullstr + 1)

#if 1
extern	void	mprint ARGS((void)); /* in mail.c */

void	gethere	ARGS((void));
void	pprompt	ARGS((register char *));

#endif /* Xn 1992-07-29 */
static	int	expanding_alias;
static	int	alias;
static	int	getsc_ ARGS((void));

/* optimized getsc_() */
#if WIN_NT
int getsc(void)
{
	int sc;

	if (*source->str == '\r' && source->str[1] == '\n')
		++source->str;
	if (*source->str != 0)
		sc = *source->str++;
	else
		sc = getsc_();
	return sc;
}

void ungetsc(void)
{
	--source->str;
	if (source->str[1] == '\n' && *source->str == '\r')
		--source->str;
}
#else
#define	getsc()	((*source->str != 0) ? *source->str++ : getsc_())
#define	ungetsc() (source->str--)
#endif /* Xn 1992-09-03 */

/*
 * Lexical analyzer
 *
 * tokens are not regular expressions, they are LL(1).
 * for example, "${var:-${PWD}}", and "$(size $(whence ksh))".
 * hence the state stack.
 */

int
#if __STDC__
yylex(int cf)
#else
yylex(cf)
	int cf;
#endif /* Xn 1992-07-17 */
{
	register int c, state;
	char states [64], *statep = states;
	XString ws;		/* expandable output word */
	register char *wp;	/* output word pointer */
	register char *sp, *dp;
	int istate;
	int c2;
	static int rec_alias_cnt = 0;
	static struct tbl *rec_alias_table[20];


	if (expanding_alias) {
		expanding_alias = 0;
		while (rec_alias_cnt-- > 0)
			rec_alias_table[rec_alias_cnt]->flag &= ~EXPALIAS;
		rec_alias_cnt = 0;
	}
  Again:
#if 0 && XN
	(void) fprintf(stderr, "after Again; before Xinit\n");
	(void) fflush(stderr);
#endif
	Xinit(ws, wp, 64);

	if (cf&ONEWORD)
		istate = SWORD;
	else if (cf&LETEXPR)
#if 1
	{
#endif /* Xn 1993-06-15 */
		istate = SDPAREN;
#if 1
		*wp++ = OQUOTE;
	}
#endif /* Xn 1993-06-15 */
	else {			/* normal lexing */
		istate = SBASE;
		while ((c = getsc()) == ' ' || c == '\t')
			;
		if (c == '#')
			while ((c = getsc()) != 0 && c != '\n')
				;
		ungetsc();
	}
	if (alias) {			/* trailing ' ' in alias definition */
		alias = 0;
		cf |= ALIAS;
	}

	/* collect non-special or quoted characters to form word */
#if 0 && XN
	(void) fprintf(stderr, "before yylex/for\n");
	(void) fflush(stderr);
#endif
	for (*statep = (char)(state = istate); /* Xn 1992-07-29 */
	     !((c = getsc()) == 0 || state == SBASE && ctype(c, C_LEX1)); ) {
		Xcheck(ws, wp);
		switch (state) {
		  case SBASE:
		  Sbase:
			switch (c) {
			  case '\\':
				c = getsc();
#if 0 && XN
				(void) fprintf(stderr, "SBASE/'\\' - c: H'%02X\n", c);
				(void) fflush(stderr);
#endif
				if (c != '\n')
					*wp++ = QCHAR, *wp++ = (char)c; /* Xn 1992-07-29 */
				else
					if (wp == Xstring(ws, wp))
						goto Again;
				break;
			  case '\'':
				*++statep = (char)(state = SSQUOTE); /* Xn 1992-07-29 */
				*wp++ = OQUOTE;
				break;
			  case '"':
				*++statep = (char)(state = SDQUOTE); /* Xn 1992-07-29 */
				*wp++ = OQUOTE;
				break;
			  default:
				goto Subst;
			}
			break;

		  Subst:
			switch (c) {
			  case '\\':
				c = getsc();
				switch (c) {
				  case '\n':
					break;
				  case '"': case '\\':
				  case '$': case '`':
					*wp++ = QCHAR, *wp++ = (char)c; /* Xn 1992-07-29 */
					break;
				  default:
					Xcheck(ws, wp);
					*wp++ = CHAR, *wp++ = '\\';
					*wp++ = CHAR, *wp++ = (char)c; /* Xn 1992-07-29 */
					break;
				}
				break;
			  case '$':
				c = getsc();
				if (c == '(') {
					*++statep = (char)(state = SPAREN); /* Xn 1992-07-29 */
					*wp++ = COMSUB;
				} else
				if (c == '{') {
					*++statep = (char)(state = SBRACE); /* Xn 1992-07-29 */
					*wp++ = OSUBST;
					c = getsc();
					do {
						Xcheck(ws, wp);
						*wp++ = (char)c; /* Xn 1992-07-29 */
						c = getsc();
					} while (ctype(c, C_ALPHA|C_DIGIT));
					*wp++ = 0;
					/* todo: more compile-time checking */
					if (c == '}')
						ungetsc();
					else if (c == '#' || c == '%') {
						/* Korn pattern trimming */
						if (getsc() == c)
							c |= 0x80;
						else
							ungetsc();
						*wp++ = (char)c; /* Xn 1992-07-29 */
					} else if (c == ':')
						*wp++ = (char)(0x80|getsc()); /* Xn 1992-07-29 */
					else
						*wp++ = (char)c; /* Xn 1992-07-29 */
				} else if (ctype(c, C_ALPHA)) {
					*wp++ = OSUBST;
					do {
						Xcheck(ws, wp);
						*wp++ = (char)c; /* Xn 1992-07-29 */
						c = getsc();
					} while (ctype(c, C_ALPHA|C_DIGIT));
					*wp++ = 0;
					*wp++ = CSUBST;
					ungetsc();
				} else if (ctype(c, C_DIGIT|C_VAR1)) {
					Xcheck(ws, wp);
					*wp++ = OSUBST;
					*wp++ = (char)c; /* Xn 1992-07-29 */
					*wp++ = 0;
					*wp++ = CSUBST;
				} else {
					*wp++ = CHAR, *wp++ = '$';
					ungetsc();
				}
				break;
			  case '`':
				*++statep = (char)(state = SBQUOTE); /* Xn 1992-07-29 */
				*wp++ = COMSUB;
				break;
			  default:
				*wp++ = CHAR, *wp++ = (char)c; /* Xn 1992-07-29 */
			}
			break;

		  case SSQUOTE:
			if (c == '\'') {
				state = *--statep;
				*wp++ = CQUOTE;
			} else
				*wp++ = QCHAR, *wp++ = (char)c; /* Xn 1992-07-29 */
			break;

		  case SDQUOTE:
			if (c == '"') {
				state = *--statep;
				*wp++ = CQUOTE;
			} else
				goto Subst;
			break;

		  case SPAREN:
			if (c == '(')
				*++statep = (char)state; /* Xn 1992-07-29 */
			else if (c == ')')
				state = *--statep;
			if (state == SPAREN)
				*wp++ = (char)c; /* Xn 1992-07-29 */
			else
				*wp++ = 0; /* end of COMSUB */
			break;

		  case SBRACE:
			if (c == '}') {
				state = *--statep;
				*wp++ = CSUBST;
			} else
				goto Sbase;
			break;

		  case SBQUOTE:
			if (c == '`') {
				*wp++ = 0;
				state = *--statep;
			} else if (c == '\\') {
				switch (c = getsc()) {
				  case '\n':
					break;
				  case '\\':
				  case '$': case '`':
					*wp++ = (char)c; /* Xn 1992-07-29 */
					break;
				  default:
					*wp++ = '\\';
					*wp++ = (char)c; /* Xn 1992-07-29 */
					break;
				}
			} else
				*wp++ = (char)c; /* Xn 1992-07-29 */
			break;

		  case SWORD:	/* ONEWORD */
			goto Subst;

		  case SDPAREN:	/* LETEXPR */
			if (c == ')') {
				if (getsc() == ')') {
#if 1
					*wp++ = CQUOTE;
#endif /* Xn 1993-06-15 */
					c = 0;
					goto Done;
				} else
					ungetsc();
			}
			goto Subst;
		}
	}
Done:
#if 0 && XN
	(void) fprintf(stderr, "after yylex/Done\n");
	(void) fflush(stderr);
#endif
	Xcheck(ws, wp);
	if (state != istate)
		yyerror("no closing quote");

	if (c == '<' || c == '>') {
		char *cp = Xstring(ws, wp);
		if (wp > cp && cp[0] == CHAR && digit(cp[1])) {
			wp = cp; /* throw away word */
			c2/*unit*/ = cp[1] - '0';
		} else
			c2/*unit*/ = c == '>'; /* 0 for <, 1 for > */
	}

#if 0 && XN
	(void) fprintf(stderr, "wp: %p; Xstring(ws, wp): %p; state: %d; SBASE: %d\n",
		wp, Xstring(ws, wp), state, SBASE);
	(void) fflush(stderr);
#endif
	if (wp == Xstring(ws, wp) && state == SBASE) {
		Xfree(ws, sp);	/* free word */
		/* no word, process LEX1 character */
#if 0 && XN
		(void) fprintf(stderr, "c: H'%02X\n", c);
		(void) fflush(stderr);
#endif
		switch (c) {
		  default:
			return c;

		  case '|':
		  case '&':
		  case ';':
#if 0 && XN
			c2 = c;
#endif
			if (getsc() == c)
				c = (c == ';') ? BREAK :
				    (c == '|') ? LOGOR :
				    (c == '&') ? LOGAND :
				    YYERRCODE;
			else
				ungetsc();
#if 0 && XN
			(void) fprintf(stderr, "exiting yylex/\"|&;\" ('%c')\n", c2);
			(void) fflush(stderr);
#endif
			return c;

		  case '>':
		  case '<': {
			register struct ioword *iop;

			iop = (struct ioword *) alloc(sizeof(*iop), ATEMP);
			iop->unit = c2/*unit*/;

			c2 = getsc();
			if (c2 == '>' || c2 == '<') {
				iop->flag = c != c2 ? IORDWR : c == '>' ? IOCAT : IOHERE;
				c2 = getsc();
			} else
				iop->flag = c == '>' ? IOWRITE : IOREAD;

			if (iop->flag == IOHERE)
				if (c2 == '-')
					iop->flag |= IOSKIP;
				else
					ungetsc();
			else
				if (c2 == '&')
					iop->flag = IODUP;
				else if (c2 == '!' && iop->flag == IOWRITE)
					iop->flag |= IOCLOB;
				else
					ungetsc();
			yylval.iop = iop;
			return REDIR;
		    }
		  case '\n':
			gethere();
			if (cf & CONTIN)
				goto Again;
			return c;

		  case '(':
			c2 = getsc();
			if (c2 == ')')
				c = MPAREN;
			else if (c2 == '(')
				c = MDPAREN;
			else
				ungetsc();
		  case ')':
#if 0 && XN
			(void) fprintf(stderr, "exiting yylex/\"()\" (%s)\n",
				(c == '(') ? "(" : (c == MPAREN) ? "MPAREN" : "MDPAREN");
			(void) fflush(stderr);
#endif
			return c;
		}
	}

	*wp++ = EOS;		/* terminate word */
	yylval.cp = Xclose(ws, wp);
	if (state == SWORD || state == SDPAREN)	/* ONEWORD? */
		return LWORD;
	ungetsc();		/* unget terminator */

	/* copy word to unprefixed string ident */
	for (sp = yylval.cp, dp = ident; dp < ident+IDENT && (c = *sp++) == CHAR; )
		*dp++ = *sp++;
	/* Make sure the ident array stays '\0' padded */
	while (dp <= ident+IDENT)
		*dp++ = '\0';
#if 0
	if (*ident == '~' || (dp = strchr(ident, '=')) != NULL && dp[1] == '~')
		"Tilde expansion";
#endif
	if (c != EOS)
		*ident = 0;	/* word is not unquoted */

	if (*ident != 0 && (cf&(KEYWORD|ALIAS))) {
		register struct tbl *p;

		p = tsearch(&lexicals, ident, hash(ident));
		if (p != NULL && (p->flag&ISSET))
			if (p->type == CKEYWD && (cf&KEYWORD)) {
				afree(yylval.cp, ATEMP);
				return (int)p->val.i; /* Xn 1992-07-29 */
			} else if (p->type == CALIAS && (cf&ALIAS) &&
				   !(p->flag&EXPALIAS)) {
				register Source *s;

				if (rec_alias_cnt == sizeof(rec_alias_table)/sizeof(rec_alias_table[0]))
					yyerror("excessive recusrsive aliasing");
				else
					rec_alias_table[rec_alias_cnt++] = p;
				p->flag |= EXPALIAS;
				/* check for recursive aliasing */
				for (s = source; s->type == SALIAS; s = s->next)
					if (s->u.tblp == p)
						return LWORD;
				afree(yylval.cp, ATEMP);

				/* push alias expansion */
				s = pushs(SALIAS);
				s->str = p->val.s;
				s->u.tblp = p;
				s->next = source;
				source = s;
				goto Again;
			} 
	}

	return LWORD;
}

#if 0
static void readhere();
#else
static void readhere ARGS((register struct ioword *));
#endif /* Xn 1992-07-17 */

void
#if __STDC__
gethere(void)
#else
gethere()
#endif /* Xn 1992-07-17 */
{
	register struct ioword **p;

	for (p = heres; p < herep; p++)
		readhere(*p);
	herep = heres;
}

/*
 * read "<<word" text into temp file
 * todo: set up E_ERR to fclose(f) on unwind
 */

static void
#if __STDC__
readhere(register struct ioword *iop)
#else
readhere(iop)
	register struct ioword *iop;
#endif /* Xn 1992-07-17 */
{
	register FILE *f;
	struct temp *h;
	register int c;
	char *eof;
	register char *cp;
	char line [LINE+1];

	eof = evalstr(iop->name, 0);

	h = maketemp(ATEMP);
	h->next = e.temps; e.temps = h;
	iop->name = h->name;
	f = fopen(h->name, "w");
	if (f == NULL)
		errorf("Cannot create temporary file\n");
	setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);

	for (;;) {
		cp = line;
		while ((c = getsc()) != '\n') {
			if (c == 0)
				errorf("here document `%s' unclosed\n", eof);
			if (cp >= line+LINE)
				break;
			*cp++ = (char)c; /* Xn 1992-07-29 */
		}
		ungetsc();
		*cp = 0;
		for (cp = line; iop->flag&IOSKIP && *cp == '\t'; cp++)
			;
		if (strcmp(eof, cp) == 0 || c == 0)
			break;
		while ((c = *cp++) != '\0')
			putc(c, f);
		while ((c = getsc()) != '\n') {
			if (c == 0)
				errorf("here document `%s' unclosed\n", eof);
			putc(c, f);
		}
		putc(c, f);
	}
	fclose(f);
}

void
#if __STDC__
yyerror(Const char *msg)
#else
yyerror(msg)
	Const char *msg;
#endif /* Xn 1992-07-17 */
{
	yynerrs++;
	while (source->type == SALIAS) /* pop aliases */
		source = source->next;
	if (source->file != NULL)
		shellf("%s[%d]: ", source->file, source->line);
	source->str = NULLSTR;	/* zap pending input */
	errorf("%s\n", msg);
}

/*
 * input for yylex with alias expansion
 */

Source *
#if __STDC__
pushs(int type)
#else
pushs(type)
	int type;
#endif /* Xn 1992-07-17 */
{
	register Source *s;

	s = (Source *) alloc(sizeof(Source), ATEMP);
	s->type = type;
	s->str = NULLSTR;
	s->line = 0;
	s->file = NULL;
	s->echo = 0;
	s->next = NULL;
	return s;
}

static int
#if __STDC__
getsc_(void)
#else
getsc_()
#endif /* Xn 1992-07-17 */
{
	register Source *s = source;
	register int c;
#if 0
	extern void	mprint();
#endif /* Xn 1992-07-29 */

#if 0 && XN
{
	static const char *type_str[] = {
		"SEOF", "STTY", "SFILE", "SWSTR", "SSTRING", "SWORDS", "SALIAS", "SHIST", "SWORDSEP"
	};
#if 0
	char buf[2];
#endif

	(void) fprintf(stderr, "s->str: %p", s->str);
	if (s->str != NULL)
		(void) fprintf(stderr, " \"%s\"", s->str);
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "s->type: ");
	if (s->type >= 0 && s->type <= 8)
		(void) fprintf(stderr, "%s\n", type_str[s->type]);
	else
		(void) fprintf(stderr, "%d\n", s->type);
	if (s->type == SWSTR || s->type == SSTRING || s->type == SWORDS || s->type == SWORDSEP)
		(void) fprintf(stderr, "s->u.strv: %p\n", s->u.strv);
	else if (s->type == SEOF || s->type == STTY || s->type == SFILE)
		(void) fprintf(stderr, "s->u.file: %p\n", s->u.file);
	else
		(void) fprintf(stderr, "s->u.tblp: %p\n", s->u.tblp);
	(void) fprintf(stderr, "s->line: %d\n", s->line);
	(void) fprintf(stderr, "s->file: %p", s->file);
	if (s->file != NULL)
		(void) fprintf(stderr, " \"%s\"", s->file);
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "s->echo: %d\n", s->echo);
	(void) fprintf(stderr, "s->next: %p\n", s->next);
	(void) fflush(stderr);
#if 0
	(void) fprintf(stderr, "Press Enter to continue: ");
	(void) fflush(stderr);
	(void) gets(buf);
#endif
}
#endif /* Xn 1993-06-15 */
	while ((c = *s->str++) == 0) {
		s->str = NULL;		/* return 0 for EOF by default */
		switch (s->type) {
		  case SEOF:
			s->str = NULLSTR;
			return 0;

		  case STTY:
			if (histpush < 0) {	/* commands pushed by dofc */
				s->type = SHIST;
				s->str = NULLSTR;
				continue;
			}
			s->line++;
			s->str = line;
			line[0] = '\0';
			mprint();
			pprompt(prompt);
			flushshf(1);	flushshf(2);
#ifdef EDIT
#ifdef EMACS
			if (flag[FEMACS])
				c = x_read(ttyfd, line, LINE);
			else
#endif
#ifdef VI
			if (flag[FVI])
				c = x_read(ttyfd, line, LINE);
			else
#endif
#endif
#if 0 && XN
			{
				int err;
#if 0 && XN
				int ret;
				struct termios t;

				ret = tcgetattr(ttyfd, &t);
				err = errno;
				(void) fprintf(stderr, "getsc_ - tcgetattr: %d\n", ret);
				if (ret == -1)
					(void) fprintf(stderr, "getsc_ - errno: %d\n", err);
				else
					(void) fprintf(stderr, "getsc_ - MIN: %ld; TIME: %ld\n",
						(long) t.c_cc[VMIN], (long) t.c_cc[VTIME]);
				(void) fflush(stderr);
#endif /* Xn 1993-05-26 */
#endif /* Xn 1993-05-26 */
#if 0 && XN
				(void) fprintf(stderr, "getsc_ - before read(%d, ...)\n", ttyfd);
				(void) fflush(stderr);
#endif /* Xn 1993-05-26 */
				c = read(ttyfd, line, LINE);
#if 0 && XN
				err = errno;
				(void) fprintf(stderr, "getsc_ - after read - c: %d\n", c);
				if (c == -1)
					(void) fprintf(stderr, "isatty(%d): %d; errno: %d\n", ttyfd, isatty(ttyfd), err);
				else
					(void) fprintf(stderr, "getsc_ - line: \"%*s\"\n", c - 1, line);
				(void) fflush(stderr);
			}
#endif /* Xn 1993-06-14 */
			if (c < 0)	/* read error */
				c = 0;
			line[c] = '\0';
			prompt = strval(global("PS2"));
			if (c == 0) { /* EOF */
				s->str = NULL;
				s->line--;
			} else {
				c = 0;
				while(line[c] && ctype(line[c], C_IFS))
					c++;
				if (!line[c]) {
					s->str = &line[c - 1];
					s->line--;
				} else {
					s->str = &line[c];
					histsave(s->str);
				}
			}
			break;

		  case SHIST:
			if (histpush == 0) {
				s->type = STTY;
				s->str = NULLSTR;
				continue;
			}
			s->line++;
			s->str = histptr[++histpush];
#if 0
			pprompt("!< ");	/* todo: PS9 */
#endif
			shellf("%s\n", s->str);
			strcpy(line, s->str);
			s->str = strchr(line, 0);
			*s->str++ = '\n';
			*s->str = 0;
			s->str = line;
			break;

		  case SFILE:
			s->line++;
#if 0 && XN
			(void) fprintf(stderr, "in SFILE - before fgets\n");
			(void) fflush(stderr);
#endif
			s->str = fgets(line, LINE, s->u.file);
#if 0 && XN
			(void) fprintf(stderr, "after fgets - s->str: ");
			if (s->str == NULL)
				(void) fprintf(stderr, "NULL; s->u.file->_file: %d", s->u.file->_file);
			else
				(void) fprintf(stderr, "\"%s\"", s->str);
			(void) fprintf(stderr, "\n");
			(void) fflush(stderr);
#endif
			if (s->str == NULL)
				if (s->u.file != stdin)
#if 0 && XN
				{
					(void) fprintf(stderr, "s->u.file->_file: %d\n", s->u.file->_file);
					(void) fflush(stderr);
#endif
					fclose(s->u.file);
#if 0 && XN
				}
#endif
			break;

		  case SWSTR:
			break;

		  case SSTRING:
			s->str = "\n";
			s->type = SEOF;
			break;

		  case SWORDS:
			s->str = *s->u.strv++;
			s->type = SWORDSEP;
			break;

		  case SWORDSEP:
			if (*s->u.strv == NULL) {
				s->str = "\n";
				s->type = SEOF;
			} else {
				s->str = " ";
				s->type = SWORDS;
			}
			break;

		  case SALIAS:
			s->str = s->u.tblp->val.s;
			if (s->str[0] != 0) {
				c = strchr(s->str, 0)[-1];
				if (c == ' ' || c == '\t')
					alias = 1;	/* trailing ' ' */
			}
			source = s = s->next;
			expanding_alias = 1;
			continue;
		}
		if (s->str == NULL) {
			s->type = SEOF;
			s->str = NULLSTR;
			return 0;
		}
		if (s->echo)
			fputs(s->str, shlout);
	}
#if WIN_NT
	if (c == '\r' && *s->str == '\n')
		c = *s->str++;
#endif /* Xn 1992-09-03 */
#if 0 && XN
	(void) fprintf(stderr, "exiting getsc_() - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
	(void) fflush(stderr);
#endif
	return c;
}

void
#if __STDC__
pprompt(register char *cp)
#else
pprompt(cp)
	register char *cp;
#endif /* Xn 1992-07-17 */
{
	while (*cp != 0)
		if (*cp != '!')
			putc(*cp++, shlout);
		else
			if (*++cp == '!')
				putc(*cp++, shlout);
			else
				shellf("%d", source->line);
	fflush(shlout);
}
