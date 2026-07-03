/*
 * execute command tree
 */

#ifndef lint
static char *RCSid = "$Id: exec.c,v 3.4 89/03/27 15:50:10 egisin Exp $";
static char *sccs_id = "@(#)exec.c	1.3 91/11/09 15:35:22 (sjg)";
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if __STDC__
#include <stdlib.h>
#endif /* Xn 1992-07-20 */
#if _POSIX_SOURCE && WIN_NT
#include <limits.h>
#endif /* Xn 1992-08-03 */
#include "sh.h"
#include "lex.h"
#include "tree.h"
#include "table.h"

#if 1
extern	void	newenv ARGS((int)); /* in main.c */
extern	void	runtraps ARGS((void)); /* in trap.c */
extern	int	gmatch ARGS((register char *, register char *)); /* in misc.c */
extern	int	timex ARGS((struct op *, int)); /* in c_sh.c */
extern	void	quitenv ARGS((void)); /* in main.c */
extern	void	error ARGS((void)); /* in main.c */
extern	int	c_exec ARGS((char **)); /* in c_sh.c */
extern	int	c_builtin ARGS((char **)); /* in c_sh.c */
#if WIN_NT
extern	int	isposix ARGS((Const char *)); /* in sh.c */
#endif /* Xn 1992-09-30 */

int	define ARGS((char *, struct op *));
#endif /* Xn 1992-07-20 */
static	int	comexec ARGS((struct op *t, char **vp, char **ap, int flags));
static	void	iosetup ARGS((struct ioword *iop));
static	void	echo ARGS((char **, char **));
static	int	herein ARGS((char *name, int sub));
#ifdef	SHARPBANG
static	void	scriptexec ARGS((struct op *t, char **ap));
#endif

/*
 * execute command tree
 */
int
#if __STDC__
execute(register struct op *t, Volatile int flags)
#else
execute(t, flags)
	register struct op *t;
	Volatile int flags;	/* if XEXEC don't fork */
