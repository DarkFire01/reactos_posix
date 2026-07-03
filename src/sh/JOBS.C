/*
 * Process and job control
 */
#ifndef lint
static char *RCSid = "$Id: jobs.c,v 3.5 89/03/27 15:51:01 egisin Exp $";
static char  sccs_id[] = "@(#)jobs.c     1.4  91/08/16  17:36:59  (sjg)";
#endif

/*
 * based on version by Ron Natalie, BRL
 * modified by Simon J. Gerraty <sjg@melb.bull.oz.au>
 *
 * TODO:
 *	change Proc table to Job table, with array of pids.
 *	make %+ be jobs, %- be jobs->next.
 *	do not JFREE members of pipeline.
 *	consider procs[] related critical sections.
 *	signal(SIGCHLD, j_sigchld) should be
 *	sigaction(SIGCHLD, sigchld, NULL),
 *	with sigchld.sa_flags = SA_RESTART.
 *	There is a simple work-around if there is no SA_RESTART.
 */

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/wait.h>
#include "sh.h"
#include "tree.h"
#ifdef JOBS
#ifdef _BSD
#include <sys/ioctl.h>
#else
#include <termios.h> /* Xn 1992-07-29 */
#endif
#endif
#ifdef _POSIX_SOURCE
#include <stdlib.h>
#include <fcntl.h>
#endif /* Xn 1992-07-29 */

#ifndef WIFCORED
#define	WIFCORED(x)	(!!((x)&0x80)) /* non-standard */
#endif

/* as of P1003.1 Draft 12.3:
 *	pid_t getpgrp(void);		// Get process group id
 *	pid_t setsid(void);		// Create session and Set process group id
 *	int setpgid(pid_t pid, pid_t pgid); // Set process group id for job control
 */

static int sigchld_caught;	/* for recording child terminations */

#ifdef JOBS
#ifdef _BSD			/* _BSD 4.* */
#define	setpgid(p, pg)	setpgrp(p, pg)
#define	getpgid(p)	getpgrp(p)
#define	tcsetpgrp(fd,p)	ioctl(fd, TIOCSPGRP, &(p))
#else				/* POSIX-compatible */
#define	getpgid(p)	getpgrp() /* 1003.1 stupidity */
#define	killpg(p, s)	kill(-(p), s)
#endif
#endif

#ifndef	SIGCHLD
#define	SIGCHLD	SIGCLD
#endif

typedef struct Proc Proc;
struct Proc {
	Proc   *next;		/* `procs' list link */
	int	job;		/* job number: %n */
	Volatile short	state;	/* proc state - Xn 1992-08-13 */
	Volatile short	notify; /* proc state has changed - Xn 1992-08-13 */
	Proc   *prev;		/* prev member of pipeline */
	pid_t	proc;		/* process id */
	pid_t	pgrp;		/* process group if flag[FMONITOR] */
	short	flags;		/* execute flags */
	int	status;		/* wait status */
	clock_t	utime, stime;	/* user/system time when JDONE */
	char	com [48];	/* command */
};

/* proc states */
#define	JFREE	0		/* unused proc */
#define	JRUN	1		/* foreground */
#define JEXIT	2		/* exit termination */
#define	JSIGNAL	3		/* signal termination */
#define	JSTOP	4		/* stopped */
#if 0 && XN

static const char *j_literal[] = { "JFREE", "JRUN", "JEXIT", "JSIGNAL", "JSTOP" };
#endif

static	Proc *procs = NULL;	/* job/process table */

clock_t	j_utime, j_stime;	/* user and system time for last job a-waited */
#ifdef JOBS
#ifdef _POSIX_SOURCE
static	sigset_t	sm_default, sm_sigchld;	/* signal masks */
static	pid_t	our_pgrp;		/* shell's pgrp */
#else
static	int	sm_default, sm_sigchld;	/* signal masks */
static	int	our_pgrp;		/* shell's pgrp */
#endif /* Xn 1992-07-29 */
#endif
static	Proc   *j_lastj;		/* last proc created by exchild */
static	int	j_lastjob = 0;		/* last created job */
static	int	j_current = 0;		/* current job */
static	int	j_previous = 0;		/* previous job */

#if 1
extern	void	restoresigs ARGS((void));
extern	int	snptreef ARGS((char *, int, char *, ...));
extern	void	cleartraps ARGS((void));

void	j_reapchld ARGS((void));

#endif /* Xn 1992-07-29 */
static	int	j_newjob ARGS((void));
static	void	j_print ARGS((Proc *j));
static	Proc   *j_search ARGS((int job));
static	int	j_waitj ARGS((Proc *j, int intr));
static	void	j_sigchld ARGS((int sig));
	
