/*
 * Command line editing - common code
 */

#include "config.h"
#if defined(EMACS) || defined(VI)

#ifndef lint
static char *RCSid = "$Id: edit.c,v 3.2 89/03/27 15:47:34 egisin Exp $";
static char *sccs_id = "@(#)edit.c	1.4 91/11/09 15:35:07 (sjg)";
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#include "sh.h"
#include "lex.h"
#include "tty.h"
#define EXTERN
#include "edit.h"
#undef EXTERN

#ifdef _CRAY2
#if 0
extern unsigned	sleep();
#else
extern unsigned	int sleep ARGS((unsigned int));
#endif /* Xn 1992-07-16 */
#endif

#if 0
int	x_do_init = 1;		/* set up tty modes */
int	x_cols = 80;		/* todo: $COLUMNS */
int	ed_erase = -1, ed_kill = -1, ed_werase = -1, ed_intr = -1, ed_quit = -1;
#endif

static int	x_noecho = 0;

/*
 * read an edited command line
 */
int
#if __STDC__
x_read(int fd, Void *buf, size_t len)
#else
x_read(fd, buf, len)
	int fd;			/* not used */
	Void *buf;
	size_t len;
#endif
{
  static int setup_done = 0;
#if 0
	char	c;
#endif /* Xn 1992-07-20 */
	int	i;

  if (setup_done != 42)
  {
    setup_done = 42;		/* these get done once only */
    x_do_init = 1;
    x_cols = 80;
    x_col = 0;
    ed_erase = -1, ed_kill = -1, ed_werase = -1, ed_intr = -1, ed_quit = -1;
    x_adj_ok = 1;
    x_adj_done = 0;
  }
	if (x_do_init)
		x_init();

	if (x_noecho)
		return(read(ttyfd, buf, len));

	(void)x_mode(TRUE);
#if 1
	i = fd;
#endif /* Xn 1992-07-20 */
#ifdef EMACS
	if (flag[FEMACS])
		i = x_emacs(buf, len);
	else
#endif
#ifdef VI
	if (flag[FVI])
		i = x_vi(buf, len);
	else
#endif
		i = -1;		/* internal error */
	(void) x_mode(FALSE);
	/* XXX -- doesn't get them all */
	if (i > 4 && strncmp(buf, "stty", 4) == 0)
		x_do_init = 1;
	if (i < 0 && errno == EINTR)
		trapsig(SIGINT);
	return i;
}

/* tty I/O */

int
#if __STDC__
x_getc(void)
#else
x_getc()
#endif /* Xn 1992-07-16 */
{
	char c;

	if (read(ttyfd, &c, 1) != 1)
		return -1;
	return c & 0x7F;
}

void
#if __STDC__
x_flush(void)
#else
x_flush()
#endif /* Xn 1992-07-16 */
{
	fflush(stdout);
}


/* NAME:
 *      x_adjust - redraw the line adjusting starting point etc.
 *
 * DESCRIPTION:
 *      This function is called when we have exceeded the bounds 
 *      of the edit window.  It increments x_adj_done so that 
 *      functions like x_ins and x_delete know that we have been 
 *      called and can skip the x_bs() stuff which has already 
 *      been done by x_redraw.
 *
 * RETURN VALUE:
 *      None
 */

void
#if __STDC__
x_adjust(void)
#else
x_adjust()
#endif /* Xn 1992-07-16 */
{
  x_adj_done++;			/* flag the fact that we were called. */
  /*
   * we had a promblem if the prompt length > x_cols / 2
   */
  if ((xbp = xcp - (x_displen / 2)) < xbuf)
    xbp = xbuf;
  xlp_valid = FALSE;
  x_redraw(x_cols);
  x_flush();
}

void
#if __STDC__
x_putc(int c)
#else
x_putc(c)
	int c;
#endif /* Xn 1992-07-16 */
{
  if (c == '\r' || c == '\n')
    x_col = 0;
  if (x_col < x_cols)
  {
    putc(c, stdout);
    switch(c)
    {
    case BEL:
      break;
    case '\r':
    case '\n':
    break;
    case '\b':
      x_col--;
      break;
    default:
      x_col++;
      break;
    }
  }
  if (x_adj_ok && (x_col < 0 || x_col >= (x_cols - 2)))
  {
    x_adjust();
  }
}

