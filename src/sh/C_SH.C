/*
 * built-in Bourne commands
 */

#ifndef lint
static char *RCSid = "Id: /u/egisin/sh/src/RCS/c_sh.c,v 3.1 88/11/03 09:14:31 egisin Exp $";
static char *sccs_id = "@(#)c_sh.c	1.3 91/11/09 15:35:24 (sjg)";
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/times.h>
#if 1
#include <sys/stat.h>
#include <time.h>
#endif /* Xn 1992-07-20 */
#include <unistd.h>		/* getcwd */
#include "sh.h"
#include "lex.h"
#include "tree.h"
#include "table.h"
#if _POSIX_SOURCE

#define CLK_TCK CLOCKS_PER_SEC
#endif /* Xn 1992-08-04 */

#if 0
static	char *clocktos();
#else
extern int include ARGS((register char *)); /* in main.c */
extern int j_lookup ARGS((char *)); /* in jobs.c */
extern int shell ARGS((Source *)); /* in main.c */
extern int getn ARGS((char *)); /* in misc.c */
extern void quitenv ARGS((void)); /* in main.c */
extern int j_stopped ARGS((void)); /* in jobs.c */
extern int define ARGS((char *, struct op *)); /* in exec.c */
extern int do_ulimit ARGS((char *, char *)); /* in ulimit.c */
extern int c_typeset ARGS((char * Const *args)); /* in c_ksh.c */

static char *clocktos ARGS((clock_t));
#endif /* Xn 1992-07-20 */

int
#if __STDC__
c_label(char **wp)
#else
c_label(wp)
	char **wp;
#endif /* Xn 1992-07-16 */
{
#if 0
	return 0;
#else
	return (wp != wp);
#endif /* Xn 1992-07-20 */
}

int
#if __STDC__
c_shift(register char **wp)
#else
c_shift(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
	register struct block *l = e.loc;
	register int n;

#if 0 && XN
	(void) fprintf(stderr, "c_sh.c/c_shift - wp[1]: \"%s\"\n", wp[1]);
	(void) fflush(stderr);
#endif /* Xn 1993-06-15 */
	n = wp[1] ? evaluate(wp[1]) : 1;
	if (l->argc < n) {
		errorf("nothing to shift\n");
		return (1);
	}
	l->argv[n] = l->argv[0];
	l->argv += n;
	l->argc -= n;
	return 0;
}

int
#if __STDC__
c_umask(register char **wp)
#else
c_umask(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
#if _POSIX_SOURCE
	register mode_t i;
#else
	register int i;
#endif /* Xn 1992-07-20 */
	register char *cp;

	if ((cp = wp[1]) == NULL) {
		i = umask(0);
		umask(i);
#if 0
		printf("%#3.3o\n", i);	/* should this be shell output? */
#else
		printf("%#3.3o\n", (unsigned short)i);	/* should this be shell output? */
#endif /* Xn 1992-07-20 */
	} else {
		for (i = 0; *cp>='0' && *cp<='7'; cp++)
			i = i*8 + (*cp-'0');
		umask(i);
	}
	return 0;
}

int
#if __STDC__
c_dot(char **wp)
#else
c_dot(wp)
	char **wp;
#endif /* Xn 1992-07-16 */
{
	char *file, *cp;

	if ((cp = wp[1]) == NULL)
		return 0;
	file = search(cp, path, 0);
	if (file == NULL)
		errorf("%s: not found\n", cp);
	if (include(file))
		return exstat;
	return -1;
}

int
#if __STDC__
c_wait(char **wp)
#else
c_wait(wp)
	char **wp;
#endif /* Xn 1992-07-16 */
{
	register char *cp;

	wp++;
	cp = *wp;
	if (cp == NULL) cp = "%";
	/* todo: print status ? */
	return waitfor(j_lookup(cp));
}