#endif /* Xn 1992-07-17 */
{
	int i;
	Volatile int rv = 0; /* Xn 1992-08-13 */
	int pv[2];
	register char **ap;
	char *s, *cp;
	struct ioword **iowp;

#if 1 || !XN
	if (t == NULL)
		return 0;

	if ((flags&XFORK) && !(flags&XEXEC) && t->type != TPIPE)
		return exchild(t, flags); /* run in sub-process */
#else
	(void) fprintf(stderr, "execute - entering\n");
	(void) fflush(stderr);
	if (t == NULL) {
		(void) fprintf(stderr, "execute - exiting (#1)\n");
		(void) fflush(stderr);
		return 0;
	}

	if ((flags&XFORK) && !(flags&XEXEC) && t->type != TPIPE) {
		(void) fprintf(stderr, "execute - before exchild\n");
		(void) fflush(stderr);
		i = exchild(t, flags); /* run in sub-process */
		(void) fprintf(stderr, "execute - after exchild - i: %d\n", i);
		(void) fprintf(stderr, "execute - exiting (#2)\n");
		(void) fflush(stderr);
		return i;
	}
#endif

	newenv(E_EXEC);
	if (trap)
		runtraps();
 
	if (t->ioact != NULL || t->type == TPIPE) {
		e.savefd = (short*) alloc(sizeofN(short, NUFILE), ATEMP);
		for (i = 0; i < NUFILE; i++)
			e.savefd[i] = 0; /* not redirected */
		/* mark fd 0/1 in-use if pipeline */
		if (flags&XPIPEI)
			e.savefd[0] = -1;
		if (flags&XPIPEO)
			e.savefd[1] = -1;
	}

	/* do redirection, to be restored in quitenv() */
	if (t->ioact != NULL)
		for (iowp = t->ioact; *iowp != NULL; iowp++)
#if 0 && XN
		{
			(void) fprintf(stderr, "execute - before iosetup\n");
			(void) fflush(stderr);
#endif
			iosetup(*iowp);
#if 0 && XN
			(void) fprintf(stderr, "execute - after iosetup\n");
			(void) fflush(stderr);
		}
#endif

#if 0 && XN
{
	ssize_t n;
	char buf[BUFSIZ];

	(void) fprintf(stderr, "execute - t: %p", t);
	if (t != NULL)
	{
		static const char *t_types[] = {
			"TEOF", "TCOM", "TPAREN", "TPIPE", "TLIST", "TOR", "TAND", "TFOR", "8", "TCASE",
			"TIF", "TWHILE", "TUNTIL", "TELIF", "TPAT", "TBRACE", "TASYNC", "TFUNCT", "TTIME", "TEXEC"
		};

		(void) fprintf(stderr, "; t->type: ");
		if (t->type < 0 || t->type > 19)
			(void) fprintf(stderr, "%d", t->type);
		else
			(void) fprintf(stderr, "%s", t_types[t->type]);
	}
	(void) fprintf(stderr, "\n");
#if 0
	(void) fprintf(stderr, "Enter password: ");
	(void) fflush(stderr);
	n = read(ttyfd, buf, sizeof buf);
	(void) fprintf(stderr, "isatty: %d; read(%d, buf, %lu): %ld\n",
		isatty_kludge(ttyfd), ttyfd, (unsigned long) sizeof buf, (signed long) n);
#endif
	(void) fflush(stderr);
}
#endif
	switch(t->type) {
	  case TCOM:
		e.type = E_TCOM;
#if 0 && XN
		(void) fprintf(stderr, "execute - before TCOM/comexec\n");
		(void) fflush(stderr);
#endif
		rv = comexec(t, eval(t->vars, DOTILDE),
			     eval(t->args, DOBLANK|DOGLOB|DOTILDE), flags);
#if 0 && XN
		(void) fprintf(stderr, "execute - after TCOM/comexec - rv: %d\n", rv);
		(void) fflush(stderr);
#endif
		break;

	  case TPAREN:
		exstat = rv = execute(t->left, flags|XFORK);
		break;

	  case TPIPE:
		flags |= XFORK;
		flags &= ~XEXEC;
		e.savefd[0] = (short)savefd(0); /* Xn 1992-07-20 */
		e.savefd[1] = (short)savefd(1); /* Xn 1992-07-20 */
		flags |= XPIPEO;
		(void) dup2(e.savefd[0], 0); /* stdin of first */
		while (t->type == TPIPE) {
			openpipe(pv);
			(void) dup2(pv[1], 1);	/* stdout of curr */
#if 0 && XN
			(void) fprintf(stderr, "before TPIPE/while/exchild\n");
			(void) fflush(stderr);
#endif
			exchild(t->left, flags);
#if 0 && XN
			(void) fprintf(stderr, "after TPIPE/while/exchild\n");
			(void) fflush(stderr);
#endif
			(void) dup2(pv[0], 0);	/* stdin of next */
			closepipe(pv);
			flags |= XPIPEI;
			t = t->right;
		}
		flags &= ~ XPIPEO;
		(void) dup2(e.savefd[1], 1); /* stdout of last */
#if 0 && XN
		(void) fprintf(stderr, "before TPIPE/exchild\n");
		(void) fflush(stderr);
#endif
		exchild(t, flags);
#if 0 && XN
		(void) fprintf(stderr, "after TPIPE/exchild\n");
		(void) fflush(stderr);
#endif
		(void) dup2(e.savefd[0], 0); /* close pipe in */
		if (!(flags&XBGND))
			exstat = rv = waitlast();
		break;

	  case TLIST:
		while (t->type == TLIST) {
			execute(t->left, 0);
			t = t->right;
		}
		rv = execute(t, 0);
		break;

	  case TASYNC:
		rv = execute(t->left, flags|XBGND|XFORK);
		break;

	  case TOR:
	  case TAND:
		rv = execute(t->left, 0);
		if (t->right != NULL && (rv == 0) == (t->type == TAND))
			rv = execute(t->right, 0);
		break;

	  case TFOR:
		e.type = E_LOOP;
		ap = (t->vars != NULL) ?
			eval(t->vars, DOBLANK|DOGLOB|DOTILDE) : e.loc->argv + 1;
		while ((i = setjmp(e.jbuf)) != 0) /* Xn 1992-07-20 */
			if (i == LBREAK)
				goto Break1;
		while (*ap != NULL) {
			setstr(global(t->str), *ap++);
			rv = execute(t->left, 0);
		}
	  Break1:
		break;

	  case TWHILE:
	  case TUNTIL:
		e.type = E_LOOP;
		while ((i = setjmp(e.jbuf)) != 0) /* Xn 1992-07-20 */
			if (i == LBREAK)
				goto Break2;
		while ((execute(t->left, 0) == 0) == (t->type == TWHILE))
			rv = execute(t->right, 0);
	  Break2:
		break;

	  case TIF:
	  case TELIF:
		if (t->right == NULL)
			break;	/* should be error */
		rv = execute(t->left, 0) == 0 ?
			execute(t->right->left, 0) :
			execute(t->right->right, 0);
		break;

	  case TCASE:
		cp = evalstr(t->str, 0);
		for (t = t->left; t != NULL && t->type == TPAT; t = t->right)
		    for (ap = t->vars; *ap; ap++)
			if ((s = evalstr(*ap, DOPAT)) != NULL && gmatch(cp, s)) /* Xn 1992-07-20 */
				goto Found;
		break;
	  Found:
		rv = execute(t->left, 0);
		break;

	  case TBRACE:
		rv = execute(t->left, 0);
		break;

	  case TFUNCT:
		rv = define(t->str, t->left);
		break;

	  case TTIME:
		rv = timex(t, flags);
		break;

	  case TEXEC:		/* an eval'd TCOM */
		s = t->args[0];
		ap = makenv();
#ifdef _MINIX				/* no F_SETFD close-on-exec */
		for (i = 10; i < 20; i++)
			close(i);
#endif
#if 0 && XN
{
#if 0
		static const char buf[] = "The gostacks distim the doshes.\n";
		ssize_t n;
		signed int err;
#endif
		char **p;

#if 0
		(void) fprintf(stderr, "execute - before execute/execve #1 - isatty(%d): %d\n", 1, isatty(1));
#endif
		(void) fprintf(stderr, "execute - fcntl(%d, F_GETFD): %d\n", 1, fcntl(1, F_GETFD));
		(void) fprintf(stderr, "\"%s\": ", t->str);
		for (p = t->args; *p != NULL; ++p)
			(void) fprintf(stderr, "\"%s\" ", *p);
		(void) fprintf(stderr, "NULL\n");
		(void) fflush(stderr);
#if 0
		errno = 0;
		n = write(1, buf, sizeof buf-1);
		err = errno;
		(void) fprintf(stderr, "execute - after write - n: %d; errno: %d\n", n, err);
#endif
}
#endif
#if WIN_NT
		if (!isposix(t->str))
			errorf("Not a POSIX program\n");
#endif /* Xn 1992-09-30 */
		execve(t->str, t->args, ap);
		if (errno == ENOEXEC) {
#ifdef	SHARPBANG
			scriptexec(t, ap);
#else
			char *shell;

			shell = strval(global("EXECSHELL"));
			if (shell && *shell) {
				if ((shell = search(shell,path,1)) == NULL)
					shell = SHELL;
			} else {
				shell = SHELL;
			}
			*t->args-- = t->str;
			*t->args = shell;
#if 0 && XN
			(void) fprintf(stderr, "execute - before execute/execve #2\n");
			(void) fflush(stderr);
#endif
#if 0 && XN
{
			char **p;
			int ret, err;

			for (p = t->args; *p != NULL; ++p)
				(void) fprintf(stderr, "\"%s\" ", *p);
			(void) fprintf(stderr, "NULL\n");
			(void) fflush(stderr);
			if (!isposix(t->args[0]))
				errorf("Not a POSIX program\n");
			ret = execve(t->args[0], t->args, ap);
			err = errno;
			(void) fprintf(stderr, "ret: %d; errno: %d\n", ret, err);
			(void) fflush(stderr);
}
#else
#if WIN_NT
			if (!isposix(t->args[0]))
				errorf("Not a POSIX program\n");
#endif /* Xn 1992-09-30 */
			execve(t->args[0], t->args, ap);
#endif
			errorf("No shell\n");
#endif	/* SHARPBANG */
		}
		errorf("%s: %s\n", s, strerror(errno));
	}

#if 0 && XN
	(void) fprintf(stderr, "execute - before execute/quitenv\n");
	(void) fflush(stderr);
#endif
	quitenv();		/* restores IO */
#if 0 && XN
	(void) fprintf(stderr, "execute - after execute/quitenv\n");
	(void) fflush(stderr);
#endif
	if (e.interactive) {	/* flush stdout, shlout */
		fflush(shf[1]);
		fflush(shf[2]);
	}
	if ((flags&XEXEC))
		exit(rv);	/* exit child */
#if 0 && XN
	(void) fprintf(stderr, "exiting execute (#3)\n");
	(void) fflush(stderr);
#endif
	return rv;
}