#ifdef DEBUG
int
#if __STDC__
x_debug_info(void)
#else
x_debug_info()
#endif /* Xn 1992-07-16 */
{
  x_flush();
  printf("\nksh debug:\n");
  printf("\tx_col == %d,\t\tx_cols == %d,\tx_displen == %d\n",
	 x_col, x_cols, x_displen);
  printf("\txcp == 0x%lx,\txep == 0x%lx\n", (long) xcp, (long) xep);
  printf("\txbp == 0x%lx,\txbuf == 0x%lx\n", (long) xbp, (long) xbuf);
  printf("\txlp == 0x%lx\n", (long) xlp);
  printf("\txlp == 0x%lx\n", (long) x_lastcp());
  printf("\n");
  x_redraw(-1);
  return 0;
}
#endif

void
#if __STDC__
x_puts(register char *s)
#else
x_puts(s)
	register char *s;
#endif /* Xn 1992-07-16 */
{
  register int	adj = x_adj_done;

  while (*s && adj == x_adj_done)
    x_putc(*s++);
}

#ifdef _BSD
static	struct sgttyb cb, cborig;
#ifdef TIOCGATC
static struct ttychars lchars, lcharsorig;
#else
static struct tchars tchars, tcharsorig;
static struct ltchars ltchars, ltcharsorig;
#endif
#else /* !_BSD */
#ifdef _POSIX_SOURCE
static	struct termios cb, cborig;
#else
static	struct termio cb, cborig;
#endif
#endif /* _BSD - Xn 1992-07-16 */

/* initialize editing mode */
void
#if __STDC__
x_init(void)
#else
x_init()
#endif /* Xn 1992-07-16 */
{
	x_do_init = 0;
#ifdef _BSD
	(void)ioctl(ttyfd, TIOCGETP, &cborig);
	if ((cborig.sg_flags & ECHO) == 0)
		x_noecho = 1;
	cb = cborig;
	ed_erase = cb.sg_erase;
	ed_kill = cb.sg_kill;
	cb.sg_flags &= ~ECHO;
	cb.sg_flags |= CBREAK;
#ifdef TIOCGATC
	(void)ioctl(ttyfd, TIOCGATC, &lcharsorig);
	lchars = lcharsorig;
	ed_werase = lchars.tc_werasc;
	lchars.tc_suspc = -1;
	lchars.tc_dsuspc = -1;
	lchars.tc_lnextc = -1;
	lchars.tc_statc = -1;
	lchars.tc_intrc = -1;
	lchars.tc_quitc = -1;
	lchars.tc_rprntc = -1;
#else
	(void)ioctl(ttyfd, TIOCGETC, &tcharsorig);
	(void)ioctl(ttyfd, TIOCGLTC, &ltcharsorig);
	tchars = tcharsorig;
	ltchars = ltcharsorig;
	ed_werase = ltchars.t_werasc;
	ltchars = ltcharsorig;
	ltchars.t_suspc = -1;
	ltchars.t_dsuspc = -1;
	ltchars.t_lnextc = -1;
	tchars.t_intrc = -1;
	tchars.t_quitc = -1;
	ltchars.t_rprntc = -1;
#endif
#else /* !_BSD */
#ifdef _POSIX_SOURCE
	(void)tcgetattr(ttyfd, &cborig);
#else
	(void)ioctl(ttyfd, TCGETA, &cborig);
#endif /* Xn 1992-07-16 */
	if ((cborig.c_lflag & ECHO) == 0)
		x_noecho = 1;
	cb = cborig;
#if 0
	ed_erase = cb.c_cc[VERASE]; /* TODO */
	ed_kill = cb.c_cc[VKILL]; /* TODO */
	ed_intr = cb.c_cc[VINTR];
	ed_quit = cb.c_cc[VQUIT];
#else
	ed_erase = (int)cb.c_cc[VERASE]; /* TODO */
	ed_kill = (int)cb.c_cc[VKILL]; /* TODO */
	ed_intr = (int)cb.c_cc[VINTR];
	ed_quit = (int)cb.c_cc[VQUIT];
#endif /* Xn 1992-07-20 */
#ifdef _CRAY2		/* brain-damaged terminal handler */
	cb.c_lflag &= ~(ICANON|ECHO);
	/* rely on print routine to map '\n' to CR,LF */
#else
	cb.c_iflag &= ~(INLCR|ICRNL);
#ifdef _BSD_SYSV	/* need to force CBREAK instead of RAW (need CRMOD on output) */
	cb.c_lflag &= ~(ICANON|ECHO);
#else
#ifdef SWTCH	/* need CBREAK to handle swtch char */
	cb.c_lflag &= ~(ICANON|ECHO);
	cb.c_lflag |= ISIG;
	cb.c_cc[VINTR] = 0377;
	cb.c_cc[VQUIT] = 0377;
#else
	cb.c_lflag &= ~(ISIG|ICANON|ECHO);
#endif
#endif
	cb.c_cc[VTIME] = 0;
	cb.c_cc[VMIN] = 1;
#endif	/* _CRAY2 */
#endif
#ifdef EMACS
	x_emacs_keys(ed_erase, ed_kill, ed_werase, ed_intr, ed_quit);
#endif
}