int
#if __STDC__
c_read(register char **wp)
#else
c_read(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
	register int c = 0;
	FILE *f = stdin;
	int expand = 1;
	register char *cp;

	for (wp++; (cp = *wp) != NULL && *cp++ == '-'; wp++) {
		while (*cp) switch (*cp++) {
		  case 'e':
			expand = 1;
			break;
		  case 'r':
			expand = 0;
			break;
		  case 'u':
			if (!digit(*cp) || (f = shf[*cp++-'0']) == NULL)
				errorf("bad -u argument\n");
			break;
		}
	}

	if (*wp == NULL)
		errorf("missing name\n");
	if ((cp = strchr(*wp, '?')) != NULL) {
		*cp = 0;
		if (flag[FTALKING]) {
			shellf("%s ", cp+1);
			fflush(shlout);
		}
	}

	for (; *wp != NULL; wp++) {
		for (cp = line; cp <= line+LINE; ) {
			if (c == '\n')
				break;
			c = getc(f);
			if (c == EOF)
				return 1;
			if (expand && c == '\\') {
				c = getc(f);
				if (c == '\n')
					c = 0;
				else
#if 0
					*cp++ = c;
#else
					*cp++ = (char)c;
#endif /* Xn 1992-07-20 */
				continue;
			}
			if (c == '\n' || wp[1] && ctype(c, C_IFS))
				break;
#if 0
			*cp++ = c;
#else
			*cp++ = (char)c;
#endif /* Xn 1992-07-20 */
		}
		*cp = 0;
		setstr(global(*wp), line);
	}
	return 0;
}

int
#if __STDC__
c_eval(register char **wp)
#else
c_eval(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
	register struct source *s;

	s = pushs(SWORDS);
	s->u.strv = wp+1;
	return shell(s);
}

void setsig ARGS((struct trap *p, handler_t f));

int
#if __STDC__
c_trap(register char **wp)
#else
c_trap(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
	int i;
	char *s;
	register struct trap *p;

	wp++;
	if (*wp == NULL) {
		for (p = sigtraps, i = SIGNALS; --i >= 0; p++) {
			if (p->trap != NULL)
				shellf("%s: %s\n", p->name, p->trap);
		}
		return 0;
	}

	s = (gettrap(*wp) == NULL) ? *wp++ : NULL; /* get command */
	if (s != NULL && s[0] == '-' && s[1] == '\0')
		s = NULL;

	/* set/clear traps */
	while (*wp != NULL) {
		p = gettrap(*wp++);
		if (p == NULL)
			errorf("trap: bad signal %s\n", wp[-1]);
		if (p->trap != NULL)
			afree((Void*)p->trap, APERM);
		p->trap = NULL;
		if (s != NULL) {
			if (strlen(s) != 0) {
				p->trap = strsave(s, APERM);
				setsig(p, trapsig);
			} else
				setsig(p, (handler_t)SIG_IGN);
		} else
			/* todo: restore to orginal value */
		    setsig(p,
		       (p->signal==SIGINT || p->signal==SIGQUIT) && flag[FTALKING]
			   ? (handler_t)SIG_IGN : (handler_t)SIG_DFL);
	}
	return 0;
}

void
#if __STDC__
setsig(register struct trap *p, handler_t f)
#else
setsig(p, f)
	register struct trap *p;
#if 0
	void (*f)();
#else
	handler_t f;
#endif
#endif /* Xn 1992-07-16 */
{
	if (p->signal == 0)
		return;
	if (signal(p->signal, SIG_IGN) != SIG_IGN || p->ourtrap) {
		p->ourtrap = 1;
		signal(p->signal, f);
	}
}

int
#if __STDC__
c_return(char **wp)
#else
c_return(wp)
	char **wp;
#endif /* Xn 1992-07-16 */
{
	wp++;
	if (*wp != NULL)
		exstat = getn(*wp);
	quitenv();		/* pop E_TCOM */
	while (e.type == E_LOOP || e.type == E_EXEC)
		quitenv();
	if (e.type == E_FUNC)
		longjmp(e.jbuf, 1);
	leave(exstat);
#if 1
	return -1;
#endif /* Xn 1992-07-20 */
}