/*
 * execute simple command
 */

static int
#if __STDC__
comexec(struct op *t, register char **vp, register char **ap, int flags)
#else
comexec(t, vp, ap, flags)
	struct op *t;
	register char **vp, **ap;
	int flags;
#endif /* Xn 1992-07-17 */
{
	int i;
	int rv = 0;
	register char *cp;
	register char **lastp;
	register struct tbl *tp = NULL;
	register struct block *l;
	static struct op texec = {TEXEC};
#if 0
	extern int c_exec(), c_builtin ();
#endif /* Xn 1992-07-16 */

#if 0 && XN
	(void) fprintf(stderr, "entering comexec\n");
	(void) fflush(stderr);
#endif
	if (flag[FXTRACE])
		echo(vp, ap);

	/* snag the last argument for $_ */
	if ((lastp = ap) != NULL && *lastp) { /* Xn 1992-07-20 */
		while (*++lastp)
			;
		setstr(typeset("_",LOCAL,0),*--lastp);
	}	

	/* create new variable/function block */
	l = (struct block*) alloc(sizeof(struct block), ATEMP);
	l->next = e.loc; e.loc = l;
	newblock();

 Doexec:
	if ((cp = *ap) == NULL)
		cp = ":";
#if 0 && XN
	(void) fprintf(stderr, "before comexec/findcom\n");
	(void) fflush(stderr);
#endif
	tp = findcom(cp, flag[FHASHALL]);
#if 0 && XN
	(void) fprintf(stderr, "after comexec/findcom - tp: %p", tp);
	if (tp != NULL)
	{
		static const char *tp_types[] = {
			"CNONE", "CSHELL", "CFUNC", "3", "CEXEC", "CALIAS", "CKEYWD"
		};

		(void) fprintf(stderr, "; tp->type: ");
		if (tp->type < 0 || tp->type > 6)
			(void) fprintf(stderr, "%d", tp->type);
		else
			(void) fprintf(stderr, "%s", tp_types[tp->type]);
	}
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
#endif

	switch (tp->type) {
	  case CSHELL:			/* shell built-in */
		while (tp->val.f == c_builtin) {
			if ((cp = *++ap) == NULL)
				break;
			tp = tsearch(&builtins, cp, hash(cp));
			if (tp == NULL)
				errorf("%s: not builtin\n", cp);
		}
		if (tp->val.f == c_exec) {
			if (*++ap == NULL) {
				e.savefd = NULL; /* don't restore redirection */
				break;
			}
			flags |= XEXEC;
			goto Doexec;
		}
		if ((tp->flag&TRACE))
			e.loc = l->next; /* no local block */
		i = (tp->flag&TRACE) ? 0 : LOCAL;
		while (*vp != NULL)
			(void) typeset(*vp++, i, 0);
#if 0 && XN
{
		char ch[2];
		extern int c_dot(char **wp);
		extern int c_print(register char **wp);

		(void) fprintf(stderr, "c_dot: %p; c_print: %p; tp->val.f: %p\n", c_dot, c_print, tp->val.f);
		(void) fprintf(stderr, "Call tp->val.f? ");
		(void) fflush(stderr);
		*ch = 'n';
		(void) read(ttyfd, ch, sizeof ch);
		if (*ch == 'y')
			rv = (*tp->val.f)(ap);
		(void) fprintf(stderr, "isatty: %d\n", isatty_kludge(ttyfd));
		(void) fflush(stderr);
}
#else
		rv = (*tp->val.f)(ap);
#endif
		break;

	case CFUNC:			/* function call */
		if (!(tp->flag&ISSET))
			errorf("%s: undefined function\n", cp);
		l->argv = ap;
		for (i = 0; *ap++ != NULL; i++)
			;
		l->argc = i - 1;
		resetopts();
		while (*vp != NULL)
			(void) typeset(*vp++, LOCAL, 0);
		e.type = E_FUNC;
		if (setjmp(e.jbuf))
			rv = exstat; /* return # */
		else
			rv = execute(tp->val.t, 0);
		break;

	case CEXEC:		/* executable command */
#if 0 && XN
		(void) fprintf(stderr, "comexec: CEXEC\n");
		(void) fflush(stderr);
#endif
		if (!(tp->flag&ISSET)) {
			/*
			 * mlj addition:
			 *
			 * If you specify a full path to a file
			 * (or type the name of a file in .) which
			 * doesn't have execute priv's, it used to
			 * just say "not found".  Kind of annoying,
			 * particularly if you just wrote a script
			 * but forgot to say chmod 755 script.
			 *
			 * This should probably be done in eaccess(),
			 * but it works here (at least I haven't noticed
			 * changing errno here breaking something
			 * else).
			 *
			 * So, we assume that if the file exists, it
			 * doesn't have execute privs; else, it really
			 * is not found.
			 */
#if _POSIX_SOURCE
			if (access(cp, F_OK) < 0)
#else
			if (access(cp, 0) < 0)
#endif /* Xn 1992-07-31 */
			    shellf("%s: not found\n", cp);
			else
			    shellf("%s: cannot execute\n", cp);
			rv = 1;
			break;
		}

		/* set $_ to program's full path */
		setstr(typeset("_", LOCAL|EXPORT, 0), tp->val.s);
		while (*vp != NULL)
			(void) typeset(*vp++, LOCAL|EXPORT, 0);

		if ((flags&XEXEC)) {
			j_exit();
			if (flag[FMONITOR] || !(flags&XBGND)) {
				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
			}
		}

		/* to fork we set up a TEXEC node and call execute */
		texec.left = t;	/* for tprint */
		texec.str = tp->val.s;
		texec.args = ap;
#if 0 && XN
		(void) fprintf(stderr, "comexec - before TEXEC/exchild\n");
		(void) fflush(stderr);
#endif
		rv = exchild(&texec, flags);
#if 0 && XN
		(void) fprintf(stderr, "comexec - after TEXEC/exchild - rv: %d\n", rv);
		(void) fflush(stderr);
#endif
		break;
	}
	if (rv != 0 && flag[FERREXIT])
		leave(rv);
#if 0 && XN
	(void) fprintf(stderr, "exiting comexec\n");
	(void) fflush(stderr);
#endif
	return (exstat = rv);
}