/* initialize job control */
void
#if __STDC__
j_init(void)
#else
j_init()
#endif /* Xn 1992-07-17 */
{
#ifdef JOBS
#ifdef NTTYDISC
	int ldisc = NTTYDISC;	/* BSD brain damage */

	if (ttyfd >= 0)
		ioctl(ttyfd, TIOCSETD, &ldisc);
#endif
	our_pgrp = getpgid(0);
	sigchld_caught = 0;
#ifdef _POSIX_SOURCE
	sigemptyset(&sm_default);
	sigemptyset(&sm_sigchld);
	sigaddset(&sm_sigchld, SIGCHLD);
	_TRACE(5, ("j_init: sm_sigchld == 0x%lX", (long)sm_sigchld));
#else
	sm_default = 0;
	sm_sigchld = sigmask(SIGCHLD);
	_TRACE(5, ("j_init: sm_sigchld == 0x%x", sm_sigchld));
#endif /* Xn 1992-07-29 */
#endif
#if defined(_SYSV) && !defined(JOBS)
	signal(SIGCHLD, SIG_DFL);	/* necessary ??? */
#endif
}

/* job cleanup before shell exit */
void
#if __STDC__
j_exit(void)
#else
j_exit()
#endif /* Xn 1992-07-17 */
{
#ifdef JOBS
	register Proc *j;
#endif
	int killed = 0;

#ifdef JOBS
	/* kill stopped jobs */
	for (j = procs; j != NULL; j = j->next)
		if (j->state == JSTOP) {
			killed ++;
			killpg(j->pgrp, SIGHUP);
			killpg(j->pgrp, SIGCONT);
		}
	if (killed)
		sleep(1);
#endif
	j_notify();

#ifdef JOBS
	if (flag[FMONITOR]) {
		flag[FMONITOR] = 0;
		j_change();
	}
#endif
}

#ifdef JOBS
/* turn job control on or off according to flag[FMONITOR] */
void
#if __STDC__
j_change(void)
#else
j_change()
#endif /* Xn 1992-07-17 */
{
	static handler_t old_tstp, old_ttin, old_ttou;

	if (flag[FMONITOR]) {
		if (ttyfd < 0) {
			flag[FMONITOR] = 0;
			shellf("job control requires tty\n");
			return;
		}
		(void) signal(SIGCHLD, j_sigchld);
		sigtraps[SIGCHLD].sig_dfl = 1; /* restore on fork */
		old_tstp = signal(SIGTSTP, SIG_IGN);
		sigtraps[SIGTSTP].sig_dfl = 1;
		old_ttin = signal(SIGTTIN, SIG_IGN);
		sigtraps[SIGTTIN].sig_dfl = 1;
		old_ttou = signal(SIGTTOU, SIG_IGN);
		sigtraps[SIGTTOU].sig_dfl = 1;
#ifdef _POSIX_SOURCE
		sigprocmask(SIG_SETMASK, &sm_default, NULL);
#else
		sigsetmask(sm_default);
#endif /* Xn 1992-07-29 */
		tcsetpgrp(ttyfd, our_pgrp);
#if 0 && XN
		(void) fprintf(stderr, "jobs.c/j_change() - ttyfd: %d\n", ttyfd);
		(void) fflush(stderr);
#endif
	} else {
		(void) signal(SIGCHLD, SIG_DFL);
		(void) signal(SIGTSTP, old_tstp);
		sigtraps[SIGTSTP].sig_dfl = 0;
		(void) signal(SIGTTIN, old_ttin);
		sigtraps[SIGTTIN].sig_dfl = 0;
		(void) signal(SIGTTOU, old_ttou);
		sigtraps[SIGTTOU].sig_dfl = 0;
	}
}
#endif

/* execute tree in child subprocess */
int
#if __STDC__
exchild(struct op *t, int flags)
#else
exchild(t, flags)
	struct op *t;
	int flags;