int
#if __STDC__
c_brkcont(register char **wp)
#else
c_brkcont(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
	int quit;

	quit = wp[1] == NULL ? 1 : getn(wp[1]);
	quitenv();		/* pop E_TCOM */
	while (e.type == E_LOOP || e.type == E_EXEC) {
		if (e.type == E_LOOP && --quit <= 0)
			longjmp(e.jbuf, (*wp[0] == 'b') ? LBREAK : LCONTIN);
		quitenv();
	}
	errorf("cannot %s\n", wp[0]);
#if 1
	return -1;
#endif /* Xn 1992-07-20 */
}


/* 91-05-27 <sjg>
 * we are supposed to not exit first try
 * if there are stopped jobs.
 */
int
#if __STDC__
c_exit(char **wp)
#else
c_exit(wp)
	char **wp;
#endif /* Xn 1992-07-16 */
{
	register char *cp;
	static int extry = 0;
	
#ifdef JOBS
	if (extry++ == 0)
	{
	  if (flag[FMONITOR] && j_stopped()) /* todo: only once */
	  {
	    errorf("There are stopped jobs\n");
	    return 1;
	  }
	}
#endif
	e.oenv = NULL;
	if ((cp = wp[1]) != NULL)
		exstat = getn(cp);
	leave(exstat);
#if 1
	return -1;	/* should never get here */
#endif /* Xn 1993-05-12 */
}

int
#if __STDC__
c_set(register char **wp)
#else
c_set(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
	struct block *l = e.loc;
#if 0
	register struct tbl *vp, **p;
#endif /* Xn 1992-07-20 */
	register char **owp = wp;
	register char *cp;
	int old_fmonitor = flag[FMONITOR];

	if ((cp = *++wp) == NULL) {
		static char * Const args [] = {"set", "-", NULL};
#if 0
		extern int c_typeset ARGS((char **args));
		return c_typeset(args);
#else
		return c_typeset((char * Const *) args);
#endif /* Xn 1992-07-16 */
	}
	
	for (; (cp = *wp) != NULL && (*cp == '-' || *cp == '+');) {
		int i, n = *cp++ == '-'; /* set or clear flag */
		wp++;
		if (*cp == '\0') {
			if (n)
				flag[FXTRACE] = flag[FVERBOSE] = 0;
			break;
		}
		if (*cp == '-')
			goto setargs;
		for (; *cp != '\0'; cp++)
			if (*cp == 'o') {
				if (*wp == NULL) {
					printoptions();
					return 0;
				}
				i = option(*wp++);
				if (i == 0)
					shellf("%s: unknown option\n", *--wp);
#if 0
				flag[i] = n;
				if (i == FEMACS && n)
					flag[FVI] = 0;
				else if (i == FVI && n)
					flag[FEMACS] = 0;
			} else if (*cp>='a' && *cp<='z')
				flag[FLAG(*cp)] = n;
#else
				flag[i] = (char)n;
				if (i == FEMACS && n)
					flag[FVI] = (char)0;
				else if (i == FVI && n)
					flag[FEMACS] = (char)0;
			} else if (*cp>='a' && *cp<='z')
				flag[FLAG(*cp)] = (char)n;
#endif /* Xn 1992-07-20 */
			else
				errorf("%c: bad flag\n", *cp);
		if (flag[FTALKING])
			flag[FERREXIT] = 0;
	}

#ifdef JOBS
	if (old_fmonitor != flag[FMONITOR])
		j_change();
#endif

	/* set $# and $* */
	if (*wp != NULL) {
	  setargs:
		owp = --wp;
		wp[0] = l->argv[0]; /* save $0 */
		while (*++wp != NULL)
			*wp = strsave(*wp, &l->area);
		l->argc = wp - owp - 1;
		l->argv = (char **) alloc(sizeofN(char *, l->argc+2), &l->area);
		for (wp = l->argv; (*wp++ = *owp++) != NULL; )
			;
		resetopts();
	}
	return 0;
}

int
#if __STDC__
c_unset(register char **wp)
#else
c_unset(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
	register char *id;
	int flagf = 0;

	for (wp++; (id = *wp) != NULL && *id == '-'; wp++)
		if (*++id == 'f')
			flagf++;
	for (; (id = *wp) != NULL; wp++)
		if (!flagf) {	/* unset variable */
			unset(global(id));
		} else {	/* unset function */
			define(id, (struct op *)NULL);
		}
	return 0;
}