static	bool_t	x_cur_mode = FALSE;

/* set/clear tty cbreak mode */

bool_t
#if __STDC__
x_mode(bool_t onoff)
#else
x_mode(onoff)
	bool_t	onoff;
#endif /* Xn 1992-07-16 */
{
#ifdef _BSD
	bool_t	prev;

	if (x_cur_mode == onoff) return x_cur_mode;
	prev = x_cur_mode;
	x_cur_mode = onoff;
	if (onoff)  {
		(void)ioctl(ttyfd, TIOCSETN, &cb);
#ifdef TIOCGATC
		(void)ioctl(ttyfd, TIOCSATC, &lchars);
#else
		(void)ioctl(ttyfd, TIOCSETC, &tchars);
		(void)ioctl(ttyfd, TIOCSLTC, &ltchars);
#endif
	}
	else {
		(void)ioctl(ttyfd, TIOCSETN, &cborig);
#ifdef TIOCGATC
		(void)ioctl(ttyfd, TIOCSATC, &lcharsorig);
#else
		(void)ioctl(ttyfd, TIOCSETC, &tcharsorig);
		(void)ioctl(ttyfd, TIOCSLTC, &ltcharsorig);
#endif
	}
	return prev;
#else	/* !_BSD */
	bool_t	prev;

	if (x_cur_mode == onoff) return x_cur_mode;
	prev = x_cur_mode;
	x_cur_mode = onoff;

	if (onoff)  {
#ifndef TCSETAW				/* e.g. Cray-2 */
		/* first wait for output to drain */
#ifdef TCSBRK
		(void)ioctl(ttyfd, TCSBRK, 1);
#else	/* the following kludge is minimally intrusive, but sometimes fails */
		(void)sleep((unsigned)1);	/* fake it */
#endif
#endif
#ifdef _POSIX_SOURCE
		(void)tcsetattr(ttyfd, TCSADRAIN, &cb);
#else
#if defined(_BSD_SYSV) || !defined(TCSETAW)
/* _BSD_SYSV needs to force TIOCSETN instead of TIOCSETP (preserve type-ahead) */
		(void)ioctl(ttyfd, TCSETA, &cb);
#else
		(void)ioctl(ttyfd, TCSETAW, &cb);
#endif
#endif /* Xn 1992-07-16 */
	}
	else {
#ifndef TCSETAW				/* e.g. Cray-2 */
		/* first wait for output to drain */
#ifdef TCSBRK
		(void)ioctl(ttyfd, TCSBRK, 1);
#else
/* doesn't seem to be necessary when leaving xmode */
/*		(void)sleep((unsigned)1);	/* fake it */
#endif
#endif
#ifdef _POSIX_SOURCE
		(void)tcsetattr(ttyfd, TCSADRAIN, &cborig);
#else
#if defined(_BSD_SYSV) || !defined(TCSETAW)
/* _BSD_SYSV needs to force TIOCSETN instead of TIOCSETP (preserve type-ahead) */
		(void)ioctl(ttyfd, TCSETA, &cborig);
#else
		(void)ioctl(ttyfd, TCSETAW, &cborig);
#endif
#endif /* Xn 1992-07-16 */
	}
	return prev;
#endif	/* _BSD */
}


/* NAME:
 *      promptlen - calculate the length of PS1 etc.
 *
 * DESCRIPTION:
 *      This function is based on a fix from guy@demon.co.uk
 *      It fixes a bug in that if PS1 contains '!', the length 
 *      given by strlen() is probably wrong.
 *
 * RETURN VALUE:
 *      length
 */
 
int
#if __STDC__
promptlen(register char *cp)
#else
promptlen(cp)
  register char *cp;
#endif /* Xn 1992-07-16 */
{
  register int count = 0;

  while (*cp)
  {
    if ( *cp++ != '!' )
      count++;
    else
      if ( *cp == '!' )
      {
	cp++;
	count++;
      }
      else
      {
	register int i = source->line;

	do
	{
	  count ++;
	}
	while( ( i /= 10 ) > 0 );
      }
  }
  return count;
}

#endif