#endif /* Xn 1992-07-17 */
{
	register int i;
	register Proc *j;
	int rv = 0;
	int forksleep;

	flags &= ~XFORK;
	if ((flags&XEXEC))
#if 1 || !XN
		return execute(t, flags);
#else
	{
		(void) fprintf(stderr, "exchild - before execute\n");
		(void) fflush(stderr);
		rv = execute(t, flags);
		(void) fprintf(stderr, "exchild - after execute - rv: %d\n", rv);
		(void) fflush(stderr);
		return rv;
	}
#endif

	/* get free Proc entry */
	for (j = procs; j != NULL; j = j->next)
		if (j->state == JFREE)
			goto Found;
	j = (Proc*) alloc(sizeof(Proc), APERM);
	j->next = procs;
	j->state = JFREE;
	procs = j;
  Found:

	j->prev = ((flags&XPIPEI)) ? j_lastj : NULL;
	j->proc = j->pgrp = 0;
	j->flags = (short)flags; /* Xn 1992-07-29 */
	j->job = (flags&XPIPEI) ? j_lastjob : j_newjob();
	snptreef(j->com, sizeof(j->com), "%T", t); /* save proc's command */
	j->com[sizeof(j->com)-1] = '\0';
	j->state = JRUN;

	/* stdio buffer must be flushed and invalidated */
	for (i = 0; i < NUFILE; i++)
		flushshf(i);

	/* create child process */
	forksleep = 0;
#if 0 && XN
	(void) fprintf(stderr, "exchild - before fork - pid: %ld; t: %p; flags: %X; i: %d; j: %p; rv: %d; forksleep: %d\n",
		(signed long) getpid(), t, (unsigned int) flags, i, j, rv, forksleep);
	(void) fflush(stderr);
#endif
	while ((i = (int)fork()) < 0 && errno == EAGAIN && forksleep < 32) { /* Xn 1992-07-29 */
		if (forksleep) {
			sleep(forksleep);
			forksleep <<= 1;
		} else
			forksleep = 1;
	}
	if (i < 0) {
		j->state = JFREE;
		errorf("cannot fork - try again\n");
	}
	j->proc = (i != 0) ? i : getpid();
#if 0 && XN
	(void) fprintf(stderr, "exchild - after fork - pid: %ld; ppid: %ld; t: %p; flags: %X; i: %d; j: %p; rv: %d; "
		"forksleep: %d\n",
		(signed long) getpid(), (signed long) getppid(), t, (unsigned int) flags, i, j, rv, forksleep);
	(void) fflush(stderr);
#endif

#ifdef JOBS
	/* job control set up */
	if (flag[FMONITOR] && !(flags&XXCOM)) {
		j->pgrp = !(flags&XPIPEI) ? j->proc : j_lastj->pgrp;
		/* do in both parent and child to avoid fork race condition */
		if (!(flags&XBGND))
#if 0 && XN
		{
#endif
			tcsetpgrp(ttyfd, j->pgrp); /* could be trouble */
#if 0 && XN
			(void) fprintf(stderr, "jobs.c/exchild() - ttyfd: %d\n", ttyfd);
			(void) fflush(stderr);
		}
#endif
		setpgid(j->proc, j->pgrp);
	}
#endif
	j_lastj = j;

	if (i == 0) {		/* child */
		e.oenv = NULL;
		if (flag[FTALKING])
			restoresigs();
		if ((flags&XBGND) && !flag[FMONITOR]) {
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			if (flag[FTALKING])
				signal(SIGTERM, SIG_DFL);
			if (!(flags&XPIPEI)) {
#if _POSIX_SOURCE
				i = open("/dev/null", O_RDONLY);
#else
				i = open("/dev/null", 0);
#endif /* Xn 1992-07-31 */
				(void) dup2(i, 0);
				close(i);
			}
		}
		for (j = procs; j != NULL; j = j->next)
			j->state = JFREE;
		ttyfd = -1;
#if 0 && XN
		(void) fprintf(stderr, "jobs.c/exchild()/child - ttyfd: %d\n", ttyfd);
		(void) fflush(stderr);
#endif
		flag[FMONITOR] = flag[FTALKING] = 0;
		cleartraps();
#if 0 && XN
		(void) fprintf(stderr, "exchild - child - before execute (no return...)\n");
		(void) fflush(stderr);
#endif
		execute(t, flags|XEXEC); /* no return */
		/* NOTREACHED */
	}

	/* shell (parent) stuff */
	if ((flags&XBGND)) { /* async statement */
		async = (int)j->proc; /* Xn 1992-07-29 */
		j_previous = j_current;
		j_current = j->job;
		if (flag[FTALKING])
			j_print(j);
	} else {		/* sync statement */
		if (!(flags&XPIPE))
#if 0 && XN
		{
			(void) fprintf(stderr, "exchild - parent - before j_waitj\n");
			(void) fflush(stderr);
#endif
			rv = j_waitj(j, 0);
#if 0 && XN
			(void) fprintf(stderr, "exchild - parent - after j_waitj - rv: %d\n", rv);
			(void) fflush(stderr);
		}
#endif
	}

	return rv;
}

/* wait for last job: pipeline or $() sub-process */
int
#if __STDC__
waitlast(void)
#else
waitlast()
#endif /* Xn 1992-07-17 */
{
#if 1 || !XN
	return j_waitj(j_lastj, 0);
#else
	int ret;

	(void) fprintf(stderr, "before waitlast/j_waitj\n");
	(void) fflush(stderr);
	ret = j_waitj(j_lastj, 0);
	(void) fprintf(stderr, "after waitlast/j_waitj\n");
	(void) fflush(stderr);
	return ret;
#endif
}

