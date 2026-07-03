/*
 * startup, main loop, enviroments and error handling
 */

#ifndef lint
static char *RCSid = "$Id: main.c,v 3.3 89/03/27 15:51:39 egisin Exp $";
static char *sccs_id = "@(#)main.c	1.3 91/11/09 15:34:27 (sjg)";
#endif

#define	Extern				/* define Externs in sh.h */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <time.h>
#if WIN_NT && PROFILE_KLUDGE
#include <limits.h>
#endif
#include "sh.h"
#include "lex.h"
#include "tree.h"
#include "table.h"

/*
 * global data
 */

Area	aperm;

#if 1
extern	void	keywords ARGS((void)); /* in syn.c */
extern	void	builtin ARGS((char *, int (*) ARGS((char **)))); /* in exec.c */
extern	int	shcomexec ARGS((register char **)); /* in exec.c */
extern	void	ignoresig ARGS((int i)); /* in trap.c */
extern	void	mcheck ARGS((void)); /* in mail.c */
extern	void	runtraps ARGS((void)); /* in trap.c */
extern	int	j_reap ARGS((void)); /* in jobs.c */
extern	void	runtrap ARGS((Trap *)); /* in trap.c */

int	include ARGS((register char *));
int	shell ARGS((Source *));
void	unwind ARGS((void));
void	newenv ARGS((int));
void	quitenv ARGS((void));
#endif /* Xn 1992-07-29 */

static	void	reclaim ARGS((void));

/*
 * shell initialization
 */

static	char	initifs [] = "IFS= \t\n"; /* must be R/W */

static	Const	char   initsubs [] = 
#ifdef sun				/* sun's don't have a real /bin */
  "${SHELL:=/usr/bin/sh} ${PATH:=/usr/bin:/usr/ucb:.} ${HOME:=/} ${PS1:=$ } ${PS2:=> } ${MAILCHECK:=600}"; /* Xn 1992-08-14 */
#else
#if WIN_NT
  "${SHELL:=/bin/sh} ${PATH:=/bin:/usr/bin:.} ${HOME:=/} ${LOGNAME:=root} ${PS1:=$ } ${PS2:=> } ${TZ:=UCT0} ${MAILCHECK:=600}";
#else
  "${SHELL:=/bin/sh} ${PATH:=/bin:/usr/bin:.} ${HOME:=/} ${PS1:=$ } ${PS2:=> } ${MAILCHECK:=600}";
#endif /* Xn 1992-08-19 */
#endif