int
#if __STDC__
c_ulimit(register char **wp)
#else
c_ulimit(wp)
	register char **wp;
#endif /* Xn 1992-07-16 */
{
#if 0
	extern int do_ulimit();
#endif /* Xn 1992-07-20 */

	return do_ulimit(wp[1], wp[2]);
}

int
#if __STDC__
c_times(char **wp)
#else
c_times(wp)
	char **wp;
#endif /* Xn 1992-07-16 */
{
	struct tms all;

	(void) times(&all);
	printf("Shell: ");
	printf("%8s user ", clocktos(all.tms_utime));
	printf("%8s system\n", clocktos(all.tms_stime));
	printf("Kids:  ");
	printf("%8s user ", clocktos(all.tms_cutime));
	printf("%8s system\n", clocktos(all.tms_cstime));

#if 0
	return 0;
#else
	return (wp != wp);
#endif /* Xn 1992-07-20 */
}

/*
 * time pipeline (really a statement, not a built-in command)
 */
int
#if __STDC__
timex(struct op *t, int f)
#else
timex(t, f)
	struct op *t;
	int f;
#endif /* Xn 1992-07-16 */
{
	int rv;
	struct tms t0, t1;
	clock_t t0t, t1t;
	extern clock_t j_utime, j_stime; /* computed by j_wait */

	j_utime = j_stime = 0;
	t0t = times(&t0);
	rv = execute(t->left, f);
	t1t = times(&t1);

	shellf("%8s real ", clocktos(t1t - t0t));
	shellf("%8s user ",
	       clocktos(t1.tms_utime - t0.tms_utime + j_utime));
	shellf("%8s system ",
	       clocktos(t1.tms_stime - t0.tms_stime + j_stime));
	shellf("\n");

	return rv;
}

static char *
#if __STDC__
clocktos(clock_t t)
#else
clocktos(t)
	clock_t t;
#endif /* Xn 1992-07-16 */
{
	static char temp[20];
	register int i;
	register char *cp = temp + sizeof(temp);

#if CLK_TCK != 100		/* convert to 1/100'ths */
	t = (t < 1000000000/CLK_TCK) ?
		(t * 100) / CLK_TCK : (t / CLK_TCK) * 100;
#endif

	*--cp = '\0';
	*--cp = 's';
	for (i = -2; i <= 0 || t > 0; i++) {
		if (i == 0)
			*--cp = '.';
#if 0
		*--cp = '0' + (char)(t%10);
#else
		*--cp = (char)('0' + (char)(t%10));
#endif /* Xn 1992-07-20 */
		t /= 10;
	}
	return cp;
}

/* dummy function, special case in comexec() */
int
#if __STDC__
c_exec(char ** wp)
#else
c_exec(wp)
	char ** wp;
#endif /* Xn 1992-07-16 */
{
#if 0
	return 0;
#else
	return (wp != wp);
#endif /* Xn 1992-07-20 */
}

/* dummy function, special case in comexec() */
int
#if __STDC__
c_builtin(char ** wp)
#else
c_builtin(wp)
	char ** wp;
#endif /* Xn 1992-07-16 */
{
#if 0
	return 0;
#else
	return (wp != wp);
#endif /* Xn 1992-07-20 */
}

#if 0
extern	int c_test ();		/* in test.c */
#else
extern	int c_test ARGS((char **));		/* in test.c */
#endif /* Xn 1992-07-16 */

Const struct builtin shbuiltins [] = {
	{"=:", c_label},
	{"=.", c_dot},
	{"[", c_test},
	{"=builtin", c_builtin},
	{"=exec", c_exec},
	{"=shift", c_shift},
	{"=wait", c_wait},
	{"read", c_read},
	{"=eval", c_eval},
	{"=trap", c_trap},
	{"=break", c_brkcont},
	{"=continue", c_brkcont},
	{"=exit", c_exit},
	{"=return", c_return},
	{"=set", c_set},
	{"unset", c_unset},
	{"umask", c_umask},
	{"test", c_test},
	{"=times", c_times},
	{"ulimit", c_ulimit},
	{NULL, NULL}
};