#if WIN_NT && PAUSE_KLUDGE
static Proc *saved_j;

#endif /* j below gets trashed during the pause(), so this should preserve it - Xn 1992-08-13 */
/* wait for job to complete or change state */
static int
#if __STDC__
j_waitj(Proc *aj, int intr)
#else
j_waitj(aj, intr)
	Proc *aj;
	int intr;		/* interruptable */
#endif /* Xn 1992-07-17 */
{
	register Proc *j;
	int rv = 1;
	int ttysig = 0;

#if 0 && XN
	(void) fprintf(stderr, "entering j_waitj\n");
	(void) fflush(stderr);
#endif
#ifdef JOBS
	if (flag[FMONITOR])
	{
#ifdef _POSIX_SOURCE
	  _TRACE(5, ("j_waitj: sigprocmask(SIG_SETMASK, sm_sigchld==0x%lX, NULL)", (long)sm_sigchld));
	  sigprocmask(SIG_SETMASK, &sm_sigchld, NULL);
#else
	  _TRACE(5, ("j_waitj: sigsetmask(sm_sigchld==0x%x)", sm_sigchld));
	  sigsetmask(sm_sigchld);
#endif /* Xn 1992-07-29 */
	}
#endif
#if 0 && XN
	(void) fprintf(stderr, "before pipeline loop\n");
	(void) fflush(stderr);
#endif
	/* wait for all members of pipeline */
	for (j = aj; j != NULL; j = j->prev) {
		/* wait for job to finish, stop, or ^C of built-in wait */
#if 0 && XN
		(void) fprintf(stderr, "before JRUN loop\n");
		(void) fflush(stderr);
#endif
		while (j->state == JRUN) {
#if 0 && XN
			(void) fprintf(stderr, "beginning JRUN loop\n");
			(void) fflush(stderr);
#endif
#ifdef JOBS
			if (flag[FMONITOR])
			{
			  /*
			   * 91-07-07 <sjg@sun0>
			   * we don't want to wait for a signal
			   * that has already arrived :-)
			   */
			  if (!sigchld_caught)
			  {
#ifdef _POSIX_SOURCE
			    int err;

			    _TRACE(4, ("j_waitj: sigprocmask(SIG_SETMASK, 0x%lX, NULL), sigchld_caught==%d",
				(long)sm_default, sigchld_caught));
#if 0 && XN
			    (void) fprintf(stderr, "before JRUN/sigprocmask - j: %p\n", j);
			    (void) fflush(stderr);
#endif
			    sigprocmask(SIG_SETMASK, &sm_default, NULL);
#if 0 && XN
			    (void) fprintf(stderr, "betwixt JRUN/sigprocmask and JRUN/pause - "
				"aj: %p; intr: %d; j: %p; rv: %d; ttysig: %d\n", aj, intr, j, rv, ttysig);
			    (void) fflush(stderr);
#endif
#if WIN_NT && PAUSE_KLUDGE
			    saved_j = j;
#endif
			    pause();
			    err = errno;
#if WIN_NT && PAUSE_KLUDGE
			    j = saved_j;
#endif
#if 0 && XN
			    (void) fprintf(stderr, "after JRUN/pause - "
				"aj: %p; intr: %d; j: %p; rv: %d; ttysig: %d\n", aj, intr, j, rv, ttysig);
			    (void) fflush(stderr);
#endif
			    _TRACE(4, ("j_waitj: pause()/errno==%d, sigchld_caught==%d", err, sigchld_caught));
#else
			    _TRACE(4, ("j_waitj: sigpause(%d), sigchld_caught==%d", sm_default, sigchld_caught));
			    sigpause(sm_default);
			    _TRACE(4, ("j_waitj: sigpause() returned %d, sigchld_caught==%d", errno, sigchld_caught));
#endif /* Xn 1992-07-29 */
			  }
			}
			else
#endif
#if 0 && XN
			{
				(void) fprintf(stderr, "before JRUN/j_sigchld - j: %p\n", j);
				(void) fflush(stderr);
#endif
				j_sigchld(0);
#if 0 && XN
				(void) fprintf(stderr, "after JRUN/j_sigchld - j: %p\n", j);
				(void) fflush(stderr);
			}
#endif
			/*
			 * Children to reap
			 */
			if (sigchld_caught)
#if 0 && XN
			{
			  (void) fprintf(stderr, "before j_waitj/j_reapchld - j: %p\n", j);
			  (void) fflush(stderr);
#endif
			  j_reapchld();
#if 0 && XN
			  (void) fprintf(stderr, "after j_waitj/j_reapchld - j: %p\n", j);
			  (void) fflush(stderr);
			}
			(void) fprintf(stderr, "before _TRACE - j: %p\n", j);
			(void) fflush(stderr);
#endif
			_TRACE(4, ("j_waitj: j->proc==%d, j->com=='%s', j->state==0x%hx, j->status==0x%x, j->notify==%hd", j->proc, j->com, j->state, j->status, j->notify));
#if 0 && XN
			(void) fprintf(stderr, "after _TRACE - sigtraps[SIGINT].set: %d; intr: %d; j: %p\n",
				sigtraps[SIGINT].set, intr, j);
			(void) fflush(stderr);
#endif
			
			if (sigtraps[SIGINT].set && intr)
#if 0 && XN
			{
				(void) fprintf(stderr, "going to Break\n");
				(void) fflush(stderr);
#endif
				goto Break;
#if 0 && XN
			}
			(void) fprintf(stderr, "ending JRUN loop");
			if (j != NULL)
			{
				(void) fprintf(stderr, "; j->state: %d", j->state);
				if (j->state >= JFREE && j->state <= JSTOP)
				{
					(void) fprintf(stderr, " (%s)", j_literal[j->state]);
					(void) fflush(stderr);
				}
			}
			(void) fprintf(stderr, "\n");
			(void) fflush(stderr);
#endif
		}
#if 0 && XN
		(void) fprintf(stderr, "after JRUN loop\n");
		(void) fflush(stderr);
#endif
		if (j->state == JEXIT) { /* exit termination */
			if (!(j->flags&XPIPEO))
				rv = WEXITSTATUS(j->status);
			j->notify = 0;
		} else
		if (j->state == JSIGNAL) { /* signalled to death */
			if (!(j->flags&XPIPEO))
				rv = 0x80 + WTERMSIG(j->status);
			if (WTERMSIG(j->status) == SIGINT ||
			    WTERMSIG(j->status) == SIGPIPE && (j->flags&XPIPEO))
				j->notify = 0;
			if (WTERMSIG(j->status) == SIGINT ||
			    WTERMSIG(j->status) == SIGQUIT)
				ttysig = 1;
		} else
#ifdef JOBS
		if (j->state == JSTOP)
			if (WSTOPSIG(j->status) == SIGTSTP)
				ttysig = 1;
#else
		;
#endif
	}
#if 0 && XN
	(void) fprintf(stderr, "after pipeline loop\n");
	(void) fflush(stderr);
#endif

	/* compute total child time for time statement */
	for (j = aj; j != NULL; j = j->prev)
		j_utime += j->utime, j_stime += j->stime;

	/* find new current job */
#ifdef JOBS
	if (aj->state == JSTOP) {
		j_previous = j_current;
		j_current = aj->job;
	} else {
#else
	{
#endif
		int hijob = 0;

		/* todo: this needs to be done in j_notify */
		/* todo: figure out what to do with j_previous */
		j_current = 0;
		for (j = procs; j != NULL; j = j->next)
			if ((j->state == JRUN || j->state == JSTOP)
			    && j->job > hijob) {
				hijob = j->job;
				j_current = j->job;
			}
	}

  Break:
#if 0 && XN
	(void) fprintf(stderr, "before job control reset\n");
	(void) fflush(stderr);
#endif
#ifdef JOBS
	if (flag[FMONITOR]) {
		/* reset shell job control state */
#ifdef _POSIX_SOURCE
		sigprocmask(SIG_SETMASK, &sm_default, NULL);
#else
		sigsetmask(sm_default);
#endif /* Xn 1992-07-29 */
		tcsetpgrp(ttyfd, our_pgrp);
#if 0 && XN
	(void) fprintf(stderr, "jobs.c/jwait_j() - ttyfd: %d\n", ttyfd);
	(void) fflush(stderr);
#endif
	}
#endif
#if 0 && XN
	(void) fprintf(stderr, "after job control reset\n");
	(void) fflush(stderr);
#endif
	if (ttysig)
		fputc('\n', shlout);
	j_notify();

#if 0 && XN
	(void) fprintf(stderr, "exiting j_waitj\n");
	(void) fflush(stderr);
#endif
	return rv;
}

/* SIGCHLD handler to reap children */
/*
 * 91-07-07 <sjg@sun0>
 * On the Sun SS2 this appears to get called
 * too quickly!
 * So just record the event and process later.
 */
static void
#if __STDC__
j_sigchld(int sig)
#else
j_sigchld(sig)
	int sig;
#endif /* Xn 1992-07-17 */
{
	sigchld_caught++;	/* acknowledge it */
#if defined(_SYSV) && !defined(JOBS)
	/* non-zero sig means called as handler */
	/* 5.2 handlers must reinstate themselves */
	if (sig) signal(SIGCHLD, j_sigchld);
#else
	if (sig)
		;
#endif /* Xn 1992-07-29 */
}

/*
 * 91-07-07 <sjg@sun0>
 * This now gets called when j_sigchld()
 * has recorded some signals...
 */
void
#if __STDC__
j_reapchld(void)
#else
j_reapchld()
#endif /* Xn 1992-07-17 */
{
	struct tms t0, t1;
#ifdef JOBS
#ifdef _POSIX_SOURCE
	sigset_t sm_old;

	sigprocmask(SIG_BLOCK, NULL, &sm_old);
#else
	int sm_old;

	sm_old = sigblock(0);	/* just get current mask */
#endif /* Xn 1992-07-29 */
#endif
	_TRACE(5, ("j_reapchld: sm_old==0x%x, sigchld_caught==%d", sm_old, sigchld_caught));
	(void) times(&t0);

	do {
		register Proc *j;
#ifdef _POSIX_SOURCE
		pid_t pid;
		int status;
#else
		int pid, status;
#endif /* Xn 1992-07-29 */
#ifdef JOBS
		if (flag[FMONITOR])
			pid = waitpid(-1, &status, (WNOHANG|WUNTRACED));
		else
#endif
			pid = wait(&status);
#if 0 && XN
		(void) fprintf(stderr, "j_reapchld - after waiting in pid %ld - pid: %ld; status: %X",
			(long) getpid(), (long) pid, (unsigned int) status);
		if (pid == (pid_t) 0)
			(void) fprintf(stderr, " - no status available");
		else if (WIFSTOPPED(status))
			(void) fprintf(stderr, " - stopped on signal %d", WSTOPSIG(status));
		else if (WIFEXITED(status))
			(void) fprintf(stderr, " - exit status %d", WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			(void) fprintf(stderr, " - terminated on signal %d", WTERMSIG(status));
		(void) fprintf(stderr, "\n");
		(void) fflush(stderr);
#endif
		if (pid <= 0)	/* return if would block (0) ... */
#if 0 && XN
		{
			(void) fprintf(stderr, "breaking...");
			(void) fflush(stderr);
#endif
			break;	/* ... or no children or interrupted (-1) */
#if 0 && XN
		}
#endif
		(void) times(&t1);

		_TRACE(5, ("j_reapchld: looking for pid==%d", pid));

		for (j = procs; j != NULL; j = j->next)
		{
		  _TRACE(6, ("j_reapchld: j->proc==%d, j->com=='%s', j->state==0x%hx, j->status==0x%x, j->notify==%hd", j->proc, j->com, j->state, j->status, j->notify));
		  if (j->state != JFREE && j->proc == pid)
		    goto Found;
		}
		_TRACE(5, ("j_reapchld: did not find pid==%d", pid));
		continue;
	  Found:
		_TRACE(5, ("j_reapchld: found pid==%d", pid));
		j->notify = 1;
		j->status = status;
#ifdef JOBS
		if (WIFSTOPPED(status))
			j->state = JSTOP;
		else
#endif
		if (WIFEXITED(status))
			j->state = JEXIT;
		else
		if (WIFSIGNALED(status))
			j->state = JSIGNAL;

		/* compute child's time */
		/* todo: what does a stopped job do? */
		j->utime = t1.tms_cutime - t0.tms_cutime;
		j->stime = t1.tms_cstime - t0.tms_cstime;
#if 0 && XN
		(void) fprintf(stderr, "past setting up j - pid: %ld\n", (long) getpid());
		(void) fflush(stderr);
#endif
		t0 = t1;
#ifdef JOBS
#ifdef _POSIX_SOURCE
		sigprocmask(SIG_BLOCK, &sm_sigchld, NULL);
#else
		sigblock(sm_sigchld);
#endif /* Xn 1992-07-29 */
#if 0 && XN
		(void) fprintf(stderr, "past sigprocmask #1 - pid: %ld\n", (long) getpid());
		(void) fflush(stderr);
#endif
#endif
		if (--sigchld_caught < 0) /* reduce the count */
		  sigchld_caught = 0;
#ifdef JOBS
		_TRACE(5, ("j_reapchld: j->proc==%d, j->com=='%s', j->state==0x%hx, j->status==0x%x, j->notify==%hd", j->proc, j->com, j->state, j->status, j->notify));
#ifdef _POSIX_SOURCE
		sigprocmask(SIG_SETMASK, &sm_old, NULL);
#else
		sigsetmask(sm_old); /* restore old mask */
#endif /* Xn 1992-07-29 */
#if 0 && XN
		(void) fprintf(stderr, "past sigprocmask #2 - pid: %ld\n", (long) getpid());
		(void) fflush(stderr);
#endif
#endif
#ifdef JOBS
	} while (flag[FMONITOR]);
#else
	} while (0);		/* only once if wait()ing */
#endif
#if 0 && XN
	(void) fprintf(stderr, "j_reapchld - exiting\n");
	(void) fflush(stderr);
#endif
}

int
#if __STDC__
j_reap(void)
#else
j_reap()
#endif /* Xn 1992-07-17 */
{
  if (sigchld_caught)
#if 0 && XN
  {
    (void) fprintf(stderr, "before j_reap/j_reapchld\n");
    (void) fflush(stderr);
#endif
    j_reapchld();
#if 0 && XN
    (void) fprintf(stderr, "after j_reap/j_reapchld\n");
    (void) fflush(stderr);
  }
#endif
#if defined(_SYSV) && !defined(JOBS)
	signal(SIGCHLD, j_sigchld);
	signal(SIGCHLD, SIG_DFL);
#endif
	return(0);
}

/* wait for child, interruptable */
int
#if __STDC__
waitfor(int job)
#else
waitfor(job)
	int job;
#endif /* Xn 1992-07-17 */
{
	register Proc *j;
#if 0 && XN
	int ret;
#endif

	if (job == 0 && j_current == 0)
		errorf("no current job\n");
	j = j_search((job == 0) ? j_current : job);
	if (j == NULL)
		errorf("no such job: %d\n", job);
	if (flag[FTALKING])
		j_print(j);
	if (e.interactive) {	/* flush stdout, shlout */
		fflush(shf[1]);
		fflush(shf[2]);
	}
#if 1 || !XN
	return j_waitj(j, 1);
#else
	(void) fprintf(stderr, "before waitfor/j_waitj\n");
	(void) fflush(stderr);
	ret = j_waitj(j, 1);
	(void) fprintf(stderr, "after waitfor/j_waitj\n");
	(void) fflush(stderr);
	return ret;
#endif
}

/* kill (built-in) a job */
void
#if __STDC__
j_kill(int job, int sig)
#else
j_kill(job, sig)
	int job;
	int sig;
#endif /* Xn 1992-07-17 */
{
	register Proc *j;

	j = j_search(job);
	if (j == NULL)
		errorf("cannot find job\n");
	if (j->pgrp == 0) {	/* !flag[FMONITOR] */
		if (kill(j->proc, sig) < 0) /* todo: all member of pipeline */
			errorf("kill: %s\n", strerror(errno));
#ifdef JOBS
	} else {
		if (sig == SIGTERM || sig == SIGHUP)
			(void) killpg(j->pgrp, SIGCONT);
		if (killpg(j->pgrp, sig) < 0)
			errorf("killpg: %s\n", strerror(errno));
#endif
	}
}

#ifdef JOBS

/* fg and bg built-ins */
int
#if __STDC__
j_resume(int job, int bg)
#else
j_resume(job, bg)
	int job;
	int bg;
#endif /* Xn 1992-07-17 */
{
	register Proc *j; 
#if 0 && XN
	int ret;
#endif
	
	j = j_search((job == 0) ? j_current : job);
	if (j == NULL)
		errorf("cannot find job\n", job);
	if (j->pgrp == 0)
		errorf("job not job-controlled\n");

	j->state = JRUN;
	j_print(j);
	flushshf(2);

	if (!bg)
#if 0 && XN
	{
#endif
  		tcsetpgrp(ttyfd, j->pgrp); /* attach shell to job */
#if 0 && XN
		(void) fprintf(stderr, "jobs.c/j_resume() - ttyfd: %d\n", ttyfd);
		(void) fflush(stderr);
	}
#endif
	if (killpg(j->pgrp, SIGCONT) < 0)
		errorf("cannot continue job %%%d\n", job);
	if (!bg)
#if 1 || !XN
		return j_waitj(j, 0);
#else
	{
		(void) fprintf(stderr, "before j_resume/j_waitj\n");
		(void) fflush(stderr);
		ret = j_waitj(j, 0);
		(void) fprintf(stderr, "after j_resume/j_waitj\n");
		(void) fflush(stderr);
		return ret;
	}
#endif
	return 0;
}

#endif

/* list jobs for jobs built-in */
void
#if __STDC__
j_jobs(void)
#else
j_jobs()
#endif /* Xn 1992-07-17 */
{
	register Proc *j; 

	for (j = procs; j != NULL; j = j->next)
		if (j->state != JFREE)
			j_print(j);
}

/* list jobs for top-level notification */
void
#if __STDC__
j_notify(void)
#else
j_notify()
#endif /* Xn 1992-07-17 */
{
	register Proc *j; 

	/*
	 * since reaping is no longer done in the signal handler
	 * we had better try here...
	 */
	if (sigchld_caught)
#if 0 && XN
	{
	  (void) fprintf(stderr, "before j_notify/j_reapchld\n");
	  (void) fflush(stderr);
#endif
	  j_reapchld();
#if 0 && XN
	  (void) fprintf(stderr, "after j_notify/j_reapchld\n");
	  (void) fflush(stderr);
	}
#endif
	
	for (j = procs; j != NULL; j = j->next) {
		if (j->state == JEXIT && !flag[FTALKING])
			j->notify = 0;
		if (j->state != JFREE && j->notify)
			j_print(j);
		if (j->state == JEXIT || j->state == JSIGNAL)
			j->state = JFREE;
		j->notify = 0;
	}
}

static void
#if __STDC__
j_print(register Proc *j)
#else
j_print(j)
	register Proc *j;
#endif /* Xn 1992-07-17 */
{
	char buf [64], *s = buf;

	switch (j->state) {
	  case JRUN:
		s = "Running";
		break;

#ifdef JOBS
	  case JSTOP:
		strcpy(buf, "Stopped ");
		s = strchr(sigtraps[WSTOPSIG(j->status)].mess, '(');
		if (s != NULL)
			strcat(buf, s);
		s = buf;
		break;
#endif

	  case JEXIT: {
		int rv;
		rv = WEXITSTATUS(j->status);
		sprintf(buf, "Done (%d)", rv);
		if (rv == 0)
			*strchr(buf, '(') = 0;
		j->state = JFREE;
		} break;

	  case JSIGNAL: {
		int sig = WTERMSIG(j->status);
		char *n = sigtraps[sig].mess;
		if (n != NULL)
			sprintf(buf, "%s", n);
		else
			sprintf(buf, "Signal %d", sig);
		if (WIFCORED(j->status))
			strcat(buf, " - core dumped");
		j->state = JFREE;
		} break;

	  default:
		s = "Hideous job state";
		j->state = JFREE;
		break;
	}
	shellf("%%%-2d%c %5d %-20s %s%s\n", j->job,
	       (j_current==j->job) ? '+' : (j_previous==j->job) ? '-' : ' ',
	       j->proc, s, j->com, (j->flags&XPIPEO) ? "|" : "");
}

/* convert % sequence to job number */
int
#if __STDC__
j_lookup(char *cp)
#else
j_lookup(cp)
	char *cp;
#endif /* Xn 1992-07-17 */
{
	register Proc *j;
	int len, job = 0;

	if (*cp == '%')		/* leading % is optional */
		cp++;
	switch (*cp) {
	  case '\0':
	  case '+':
		job = j_current;
		break;

	  case '-':
		job = j_previous;
		break;

	  case '0': case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7': case '8': case '9': 
		job = atoi(cp);
		break;

	  case '?':		/* %?string */
		for (j = procs; j != NULL; j = j->next)
			if (j->state != JFREE && strstr(j->com, cp+1) != NULL)
				job = j->job;
		break;

	  default:		/* %string */
		len = strlen(cp);
		for (j = procs; j != NULL; j = j->next)
			if (j->state != JFREE && strncmp(cp, j->com, len) == 0)
				job = j->job;
		break;
	}
	if (job == 0)
		errorf("%s: no such job\n", cp);
	return job;
}

/* are any stopped jobs ? */
#ifdef JOBS
int
#if __STDC__
j_stopped(void)
#else
j_stopped()
#endif /* Xn 1992-07-17 */
{
	register Proc *j; 

	for (j = procs; j != NULL; j = j->next)
		if (j->state == JSTOP)
			return 1;
	return 0;
}
#endif

/* create new job number */
static int
#if __STDC__
j_newjob(void)
#else
j_newjob()
#endif /* Xn 1992-07-17 */
{
	register Proc *j; 
	register int max = 0;
	
	j_lastjob ++;
	for (j = procs; j != NULL; j = j->next)
		if (j->state != JFREE && j->job)
			if (j->job > max)
				max = j->job;
	if (j_lastjob > max)
		j_lastjob = max + 1;
	return j_lastjob;
}

/* search for job by job number */
static Proc *
#if __STDC__
j_search(int job)
#else
j_search(job)
	int job;
#endif /* Xn 1992-07-17 */
{
	register Proc *j;

	for (j = procs; j != NULL; j = j->next)
		if (j->state != JFREE && job == j->job && !(j->flags&XPIPEO))
			return j;
	return NULL;
}