#if WIN_NT
Const	char *initcoms [] = {
#else
static	Const	char *initcoms [] = {
#endif /* Xn 1992-08-17 */
	"cd", ".", NULL,		/* set up $PWD */
#if WIN_NT
	"typeset", "-x", "SHELL", "PATH", "HOME", "LOGNAME", "TZ", NULL,
	"typeset", "-r", "HOME", "LOGNAME", "PWD", "OLDPWD", NULL,
	"typeset", "-p", "COMSPEC", "ENV", "EXECSHELL", "HOME", "HOMEPATH",
	  "INCLUDE", "INIT", "LIB", "LIBPATH", "NTMAKEENV", "OS2LIBPATH",
	  "PATH", "SHELL", "SYSTEMROOT", "TEMP", "TERMCAP", "TMP", "WINDIR",
	  "_NTDRIVE", "_PSXLIBPATH", NULL,
#else
	"typeset", "-x", "SHELL", "PATH", "HOME", NULL,
	"typeset", "-r", "PWD", "OLDPWD", NULL,
#endif /* -p represents "convert to POSIX" - Xn 1993-06-14 */
	"typeset", "-i", "SECONDS=0", "OPTIND=1", NULL,
	"alias",
	  "integer=typeset -i", "pwd=print -r \"$PWD\"",
	  "history=fc -l", "r=fc -e -", "nohup=nohup ",
	  "login=exec login", "newgrp=exec newgrp",
	  "type=whence -v", "functions=typeset -f",
#if 1
	  "echo=print -", "true=:", "false=let", "[=\\[", NULL,
#else
	  "echo=print", "true=:", "false=let", "[=\\[", NULL,
#endif /* Xn 1992-09-08 */
	NULL
};

#ifdef USE_TRACE
/*
 * use SIGUSR1 to bump up Trace_level
 * use SIGUSR2 to clear Trace_level
 */
void
#if __STDC__
set_TraceLev(int sig)
#else
set_TraceLev(sig)
	int sig;
#endif /* Xn 1992-07-17 */
{
  switch(sig)
  {
  case SIGUSR1:
    Trace_level++;
    break;
  case SIGUSR2:
    Trace_level = 0;
    break;
  }
  if (sig > 0)
    (void) signal(sig, set_TraceLev);
  return;
}
#endif

#if 0
main(argc, argv, envp)
	int argc;
	register char **argv;
	char **envp;
#else
int
#if __STDC__
main(int argc, char **argv)
#else
main(argc, argv)
	int argc;
	char **argv;
#endif
#endif /* Xn 1992-07-17 */
{
	register int i;
	register char *arg;
	int cflag = 0, qflag = 0, fflag = 0;
	char *name;
	register Source *s;
	register struct block *l = &globals;
	register char **wp0, **wp;
	extern char ksh_version [];
#if 1
	char **envp;
	extern char **environ;
#endif /* Xn 1992-07-16 */
#if 0
	extern time_t time();
#endif /* Xn 1992-07-17 */

#if WIN_NT
	envp = environ;
#endif /* Xn 1992-07-16 */
#if 0 && XN
	(void) printf("stdout\n");
	(void) fflush(stdout);
	(void) fprintf(stderr, "stderr\n");
	(void) fflush(stderr);
	(void) fprintf(stderr, "main - entering\n");
	(void) fflush(stderr);
#endif /* Xn 1993-05-26 */
	(void) fflush(stderr);
	ainit(&aperm);		/* initialize permanent Area */

	/* set up base enviroment */
	e.type = E_NONE;
	ainit(&e.area);
	e.loc = l;
	e.savefd = NULL;
	e.oenv = NULL;

	initctypes();
#if 0 && XN
	(void) fprintf(stderr, "main - before fopenshf calls\n");
	(void) fflush(stderr);
#endif /* Xn 1993-05-26 */

	/* open file streams for fd's 0,1,2 */
	fopenshf(0);	fopenshf(1);	fopenshf(2);
#if 0 && XN
	(void) fprintf(stderr, "main - after fopenshf calls\n");
	(void) fflush(stderr);
#endif /* Xn 1993-05-26 */

	/* set up variable and command dictionaries */
	newblock();		/* set up global l->vars and l->funs */
	tinit(&commands, APERM);
	tinit(&builtins, APERM);
	tinit(&lexicals, APERM);
	tinit(&homedirs, APERM);

	/* import enviroment */
#if 0 && XN
	(void) fprintf(stderr, "main - before import - envp: %p\n", envp);
	(void) fflush(stderr);
#endif /* Xn 1993-05-26 */
#if WIN_NT
	kshpid = (pid_t) -1; /* flag for initializing typeset -p variables */
#endif /* Xn 1992-08-17 */
	if (envp != NULL)
		for (wp = envp; *wp != NULL; wp++)
			import(*wp);
#if 0 && XN
	(void) fprintf(stderr, "main - after import - environ:");
	for (wp = environ; *wp != NULL; ++wp)
		(void) fprintf(stderr, " \"%s\"", *wp);
	(void) fputc('\n', stderr);
	(void) fflush(stderr);
#endif /* Xn 1993-05-26 */

	kshpid = getpid();
	typeset(initifs, 0, 0);	/* for security */
	typeset(ksh_version, 0, 0); /* RDONLY */

#ifdef USE_TRACE
	(void) signal(SIGUSR1, set_TraceLev);
	(void) signal(SIGUSR2, set_TraceLev);
	_TRACE(0, ("Traces enabled.")); /* allow _TRACE to setup */
#endif

	/* define shell keywords */
	keywords();

	/* define built-in commands */
	for (i = 0; shbuiltins[i].name != NULL; i++)
		builtin(shbuiltins[i].name, shbuiltins[i].func);
	for (i = 0; kshbuiltins[i].name != NULL; i++)
		builtin(kshbuiltins[i].name, kshbuiltins[i].func);

	/* assign default shell variable values */
	substitute(initsubs, 0);
	setint(typeset("PPID", INTEGER, 0), (long) getppid());
	typeset("PPID", RDONLY, 0);
	setint(typeset("RANDOM", INTEGER, 0), (long) time((time_t *)0));
	/* execute initialization statements */
	for (wp0 = (char**) initcoms; *wp0 != NULL; wp0 = wp+1) {
		/* copy because the alias initializers are readonly */
		for (wp = wp0; *wp != NULL; wp++)
			*wp = strsave(*wp, ATEMP);
		shcomexec(wp0);
	}
	afreeall(ATEMP);

	if (geteuid() == 0)
		setstr(global("PS1"), "# ");

	s = pushs(SFILE);
#if 0 && XN
	(void) fprintf(stderr, "s->str: %p", s->str);
	if (s->str != NULL)
		(void) fprintf(stderr, " \"%s\"", s->str);
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "s->type: %d\n", s->type);
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
#endif /* Xn 1993-05-26 */
	s->u.file = stdin;
	cflag = 0;
	name = *argv++;

	/* what a bloody mess */
	if (--argc >= 1) {
		if (argv[0][0] == '-' && argv[0][1] != '\0') {
			for (arg = argv[0]+1; *arg; arg++)
				switch (*arg) {
				  case 'c':
					cflag = 1;
					if (--argc > 0) {
						s->type = SSTRING;
						s->str = *++argv;
					}
					break;
	
				  case 'q':
					qflag = 1;
					break;

				  default:
					if (*arg>='a' && *arg<='z')
						flag[FLAG(*arg)]++;
				}
		} else {
			argv--;
			argc++;
		}
		if (s->type == SFILE && --argc > 0 && !flag[FSTDIN]) {
			if ((s->u.file = fopen(*++argv, "r")) == NULL)
				errorf("%s: cannot open\n", *argv);
			fflag = 1;
			s->file = name = *argv;
			argc--;
#if WIN_NT
			_fileno(s->u.file) = savefd(_fileno(s->u.file));
#else
			fileno(s->u.file) = savefd(fileno(s->u.file));
#endif /* Xn 1993-04-02 */
			setvbuf(s->u.file, (char *)NULL, _IOFBF, BUFSIZ);
		}
	}

	if (s->type == SFILE) {
		if (fileno(s->u.file) == 0)
			flag[FSTDIN] = 1;
		if (isatty(0) && isatty(1) && !cflag && !fflag)
			flag[FTALKING] = 1;
		if (flag[FTALKING] && flag[FSTDIN])
			s->type = STTY;
	}
	if (s->type == STTY) {
		ttyfd = fcntl(0, F_DUPFD, FDBASE);
#if 0 && XN
		(void) fprintf(stderr, "main - s->type == STTY - ttyfd: %d; isatty(%d): %d\n", ttyfd, ttyfd, isatty(ttyfd));
		(void) fflush(stderr);
#endif /* Xn 1993-05-26 */
#if _POSIX_SOURCE
		(void) fcntl(ttyfd, F_SETFD, FD_CLOEXEC);
#else
		(void) fcntl(ttyfd, F_SETFD, FD_CLEXEC);
#endif /* Xn 1992-07-16 */
#if 0 && XN
		(void) fprintf(stderr, "main - isatty(0): %d; isatty(%d): %d\n", isatty(0), ttyfd, isatty(ttyfd));
		(void) fflush(stderr);
#endif /* Xn 1993-05-26 */
#ifdef EMACS
		x_init_emacs();
#endif
	}

	/* initialize job control */
	j_init();

	if (!qflag)
		ignoresig(SIGQUIT);

	l->argv = argv;
	l->argc = argc;
	l->argv[0] = name;
	resetopts();

#if WIN_NT && PROFILE_KLUDGE
	if (getppid() == (pid_t) 1) {
		char filename[PATH_MAX+1];

		flag[FTALKING] = 1;
		(void) include("/etc/profile");
		(void) include(strcat(strcpy(filename, strval(global("HOME"))), ".profile"));
	}
#else
	if (name[0] == '-') {
		flag[FTALKING] = 1;
		(void) include("/etc/profile");
		(void) include(".profile");
	}
#endif /* Xn 1992-09-16 */

	/* include $ENV */
	arg = substitute(strval(global("ENV")), DOTILDE);
	if (*arg != '\0')
		(void) include(arg);

	if (flag[FTALKING]) {
		signal(SIGTERM, trapsig);
		ignoresig(SIGINT);
	} else
		flag[FHASHALL] = 1;

#ifdef JOBS			/* todo: could go before includes? */
	if (s->type == STTY) {
		flag[FMONITOR] = 1;
		j_change();
	}
#endif

#if 0 && XN
	(void) fprintf(stderr, "before main/shell\n");
	(void) fflush(stderr);
#endif
	argc = shell(s);
#if 0 && XN
	(void) fprintf(stderr, "after main/shell\n");
	(void) fflush(stderr);
#endif
	leave(argc);
#if 1
	return EXIT_FAILURE;
#endif /* Xn 1992-07-29 */
}