#ifdef	SHARPBANG
static void
#if __STDC__
scriptexec(register struct op *tp, register char **ap)
#else
scriptexec(tp, ap)
	register struct op *tp;
	register char **ap;
#endif /* Xn 1992-07-17 */
{
	char line[LINE];
	register char *cp;
	register int fd, n;
	char *shell;

	shell = strval(global("EXECSHELL"));
	if (shell && *shell) {
		if ((shell = search(shell,path,1)) == NULL)
			shell = SHELL;
	} else {
		shell = SHELL;
	}

	*tp->args-- = tp->str;
	line[0] = '\0';
#if _POSIX_SOURCE
	if ((fd = open(tp->str, O_RDONLY)) >= 0) {
#else
	if ((fd = open(tp->str,0)) >= 0) {
#endif /* Xn 1992-07-31 */
		if ((n = read(fd, line, LINE - 1)) > 0)
			line[n] = '\0';
		(void) close(fd);
	}
	if (line[0] == '#' && line[1] == '!') {
		cp = &line[2];
		while (*cp && (*cp == ' ' || *cp == '\t'))
			cp++;
		if (*cp && *cp != '\n') {
			*tp->args = cp;
			while (*cp && *cp != '\n' && *cp != ' ' && *cp != '\t')
				cp++;
			if (*cp && *cp != '\n') {
				*cp++ = '\0';
				while (*cp && (*cp == ' ' || *cp == '\t'))
					cp++;
				if (*cp && *cp != '\n') {
					tp->args--;
					tp->args[0] = tp->args[1];
					tp->args[1] = cp;
					while (*cp && *cp != '\n' &&
					       *cp != ' ' && *cp != '\t')
						cp++;
				}
			}
			*cp = '\0';
		} else
			*tp->args = shell;
	} else
		*tp->args = shell;

#if 0 && XN
	(void) fprintf(stderr, "before scriptexec/execve\n");
	(void) fflush(stderr);
#endif
#if 0 && XN
{
	char **p;
	int ret, err;

	for (p = tp->args; *p != NULL; ++p)
		(void) fprintf(stderr, "\"%s\" ", *p);
	(void) fprintf(stderr, "NULL\n");
	(void) fflush(stderr);
	if (!isposix(tp->args[0]))
		errorf("Not a POSIX program\n");
	ret = execve(tp->args[0], tp->args, ap);
	err = errno;
	(void) fprintf(stderr, "ret: %d; errno: %d\n", ret, err);
	(void) fflush(stderr);
}
#else
#if WIN_NT
	if (!isposix(tp->args[0]))
		errorf("Not a POSIX program\n");
#endif /* Xn 1992-09-30 */
	(void) execve(tp->args[0], tp->args, ap);
#endif
	errorf( "No shell\n" );
}
#endif	/* SHARPBANG */

int
#if __STDC__
shcomexec(register char **wp)
#else
shcomexec(wp)
	register char **wp;
#endif /* Xn 1992-07-17 */
{
	register struct tbl *tp;

	tp = tsearch(&builtins, *wp, hash(*wp));
	if (tp == NULL)
		errorf("%s: shcomexec botch\n", *wp);
	return (*tp->val.f)(wp);
}

/*
 * define function
 */
int
#if __STDC__
define(char *name, struct op *t)
#else
define(name, t)
	char	*name;
	struct op *t;
#endif /* Xn 1992-07-17 */
{
	register struct block *l;
	register struct tbl *tp;

	for (l = e.loc; l != NULL; l = l->next) {
		lastarea = &l->area;
		tp = tsearch(&l->funs, name, hash(name));
		if (tp != NULL && (tp->flag&DEFINED))
			break;
		if (l->next == NULL) {
			tp = tenter(&l->funs, name, hash(name));
			tp->flag = DEFINED|FUNCT;
			tp->type = CFUNC;
		}
	}

	if ((tp->flag&ALLOC))
		tfree(tp->val.t, lastarea);
	tp->flag &= ~(ISSET|ALLOC);

	if (t == NULL) {		/* undefine */
		tdelete(tp);
		return 0;
	}

	tp->val.t = tcopy(t, lastarea);
	tp->flag |= (ISSET|ALLOC);

	return 0;
}

/*
 * add builtin
 */
void
#if __STDC__
builtin(char *name, int (*func) ARGS((char **)))
#else
builtin(name, func)
	char *name;
	int (*func) ARGS((char **));
#endif /* Xn 1992-07-17 */
{
	register struct tbl *tp;
	int flag = DEFINED;

	if (*name == '=') {		/* sets keyword variables */
		name++;
		flag |= TRACE;	/* command does variable assignment */
	}

	tp = tenter(&builtins, name, hash(name));
	tp->flag |= flag;
	tp->type = CSHELL;
	tp->val.f = func;
}

/*
 * find command
 * either function, hashed command, or built-in (in that order)
 */
struct tbl *
#if __STDC__
findcom(char *name, int insert)
#else
findcom(name, insert)
	char	*name;
	int	insert;			/* insert if not found */
#endif /* Xn 1992-07-17 */
{
	register struct block *l = e.loc;
	unsigned int h = hash(name);
	register struct	tbl *tp = NULL;
	static struct tbl temp;

#if 0 && XN
	(void) fprintf(stderr, "entering findcom\n");
	(void) fflush(stderr);
#endif
	if (strchr(name, '/') != NULL) {
		tp = &temp;
		tp->type = CEXEC;
		tp->flag = 0;	/* make ~ISSET */
		goto Search;
	}
	for (l = e.loc; l != NULL; l = l->next) {
#if 0 && XN
		(void) fprintf(stderr, "before findcom/for/tsearch\n");
		(void) fflush(stderr);
#endif
		tp = tsearch(&l->funs, name, h);
#if 0 && XN
		(void) fprintf(stderr, "after findcom/for/tsearch\n");
		(void) fflush(stderr);
#endif
		if (tp != NULL && (tp->flag&DEFINED))
			break;
	}
	if (tp == NULL) {
#if 0 && XN
		(void) fprintf(stderr, "before findcom/if/tsearch #1 - &commands: %p; name: \"%s\"; h: %u\n",
			&commands, name, h);
		(void) fflush(stderr);
#endif
		tp = tsearch(&commands, name, h);
#if 0 && XN
		(void) fprintf(stderr, "after findcom/if/tsearch #1\n");
		(void) fflush(stderr);
#endif
#if _POSIX_SOURCE
		if (tp != NULL && eaccess(tp->val.s, X_OK) != 0) {
#else
		if (tp != NULL && eaccess(tp->val.s,1) != 0) {
#endif /* Xn 1992-07-31 */
#if 0 && XN
			(void) fprintf(stderr, "entering eaccess if\n");
			(void) fflush(stderr);
#endif
			if (tp->flag&ALLOC)
				afree(tp->val.s, commands.areap);
			tp->type = CEXEC;
			tp->flag = DEFINED;
#if 0 && XN
			(void) fprintf(stderr, "exiting eaccess if\n");
			(void) fflush(stderr);
#endif
		}
	}
	if (tp == NULL)
#if 0 && XN
	{
		(void) fprintf(stderr, "before findcom/if/tsearch #2\n");
		(void) fflush(stderr);
#endif
		tp = tsearch(&builtins, name, h);
#if 0 && XN
		(void) fprintf(stderr, "after findcom/if/tsearch #2\n");
		(void) fflush(stderr);
	}
#endif
	if (tp == NULL) {
#if 0 && XN
		(void) fprintf(stderr, "before findcom/tenter\n");
		(void) fflush(stderr);
#endif
		tp = tenter(&commands, name, h);
#if 0 && XN
		(void) fprintf(stderr, "after findcom/tenter\n");
		(void) fflush(stderr);
#endif
		tp->type = CEXEC;
		tp->flag = DEFINED;
	}
  Search:
#if 0 && XN
	(void) fprintf(stderr, "at findcom/Search\n");
	(void) fflush(stderr);
#endif
	if (tp->type == CEXEC && !(tp->flag&ISSET)) {
		if (!insert) {
			tp = &temp;
			tp->type = CEXEC;
			tp->flag = 0;	/* make ~ISSET */
		}
		name = search(name, path, 1);
		if (name != NULL) {
			tp->val.s = strsave(name,
					    (tp == &temp) ? ATEMP : APERM);
			tp->flag |= ISSET|ALLOC;
		}
	}
#if 0 && XN
	(void) fprintf(stderr, "exiting findcom\n");
	(void) fflush(stderr);
#endif
	return tp;
}

/*
 * flush executable commands with relative paths
 */
void
#if __STDC__
flushcom(int all)
#else
flushcom(all)
	int all;		/* just relative or all */
#endif /* Xn 1992-07-17 */
{
	register struct tbl *tp;

	for (twalk(&commands); (tp = tnext()) != NULL; )
		if ((tp->flag&ISSET) && (all || tp->val.s[0] != '/')) {
			if ((tp->flag&ALLOC))
				afree(tp->val.s, commands.areap);
			tp->flag = DEFINED; /* make ~ISSET */
		}
}

/*
 * search for command with PATH
 */
char *
#if __STDC__
search(char *name, char *path, int mode)
#else
search(name, path, mode)
	char *name, *path;
	int mode;		/* 0: readable; 1: executable */
#endif /* Xn 1992-07-17 */
{
#if _POSIX_SOURCE && WIN_NT
	static const char exe[] = ".exe";
	static char name_exe[PATH_MAX+1];
#endif /* Xn 1992-08-13 */
	register int i;
	register char *sp, *tp;
	struct stat buf;

	if (strchr(name, '/'))
#if _POSIX_SOURCE
		return (eaccess(name, (mode) ? X_OK : F_OK) == 0) ? name :
#if WIN_NT
			(eaccess(strcat(strcpy(name_exe, name), exe), (mode) ? X_OK : F_OK) == 0) ? name_exe : NULL;
#else
			NULL;
#endif
#else
		return (eaccess(name, mode) == 0) ? name : NULL;
#endif /* Xn 1992-08-13 */

	sp = path;
	while (sp != NULL) {
		tp = line;
		for (; *sp != '\0'; tp++)
			if ((*tp = *sp++) == ':') {
				--sp;
				break;
			}
		if (tp != line)
			*tp++ = '/';
		for (i = 0; (*tp++ = name[i++]) != '\0';)
			;
#if _POSIX_SOURCE
		i = eaccess(line, (mode) ? X_OK : F_OK);
#else
		i = eaccess(line, mode);
#endif /* Xn 1992-07-31 */
		if (i == 0 && (mode != 1 || stat(line,&buf) == 0 &&
		    (buf.st_mode & S_IFMT) == S_IFREG))
			return line;
#if _POSIX_SOURCE && WIN_NT
		if (i == -1) {
			i = eaccess(strcat(strcpy(name_exe, line), exe), (mode) ? X_OK : F_OK);
			if (i == 0 && (mode != 1 || stat(name_exe, &buf) == 0 && (buf.st_mode & S_IFMT) == S_IFREG))
				return name_exe;
		}
#endif /* Xn 1992-08-13 */
		/* what should we do about EACCES? */
		if (*sp++ == '\0')
			sp = NULL;
	}
	return NULL;
}

/*
 * set up redirection, saving old fd's in e.savefd
 */
static void
#if __STDC__
iosetup(register struct ioword *iop)
#else
iosetup(iop)
	register struct ioword *iop;
#endif /* Xn 1992-07-17 */
{
	register int u = -1;
	char *cp = iop->name;
#if !_POSIX_SOURCE
	extern long lseek();
#endif /* Xn 1992-07-16 */

#if 0 && XN
	(void) fprintf(stderr, "iosetup - iop->unit: %d\n", iop->unit);
	(void) fflush(stderr);
#endif
	if (iop->unit == 0 || iop->unit == 1 || iop->unit == 2)
		e.interactive = 0;
#if 0
	if (e.savefd[iop->unit] != 0)
		errorf("file descriptor %d already redirected\n", iop->unit);
#endif
#if 0
	e.savefd[iop->unit] = savefd(iop->unit);
#else
	e.savefd[iop->unit] = (short)savefd(iop->unit);
#endif /* Xn 1992-07-20 */

	if ((iop->flag&IOTYPE) != IOHERE)
		cp = evalstr(cp, DOTILDE);

#if 0 && XN
	(void) fprintf(stderr, "iosetup - cp: \"%s\"\n", cp);
	(void) fflush(stderr);
#endif
	switch (iop->flag&IOTYPE) {
	  case IOREAD:
#if _POSIX_SOURCE
		u = open(cp, O_RDONLY);
#else
		u = open(cp, 0);
#endif /* Xn 1992-07-31 */
		break;

	  case IOCAT:
#if _POSIX_SOURCE
		if ((u = open(cp, O_WRONLY)) >= 0) {
			(void) lseek(u, (off_t)0, SEEK_END);
			break;
		}
#else
		if ((u = open(cp, 1)) >= 0) {
			(void) lseek(u, (long)0, 2);
			break;
		}
#endif /* Xn 1992-07-31 */
		/* FALLTHROUGH */
	  case IOWRITE:
#if 0 && XN
		u = dup(11);
		(void) fprintf(stderr, "iosetup - u: %d; isatty(u): %d\n", u, isatty_kludge(u));
		(void) fflush(stderr);
#else
#if _POSIX_SOURCE
		u = creat(cp, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#else
		u = creat(cp, 0666);
#endif /* Xn 1992-07-31 */
#endif
		break;

	  case IORDWR:
#if _POSIX_SOURCE
		u = open(cp, O_RDWR);
#else
		u = open(cp, 2);
#endif /* Xn 1992-07-31 */
		break;

	  case IOHERE:
		u = herein(cp, iop->flag&IOEVAL);
		/* cp may have wrong name */
		break;

	  case IODUP:
		if (*cp == '-')
			close(u = iop->unit);
		else
		if (digit(*cp))
			u = *cp - '0';
		else
			errorf("%s: illegal >& argument\n", cp);
		break;
	}
#if 0 && XN
	(void) fprintf(stderr, "iosetup - u: %d\n", u);
	(void) fflush(stderr);
#endif
	if (u < 0)
		errorf("%s: cannot %s\n", cp,
		       (iop->flag&IOTYPE) == IOWRITE ? "create" : "open");
	if (u != iop->unit) {
		(void) dup2(u, iop->unit);
		if (iop->flag != IODUP)
			close(u);
	}

#if 0 && XN
	(void) fprintf(stderr, "iosetup - before fopenshf - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
	(void) fflush(stderr);
#endif
	fopenshf(iop->unit);
#if 0 && XN
	(void) fprintf(stderr, "iosetup - after fopenshf - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
	(void) fflush(stderr);
#endif
}

/*
 * open here document temp file.
 * if unquoted here, expand here temp file into second temp file.
 */
static int
#if __STDC__
herein(char *hname, int sub)
#else
herein(hname, sub)
	char *hname;
	int sub;
#endif /* Xn 1992-07-17 */
{
	int fd;
	FILE * Volatile f = NULL;

	f = fopen(hname, "r");
	if (f == NULL)
		return -1;
	setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);

	if (sub) {
		char *cp;
		struct source *s;
		struct temp *h;

		newenv(E_ERRH);
		if (setjmp(e.jbuf)) {
			if (f != NULL)
				fclose(f);
			quitenv();
			return -1; /* todo: error()? */
		}

		/* set up yylex input from here file */
		s = pushs(SFILE);
		s->u.file = f;
		source = s;
		if (yylex(ONEWORD) != LWORD)
			errorf("exec:herein error\n");
		cp = evalstr(yylval.cp, 0);

		/* write expanded input to another temp file */
		h = maketemp(ATEMP);
		h->next = e.temps; e.temps = h;
		if (h == NULL)
			error();
		f = fopen(h->name, "w+");
		if (f == NULL)
			error();
		setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);
		fputs(cp, f);
		rewind(f);

		quitenv();
	}
	fd = dup(fileno(f));
	fclose(f);
	return fd;
}

static void
#if __STDC__
echo(register char **vp, register char **ap)
#else
echo(vp, ap)
	register char **vp, **ap;
#endif /* Xn 1992-07-17 */
{
	shellf("+");
	while (*vp != NULL)
		shellf(" %s", *vp++);
	while (*ap != NULL)
		shellf(" %s", *ap++);
	shellf("\n");
}

