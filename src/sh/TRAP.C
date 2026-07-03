/*
 * signal handling
 */

static char *RCSid = "$Id: trap.c,v 3.2 89/03/27 15:52:06 egisin Exp $";

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include "sh.h"

#if 1
extern int getn ARGS((char *)); /* in misc.c */
extern void unwind ARGS((void)); /* in main.c */
extern int command ARGS((register char *)); /* in main.c */

#endif /* Xn 1992-07-31 */
Trap sigtraps [SIGNALS] = {
	{0,	"EXIT", "Signal 0"}, /* todo: belongs in e.loc->exit */
#if WIN_NT
	{SIGABRT, "ABRT", "Abort"},
	{SIGALRM, "ALRM", "Alarm clock"},
	{SIGFPE, "FPE", "Floating exception"},
	{SIGHUP, "HUP", "Hangup"},
	{SIGILL, "ILL", "Illegal instruction"},
	{SIGINT, "INT", "Interrupt"},
	{SIGKILL, "KILL", "Killed"},
	{SIGPIPE, "PIPE", "Broken pipe"},
	{SIGQUIT, "QUIT", "Quit"},
	{SIGSEGV, "SEGV", "Memory fault"},
	{SIGTERM, "TERM", "Terminated"},
	{SIGUSR1, "USR1", "User defined signal 1"},
	{SIGUSR2, "USR2", "User defined signal 2"},
#ifdef JOBS
	{SIGCHLD, "CHLD", "Death of a child"},
	{SIGCONT, "CONT", "Continue"},
	{SIGSTOP, "STOP", "Stop (signal)"},
	{SIGTSTP, "TSTP", "Stop"},
	{SIGTTIN, "TTIN", "Stop (tty input)"},
	{SIGTTOU, "TTOU", "Stop (tty output)"},
#endif /* JOBS */
#else /* !WIN_NT */
	{SIGHUP, "HUP", "Hangup"},
	{SIGINT, "INT", "Interrupt"},
	{SIGQUIT, "QUIT", "Quit"},
	{SIGILL, "ILL", "Illegal instruction"},
	{SIGTRAP, "TRAP", "Trace trap"},
#ifdef	SIGABRT
	{SIGABRT, "ABRT", "Abort"},
#else
	{SIGIOT, "IOT", "IOT instruction"},
#endif
	{SIGEMT, "EMT", "EMT trap"},
	{SIGFPE, "FPE", "Floating exception"},
	{SIGKILL, "KILL", "Killed"},
	{SIGBUS, "BUS", "Bus error"},
	{SIGSEGV, "SEGV", "Memory fault"},
	{SIGSYS, "SYS", "Bad system call"},
	{SIGPIPE, "PIPE", "Broken pipe"},
	{SIGALRM, "ALRM", "Alarm clock"},
	{SIGTERM, "TERM", "Terminated"},
#ifdef _SYSV
	{SIGUSR1, "USR1", "User defined signal 1"},
	{SIGUSR2, "USR2", "User defined signal 2"},
	{SIGCLD, "CLD", "Death of a child"},
	{SIGPWR, "PWR", "Power-fail restart"},
#else
#ifdef JOBS			/* todo: need to be more portable */
	{SIGURG, "URG", "Urgent condition"}, /* BSDism */
	{SIGSTOP, "STOP", "Stop (signal)"},
	{SIGTSTP, "TSTP", "Stop"},
	{SIGCONT, "CONT", "Continue"},
	{SIGCHLD, "CHLD", "Waiting children"},
	{SIGTTIN, "TTIN", "Stop (tty input)"},
	{SIGTTOU, "TTOU", "Stop (tty output)"},
#endif
#endif
#endif /* WIN_NT - Xn 1992-07-16 */
};

Trap *
#if __STDC__
gettrap(char *name)
#else
gettrap(name)
	char *name;
#endif /* Xn 1992-07-17 */
{
	int i;
	register Trap *p;

	if (digit(*name)) {
		i = getn(name);
#if 0
		return (0 <= i && i < SIGNALS) ? &sigtraps[getn(name)] : NULL;
#else
		return (0 <= i && i < SIGNALS && sigtraps[i].name != NULL) ? &sigtraps[i] : NULL;
#endif /* Xn 1992-07-16 */
	}
#if 0
	if (strcmp("ERR", name) == 0)
		return &e.loc->err;
	if (strcmp("EXIT", name) == 0)
		return &e.loc->exit;
#endif
	for (p = sigtraps, i = SIGNALS; --i >= 0; p++)
		if (p->name != NULL && strcmp(p->name, name) == 0)
			return p;
	return NULL;
}

/*
 * trap signal handler
 */
void
#if __STDC__
trapsig(int i)
#else
trapsig(i)
	int i;
#endif /* Xn 1992-07-17 */
{
	trap = sigtraps[i].set = 1;
	if (i == SIGINT && e.type == E_PARSE)
		/* dangerous but necessary to deal with BSD silly signals */
		longjmp(e.jbuf, 1);
	(void) signal(i, trapsig); /* todo: use sigact */
}

#if 1
void runtrap ARGS((Trap *));

#endif /* Xn 1992-07-31 */
/*
 * run any pending traps
 */
void
#if __STDC__
runtraps(void)
#else
runtraps()
#endif /* Xn 1992-07-17 */
{
	int i;
	register Trap *p;

	for (p = sigtraps, i = SIGNALS; --i >= 0; p++)
		if (p->set)
			runtrap(p);
	trap = 0;
}

void
#if __STDC__
runtrap(Trap *p)
#else
runtrap(p)
	Trap *p;
#endif /* Xn 1992-07-17 */
{
	char *trapstr;

	p->set = 0;
	if ((trapstr = p->trap) == NULL)
		if (p->signal == SIGINT)
			unwind();	/* return to shell() */
		else
			return;
	if (p->signal == 0)	/* ??? */
		p->trap = 0;
	command(trapstr);
}
 
/* restore signals for children */
void
#if __STDC__
cleartraps(void)
#else
cleartraps()
#endif /* Xn 1992-07-17 */
{
	int i;
	register Trap *p;

	if ((p = sigtraps)->trap != NULL) {	/* Maybe put in exchild() */
		afree((Void *)p->trap,APERM);	/* Necessary? */
		p->trap = NULL;
	}
	for (i = SIGNALS, p = sigtraps; --i >= 0; p++) {
		p->set = 0;
		if (p->ourtrap && signal(p->signal, SIG_IGN) != SIG_IGN)
			(void) signal(p->signal, SIG_DFL);
	}
}

void
#if __STDC__
ignoresig(int i)
#else
ignoresig(i)
	int i;
#endif /* Xn 1992-07-17 */
{
	if (signal(i, SIG_IGN) != SIG_IGN)
		sigtraps[i].sig_dfl = 1;
}

void
#if __STDC__
restoresigs(void)
#else
restoresigs()
#endif /* Xn 1992-07-17 */
{
	int i;
	register Trap *p;

	for (p = sigtraps, i = SIGNALS; --i >= 0; p++)
		if (p->sig_dfl) {
			p->sig_dfl = 0;
			(void) signal(p->signal, SIG_DFL);
		}
}