int
#if __STDC__
include(register char *name)
#else
include(name)
	register char *name;
#endif /* Xn 1992-07-17 */
{
	register FILE *f;
	register Source *s;

#if 0 && XN
{
#if 1
	unsigned int ui;

#endif
	(void) fprintf(stderr, "include - entering - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
#if 1
	for (ui = 0; ui < _NFILE; ++ui)
		if (_iob[ui]._ptr != NULL)
			(void) fprintf(stderr, "_iob[%2u]._file: %d\n", ui, _iob[ui]._file);
#endif
	(void) fflush(stderr);
#endif /* 0 && XN */
	if (strcmp(name, "-") != 0) {
#if 0 && XN
		(void) fprintf(stderr, "include - before fopen(\"%s\", \"r\") - isatty(%d): %d\n",
			name, ttyfd, isatty_kludge(ttyfd));
		(void) fflush(stderr);
#endif
		f = fopen(name, "r");
#if 0 && XN
		(void) fprintf(stderr, "include - after fopen(\"%s\", \"r\") - isatty(%d): %d\n",
			name, ttyfd, isatty_kludge(ttyfd));
		(void) fflush(stderr);
#endif
		if (f == NULL)
			return 0;
		/* todo: the savefd doesn't get popped */
#if WIN_NT
		_fileno(f) = savefd(_fileno(f)); /* questionable */
#else
		fileno(f) = savefd(fileno(f)); /* questionable */
#endif /* Xn 1993-04-02 */
#if 0 && XN
		(void) fprintf(stderr, "include - after savefd - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
		(void) fflush(stderr);
#endif
		setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);
#if 0 && XN
		(void) fprintf(stderr, "include - after setvbuf - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
		(void) fflush(stderr);
#endif
	} else
		f = stdin;
#if 0 && XN
	(void) fprintf(stderr, "include - after f is initialized - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
#if 1
	for (ui = 0; ui < _NFILE; ++ui)
		if (_iob[ui]._ptr != NULL)
			(void) fprintf(stderr, "_iob[%2u]._file: %d\n", ui, _iob[ui]._file);
#endif
	(void) fflush(stderr);
}
#endif
	s = pushs(SFILE);
	s->u.file = f;
	s->file = name;
#if 0 && XN
	(void) fprintf(stderr, "include - before shell - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
	(void) fflush(stderr);
#endif
#if 0 && XN
{
	char ch[2];

	(void) fprintf(stderr, "Call include/shell? ");
	(void) fflush(stderr);
	(void) read(ttyfd, ch, sizeof ch);
	(void) fprintf(stderr, "isatty: %d\n", isatty_kludge(ttyfd));
	(void) fflush(stderr);
	if (*ch == 'y')
		shell(s);
}
#else
	/*return*/ shell(s);
#endif
#if 0 && XN
	(void) fprintf(stderr, "include - after shell - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
	(void) fflush(stderr);
#endif
	if (f != stdin)
#if 0 && XN
	{
		(void) fprintf(stderr, "include - fileno(f): %d\n", fileno(f));
		(void) fflush(stderr);
#endif
		fclose(f);
#if 0 && XN
	}
#endif
#if 0 && XN
	(void) fprintf(stderr, "include - exiting - isatty(%d): %d\n", ttyfd, isatty_kludge(ttyfd));
	(void) fflush(stderr);
#endif
	return 1;
}

int
#if __STDC__
command(register char *comm)
#else
command(comm)
	register char *comm;
#endif /* Xn 1992-07-17 */
{
	register Source *s;

	s = pushs(SSTRING);
	s->str = comm;
	return shell(s);
}

/*
 * run the commands from the input source, returning status.
 */
int
#if __STDC__
shell(Source *s)
#else
shell(s)
	Source *s;		/* input source */
#endif /* Xn 1992-07-17 */
{
	struct op *t;
	Volatile int attempts = 13;
	Volatile int wastty;
	Volatile int reading = 0;
#if 0
	extern void mcheck();
#endif /* Xn 1992-07-29 */

	newenv(E_PARSE);
	e.interactive = 1;
	exstat = 0;
#if 0 && XN
	(void) fprintf(stderr, "shell - before setjmp; e.temps: %p\n", e.temps);
	(void) fflush(stderr);
#endif
	if (setjmp(e.jbuf)) {
#if 0 && XN
		(void) fprintf(stderr, "shell - after setjmp (returned non-zero); e.temps: %p\n", e.temps);
		(void) fflush(stderr);
#endif
		/*shellf("<unwind>");*/
		if (trap)	/* pending SIGINT */
			shellf("\n");
		if (reading && s->type == STTY && s->line)
			s->line--;
		sigtraps[SIGINT].set = 0;
	}
#if 0 && XN
	(void) fprintf(stderr, "shell - after setjmp (returned zero); e.temps: %p\n", e.temps);
	(void) fflush(stderr);
#endif

#if 0
	while (1) {
#else
	for (;;) {
#endif /* Xn 1992-07-29 */
		if (trap)
			runtraps();
		if (flag[FTALKING])
			signal(SIGINT, trapsig);

		if (s->next == NULL)
			s->echo = flag[FVERBOSE];

		j_notify();

		if ((wastty = (s->type == STTY)) || s->type == SHIST) {
			prompt = substitute(strval(global("PS1")), 0);
			mcheck();
		}

		reading = 1;
#if 0 && XN
		(void) fprintf(stderr, "before shell/compile\n");
		(void) fflush(stderr);
#endif
		t = compile(s);
#if 0 && XN
		(void) fprintf(stderr, "after shell/compile - t: %p", t);
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
		(void) fflush(stderr);
#endif
		reading = 0;
		j_reap();
		if (t != NULL && t->type == TEOF)
			if (wastty && flag[FIGNEOF] && --attempts > 0) {
				shellf("Use `exit'\n");
				s->type = STTY;
				continue;
			}
			else
				break;
		flushshf(2);	/* flush -v output */

		if (!flag[FNOEXEC] || s->type == STTY)
#if 0 && XN
			(void) fprintf(stderr, "execute(%p, 0)\n", t);
			(void) fflush(stderr);
#else
			execute(t, 0);
#endif

		reclaim();
	}
#if 0
  Error:
#endif /* Xn 1992-07-29 */
	quitenv();
#if 0 && XN
	(void) fprintf(stderr, "exiting shell - exstat: %d\n", exstat);
	(void) fflush(stderr);
#endif
	return exstat;
}

void
#if __STDC__
leave(int rv)
#else
leave(rv)
	int rv;
#endif /* Xn 1992-07-17 */
{
	if (e.type == E_TCOM && e.oenv != NULL)	/* exec'd command */
		unwind();
	runtrap(&sigtraps[0]);
	j_exit();
	exit(rv);
	/* NOTREACHED */
}

void
#if __STDC__
error(void)
#else
error()
#endif /* Xn 1992-07-17 */
{
	if (flag[FERREXIT] || !flag[FTALKING])
		leave(1);
	unwind();
}

/* return to closest error handler or shell(), exit if none found */
void
#if __STDC__
unwind(void)
#else
unwind()
#endif /* Xn 1992-07-17 */
{
#if 0
	while (1)
#else
	for (;;)
#endif /* Xn 1992-07-29 */
		switch (e.type) {
		  case E_NONE:
			leave(1);
			/* NOTREACHED */
		  case E_PARSE:
			longjmp(e.jbuf, 1);
			/* NOTREACHED */
		  case E_ERRH:
			longjmp(e.jbuf, 1);
			/* NOTREACHED */
		  default:
			quitenv();
			break;
		}
}

void
#if __STDC__
newenv(int type)
#else
newenv(type)
	int type;
#endif /* Xn 1992-07-17 */
{
	register struct env *ep;

	ep = (struct env *) alloc(sizeof(*ep), ATEMP);
	*ep = e;
	ainit(&e.area);
	e.type = type;
	e.oenv = ep;
	e.savefd = NULL;
	e.temps = NULL;
}

void
#if __STDC__
quitenv(void)
#else
quitenv()
#endif /* Xn 1992-07-17 */
{
	register struct env *ep;
	register int fd;

#if 0 && XN
{
	static const char *e_types[] = {
		"E_NONE", "E_PARSE", "E_EXEC", "E_LOOP", "4", "E_TCOM", "E_FUNC", "E_ERRH"
	};

	(void) fprintf(stderr, "quitenv - e.loc: %p; ep->loc: %p\n", e.loc, e.oenv->loc);
	(void) fprintf(stderr, "e.type: ");
	if (e.type >= 0 && e.type <= 7)
		(void) fprintf(stderr, "%s", e_types[e.type]);
	else
		(void) fprintf(stderr, "%d", e.type);
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "e.savefd:");
	if (e.savefd == NULL)
		(void) fprintf(stderr, " NULL");
	else
		for (fd = 0; fd < NUFILE; ++fd)
			(void) fprintf(stderr, " %d", e.savefd[fd]);
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "e.oenv: %p\n", e.oenv);
	(void) fprintf(stderr, "e.interactive: %d\n", e.interactive);
	(void) fflush(stderr);
}
#endif
	if ((ep = e.oenv) == NULL)
		exit(exstat);	/* exit child */
	if (e.loc != ep->loc)
		popblock();
	if (e.savefd != NULL)
		for (fd = 0; fd < NUFILE; fd++)
			restfd(fd, e.savefd[fd]);
	reclaim();
	e = *ep;
}

/* remove temp files and free ATEMP Area */
static void
#if __STDC__
reclaim(void)
#else
reclaim()
#endif /* Xn 1992-07-17 */
{
	register struct temp *tp;

	for (tp = e.temps; tp != NULL; tp = tp->next)
		remove(tp->name);
	e.temps = NULL;
	afreeall(&e.area);
}

void
#if __STDC__
aerror(Area *ap, Const char *msg)
#else
aerror(ap, msg)
	Area *ap;
	Const char *msg;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (ap != NULL)
		;
#endif /* Xn 1992-07-29 */
	errorf("alloc internal error: %s\n", msg);
}

