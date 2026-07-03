/*
 *  Emacs-like command line editing and history
 *
 *  created by Ron Natalie at BRL
 *  modified by Doug Kingston, Doug Gwyn, and Lou Salkind
 *  adapted to PD ksh by Eric Gisin
 */

#include "config.h"
#ifdef EMACS

#ifndef lint
static char *RCSid = "$Id: emacs.c,v 3.2 89/03/27 15:49:17 egisin Exp $";
static char *sccs_id = "@(#)emacs.c	1.5 91/11/09 15:35:13 (sjg)";
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#if WIN_NT
#include <limits.h>
#endif /* Xn 1993-04-02 */
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include "sh.h"
#include "lex.h"
#include "tree.h"		/* DOTILDE */
#include "table.h"
#include "expand.h"
#include "edit.h"

#define PUSH_DELETE 1			/* push all deletes of >1 char */

static	Area	aedit;
#define	AEDIT	&aedit		/* area for kill ring and macro defns */

#undef CTRL			/* _BSD brain damage */
#define	CTRL(x)		((x) == '?' ? 0x7F : (x) & 0x1F)	/* ASCII */
#define	UNCTRL(x)	((x) == 0x7F ? '?' : (x) | 0x40)	/* ASCII */

#ifndef S_ISDIR
#define S_ISDIR(mode)	(((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode)	(((mode) & S_IFMT) == S_IFREG)
#endif

/* values returned by keyboard functions */
#define	KSTD	0
#define	KPREF	1		/* ^[, ^X */
#define	KEOL	2		/* ^M, ^J */
#define	KINTR	3		/* ^G, ^C */
#define	KNULL	4
	
struct	x_ftab  {
	int	(*xf_func)();
	char	*xf_name;
	char	xf_db_tab;
	char	xf_db_char;
	short	xf_flags;
};

#define	XF_ALLOC	2
#define	XF_NOBIND	4

#define	iscfs(c)	(c == ' ' || c == '\t')	/* Separator for completion */
#define	ismfs(c)	(!(isalnum(c)|| c == '$'))  /* Separator for motion */
#define	BEL		0x07
#define	CMASK		0x7F	/* 7-bit ASCII character mask */

static	int	x_prefix1 = CTRL('['), x_prefix2 = CTRL('X');
static	char   **x_histp;	/* history position */
static	char   **x_nextcmdp;	/* for newline-and-next */
static	char	*xmp;		/* mark pointer */
static	int    (*x_last_command)();
/*static	struct	x_ftab *x_tab[3][128];*/
static	struct	x_ftab Const *(*x_tab)[128] = NULL; /* key definition */
static	char    *(*x_atab)[128] = NULL; /* macro definitions */
#define	KILLSIZE	20
static	char    *killstack[KILLSIZE];
static	int	killsp, killtp;
static	int	x_curprefix;
static	char    *macroptr;
static	int	x_maxlen;	/* to determine column width */

#if 0
static	void	x_goto(), x_bs(), x_delete(), x_ins(), x_mapin();
static	int	x_fword(), x_bword(), x_size(), x_size_str();
static	void	x_zotc(), x_zots(), x_push(), x_load_hist();
static	void	compl_command(), compl_dec(), compl_file();
static	int	x_insert(), x_ins_string(), x_del_back();
static	int	x_del_char(), x_del_bword(), x_mv_bword(), x_mv_fword();
static	int	x_del_fword(), x_mv_back(), x_mv_forw(), x_search_char();
static	int	x_newline(), x_end_of_text(), x_abort(), x_error();
static	int	x_beg_hist(), x_end_hist(), x_prev_com(), x_next_com();
static	int	x_search_hist(), x_del_line(), x_mv_end(), x_mv_begin();
static	int	x_draw_line(), x_transpose(), x_meta1(), x_meta2();
static	int	x_kill(), x_yank(), x_meta_yank(), x_literal();
static	int	x_stuffreset(), x_stuff(), x_complete(), x_enumerate();
static	int	x_set_mark(), x_kill_region(), x_xchg_point_mark();
static	int	x_nl_next_com(), x_eot_del();
static	int	x_copy_arg();
static	int	x_noop();
#ifdef SILLY
static	int	x_game_of_life();
#endif
static	int	x_comp_file(), x_comp_comm();
static	int	x_list_file(), x_list_comm();
static	int	strmatch();
static int	x_prev_histword(), x_set_arg(), x_fold_case();
#else
extern	void	pprompt ARGS((register char *)); /* in lex.c */
static	void	x_goto ARGS((register char *)), x_bs ARGS((int));
static	void	x_delete ARGS((int)), x_ins ARGS((char *));
static	void	x_mapin ARGS((char *));
static	int	x_fword ARGS((void)), x_bword ARGS((void)), x_size ARGS((int));
static	int	x_size_str ARGS((register char *));
static	void	x_zotc ARGS((int)), x_zots ARGS((register char *));
static	void	x_push ARGS((int)), x_load_hist ARGS((register char **));
static	void	compl_command ARGS((int)), compl_dec ARGS((int));
static	void	compl_file ARGS((int));
static	int	x_insert ARGS((int)), x_ins_string ARGS((int));
static	int	x_del_back ARGS((int));
static	int	x_del_char ARGS((int)), x_del_bword ARGS((int));
static	int	x_mv_bword ARGS((int)), x_mv_fword ARGS((int));
static	int	x_del_fword ARGS((int)), x_mv_back ARGS((int));
static	int	x_mv_forw ARGS((int)), x_search_char ARGS((int));
static	int	x_newline ARGS((int)), x_end_of_text ARGS((int));
static	int	x_abort ARGS((int)), x_error ARGS((int));
static	int	x_beg_hist ARGS((int)), x_end_hist ARGS((int));
static	int	x_prev_com ARGS((int)), x_next_com ARGS((int));
static	int	x_search_hist ARGS((int)), x_del_line ARGS((int));
static	int	x_mv_end ARGS((int)), x_mv_begin ARGS((int));
static	int	x_draw_line ARGS((int)), x_transpose ARGS((int));
static	int	x_meta1 ARGS((int)), x_meta2 ARGS((int));
static	int	x_kill ARGS((int)), x_yank ARGS((int));
static	int	x_meta_yank ARGS((int)), x_literal ARGS((int));
static	int	x_stuffreset ARGS((int)), x_stuff ARGS((int));
static	int	x_complete ARGS((int)), x_enumerate ARGS((int));
static	int	x_set_mark ARGS((int)), x_kill_region ARGS((int));
static	int	x_xchg_point_mark ARGS((int));
static	int	x_nl_next_com ARGS((int)), x_eot_del ARGS((int));
static	int	x_copy_arg ARGS((int));
static	int	x_noop ARGS((int));
#ifdef SILLY
static	int	x_game_of_life ARGS((int));
#endif
static	int	x_comp_file ARGS((int)), x_comp_comm ARGS((int));
static	int	x_list_file ARGS((int)), x_list_comm ARGS((int));
static	int	strmatch ARGS((register char *, register char *));
static	int	x_prev_histword ARGS((void)), x_set_arg ARGS((int));
static	int	 x_fold_case ARGS((int));
#endif /* Xn 1992-07-17 */


static	struct x_ftab Const x_ftab[] = {
 	{x_insert,	"auto-insert",		0,	 0,	0 },
	{x_error,	"error",		0,	 0,	0 },
 	{x_ins_string,	"macro-string",		0,	 0,	XF_NOBIND|XF_ALLOC},
	{x_del_back,	"delete-char-backward",	0, CTRL('H'),	0 },
	{x_eot_del,	"eot-or-delete",	0, CTRL('D'),	0 },
	{x_del_bword,	"delete-word-backward",	1, CTRL('H'),	0 },
	{x_mv_bword,	"backward-word", 	1,	'b',	0 },
	{x_del_line,	"kill-line",		0, 	0,	0 },
	{x_abort,	"abort",		0,	0,	0 },
	{x_noop,	"no-op",		0,	0,	0 },
/* Do not move the above! */
	{x_mv_fword,	"forward-word",		1,	'f',	0 },
	{x_del_fword,	"delete-word-forward", 	1,	'd',	0 },
	{x_mv_back,	"backward-char",	0, CTRL('B'),	0 },
	{x_mv_forw,	"forward-char",		0, CTRL('F'),	0 },
	{x_search_char,	"search-character",	0, CTRL(']'),	0 },
	{x_newline,	"newline",		0, CTRL('M'),	0 },
	{x_newline,	"newline",		0, CTRL('J'),	0 },
	{x_end_of_text,	"eot",			0, CTRL('_'),	0 },
	{x_abort,	"abort",		0, CTRL('G'),	0 },
	{x_prev_com,	"up-history",		0, CTRL('P'),	0},
	{x_next_com,	"down-history",		0, CTRL('N'),	0},
	{x_search_hist,	"search-history",	0, CTRL('R'),	0},
	{x_beg_hist,	"beginning-of-history",	1,	'<',	0},
	{x_end_hist,	"end-of-history",	1,	'>',	0},
	{x_mv_end,	"end-of-line",		0, CTRL('E'),	0 },
	{x_mv_begin,	"beginning-of-line",	0, CTRL('A'),	0 },
	{x_draw_line,	"redraw",		0, CTRL('L'),	0 },
	{x_meta1,	"prefix-1",		0, CTRL('['),	0 },
	{x_meta2,	"prefix-2",		0, CTRL('X'),	0 },
	{x_kill,	"kill-to-eol",		0, CTRL('K'),	0 },
	{x_yank,	"yank",			0, CTRL('Y'),	0 },
	{x_meta_yank,	"yank-pop", 		1,	'y',	0 },
	{x_literal,	"quote",		0, CTRL('^'),	0 },
	{x_stuffreset, 	"stuff-reset",		0,	 0,	0 },
#if defined(BRL) && defined(TIOCSTI)
	{x_stuff, 	"stuff",		0, CTRL('T'),	0 },
	{x_transpose,	"transpose-chars",	0,	 0,	0 },
#else
	{x_stuff, 	"stuff",		0,	 0,	0 },
	{x_transpose,	"transpose-chars",	0, CTRL('T'),	0 },
#endif
	{x_complete,	"complete",		1, CTRL('['),	0 },
	{x_enumerate,	"list",			1,	'?',	0 },
	{x_comp_file,	"complete-file",	1, CTRL('X'),	0 },
	{x_comp_comm,	"complete-command",	2, CTRL('['),	0 },
	{x_list_file,	"list-file",		0,	 0,	0 },
	{x_list_comm,	"list-command",		2,	'?',	0 },
	{x_nl_next_com,	"newline-and-next",	0, CTRL('O'),	0 },
	{x_set_mark,	"set-mark-command",	1,	' ',	0 },
	{x_kill_region,	"kill-region",		0, CTRL('W'),	0 },
	{x_xchg_point_mark, "exchange-point-and-mark", 2, CTRL('X'), 0 },
#if 0
	{x_copy_arg,	"copy-last-arg",	1,	'_',	0},
#endif
#ifdef SILLY
	{x_game_of_life, "play-game-of-life",	0,	0,	0 },
#endif 
#ifdef DEBUG
        {x_debug_info,	"debug-info",		1, CTRL('H'),	0 },
#endif
	{x_prev_histword, "prev-hist-word", 	1,	'.',	0 },
	{x_prev_histword, "prev-hist-word", 	1,	'_',	0 },
        {x_set_arg,	"",			1,	'0',	0 },
        {x_set_arg,	"",			1,	'1',	0 },
        {x_set_arg,	"",			1,	'2',	0 },
        {x_set_arg,	"",			1,	'3',	0 },
        {x_set_arg,	"",			1,	'4',	0 },
        {x_set_arg,	"",			1,	'5',	0 },
        {x_set_arg,	"",			1,	'6',	0 },
        {x_set_arg,	"",			1,	'7',	0 },
        {x_set_arg,	"",			1,	'8',	0 },
        {x_set_arg,	"",			1,	'9',	0 },
        {x_fold_case,	"upcase-word",		1,	'U',	0 },
        {x_fold_case,	"downcase-word",	1,	'L',	0 },
        {x_fold_case,	"capitalize-word",	1,	'C',	0 },
        {x_fold_case,	"upcase-word",		1,	'u',	0 },
        {x_fold_case,	"downcase-word",	1,	'l',	0 },
        {x_fold_case,	"capitalize-word",	1,	'c',	0 },
	{ 0 }
};

#define	xft_insert &x_ftab[0]
#define	xft_error &x_ftab[1]
#define	xft_ins_string &x_ftab[2]
#define	xft_erase &x_ftab[3]
#define	xft_kill &x_ftab[7]
#define	xft_werase &x_ftab[5]
#define xft_intr &x_ftab[8]
#define	xft_quit &x_ftab[9]

int
#if __STDC__
x_emacs(char *buf, size_t len)
#else
x_emacs(buf, len)
	char *buf;
	size_t len;
#endif /* Xn 1992-07-16 */
{
	char	c;
	int	i;
	int   (*func)();
	extern	x_insert();

	xbp = xbuf = buf; xend = buf + len;
	xlp = xcp = xep = buf;
	*xcp = 0;
	xlp_valid = TRUE;
	xmp = NULL;
	x_curprefix = 0;
	macroptr = null;
	x_histp = histptr + 1;

	if (x_nextcmdp != NULL) {
		x_load_hist(x_nextcmdp);
		x_nextcmdp = NULL;
	}

	/* <sjg@sun0> this may not be correct */
	if ((i = atoi(strval(global("COLUMNS")))) > 0)
	  x_cols = i;
	else
	  x_cols = 80;
	x_col = promptlen(prompt);
	x_adj_ok = 1;
	x_displen = x_cols - 2 - x_col;
	x_adj_done = 0;

#if 0
	while (1)  {
#else
	for (;;)  {
#endif /* Xn 1992-07-20 */
		x_flush();
		if (*macroptr)  {
			c = *macroptr++;
			if (*macroptr == 0)
				macroptr = null;
		}
		else {
#if 0
			if ((c = x_getc()) < 0)
				return i;
#else
			int cc;

			if ((cc = x_getc()) < 0)
				return i;
			else
				c = (char)cc;
#endif /* Xn 1992-07-20 */
		}

		if (x_curprefix == -1)
			func = x_insert;
		else
			func = x_tab[x_curprefix][c&CMASK]->xf_func;
		if (func == NULL)
			func = x_error;
		i = c | (x_curprefix << 8);
		x_curprefix = 0;
		switch (i = (*func)(i))  {
		  case KSTD:
			x_last_command = func;
		  case KPREF:
		  case KNULL:
			break;
		  case KEOL:
			i = xep - xbuf;
			x_last_command = 0;
			return i;
		  case KINTR:	/* special case for interrupt */
			errno = EINTR;
			return -1;
		}
	}
}

static int
#if __STDC__
x_insert(int c)
#else
x_insert(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	char	str[2];

	/*
	 *  Should allow tab and control chars.
	 */
	if (c == 0)  {
		x_putc(BEL);
		return KSTD;
	}
#if 0
	str[0] = c;
#else
	str[0] = (char)c;
#endif /* Xn 1992-07-20 */
	str[1] = 0;
	x_ins(str);
	return KSTD;
}

static int
#if __STDC__
x_ins_string(int c)
#else
x_ins_string(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	if (*macroptr)   {
		x_putc(BEL);
		return KSTD;
	}
	macroptr = x_atab[c>>8][c & CMASK];
	return KSTD;
}

static void
#if __STDC__
x_ins(char *cp)
#else
x_ins(cp)
	char	*cp;
#endif /* Xn 1992-07-16 */
{
#if 0
	int	count, i;
#else
	int	count;
#endif /* Xn 1992-07-20 */
	register int	adj = x_adj_done;

	count = strlen(cp);
	if (xep+count >= xend) {
		x_putc(BEL);
		return;
	}

	if (xcp != xep)
		memmove(xcp+count, xcp, xep - xcp + 1);
	else
		xcp[count] = 0;
	memmove(xcp, cp, count);
	/*
	 * x_zots() may result in a call to x_adjust()
	 * we want xcp to reflect the new position.
	 */
	cp = xcp;
	xcp += count;
	xep += count;
	xlp_valid = FALSE;
	x_lastcp();
	x_adj_ok = (xcp >= xlp);
	x_zots(cp);
	if (adj == x_adj_done)	/* has x_adjust() been called? */
	{
	  /* no */
	  for (cp = xlp; cp > xcp; )
	    x_bs(*--cp);
	}

	x_adj_ok = 1;
	return;
}

static int
#if __STDC__
x_del_back(int c)
#else
x_del_back(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (xcp == xbuf)  {
		x_putc(BEL);
		return KSTD;
	}
	x_goto(xcp - 1);
	x_delete(1);
	return KSTD;
}

static int
#if __STDC__
x_del_char(int c)
#else
x_del_char(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (xcp == xep)  {
		x_putc(BEL);
		return KSTD;
	}
	x_delete(1);
	return KSTD;
}

static void
#if __STDC__
x_delete(int nc)
#else
x_delete(nc)
	int nc;
#endif /* Xn 1992-07-16 */
{
	int	i,j;
	char	*cp;
	
	if (nc == 0)
		return;
	if (xmp != NULL) {
		if (xcp + nc > xmp)
			xmp = xcp;
		else if (xmp > xcp)
			xmp -= nc;
	}
#ifdef PUSH_DELETE
	/*
	 * This lets us yank a word we have deleted.
	 */
	if (nc > 1)
	  x_push(nc);
#endif
	xep -= nc;
	cp = xcp;
	j = 0;
	i = nc;
	while (i--)  {
		j += x_size(*cp++);
	}
	memmove(xcp, xcp+nc, xep - xcp + 1);	/* Copies the null */
	x_adj_ok = 0;			/* don't redraw */
	x_zots(xcp);
	/*
	 * if we are already filling the line,
	 * there is no need to ' ','\b'.
	 * But if we must, make sure we do the minimum.
	 */
	if ((i = x_cols - 2 - x_col) > 0)
	{
	  j = (j < i) ? j : i;
	  i = j;
	  while (i--)
	    x_putc(' ');
	  i = j;
	  while (i--)
	    x_putc('\b');
	}
	/*x_goto(xcp);*/
	x_adj_ok = 1;
	xlp_valid = FALSE;
	for (cp = x_lastcp(); cp > xcp; )
	  x_bs(*--cp);

	return;	
}

static int
#if __STDC__
x_del_bword(int c)
#else
x_del_bword(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_delete(x_bword());
	return KSTD;
}

static int
#if __STDC__
x_mv_bword(int c)
#else
x_mv_bword(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	(void)x_bword();
	return KSTD;
}

static int
#if __STDC__
x_mv_fword(int c)
#else
x_mv_fword(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_goto(xcp + x_fword());
	return KSTD;
}

static int
#if __STDC__
x_del_fword(int c)
#else
x_del_fword(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_delete(x_fword());
	return KSTD;
}

static int
#if __STDC__
x_bword(void)
#else
x_bword()
#endif /* Xn 1992-07-16 */
{
	int	nc = 0;
	register char *cp = xcp;

	if (cp == xbuf)  {
		x_putc(BEL);
		return 0;
	}
	if (x_last_command != x_set_arg)
	  x_arg = 1;
	while (x_arg--)
	{
	  while (cp != xbuf && ismfs(cp[-1]))
	  {
	    cp--;
	    nc++;
	  }
	  while (cp != xbuf && !ismfs(cp[-1]))
	  {
	    cp--;
	    nc++;
	  }
	}
	x_goto(cp);
	return nc;
}

static int
#if __STDC__
x_fword(void)
#else
x_fword()
#endif /* Xn 1992-07-16 */
{
	int	nc = 0;
	register char	*cp = xcp;

	if (cp == xep)  {
		x_putc(BEL);
		return 0;
	}
	if (x_last_command != x_set_arg)
	  x_arg = 1;
	while (x_arg--)
	{
	  while (cp != xep && !ismfs(*cp))
	  {
	    cp++;
	    nc++;
	  }
	  while (cp != xep && ismfs(*cp))
	  {
	    cp++;
	    nc++;
	  }
	}
	return nc;
}

static void
#if __STDC__
x_goto(register char *cp)
#else
x_goto(cp)
	register char *cp;
#endif /* Xn 1992-07-16 */
{
  if (cp < xbp || cp >= (xbp + x_displen))
  {
    /* we are heading off screen */
    xcp = cp;
    x_adjust();
  }
  else
  {
    if (cp < xcp)		/* move back */
    {
      while (cp < xcp)
	x_bs(*--xcp);
    }
    else
    {
      if (cp > xcp) 		/* move forward */
      {
	while (cp > xcp)
	  x_zotc(*xcp++);
      }
    }
  }
}

static void
#if __STDC__
x_bs(int c)
#else
x_bs(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	register i;
	i = x_size(c);
	while (i--)
		x_putc('\b');
}

static int
#if __STDC__
x_size_str(register char *cp)
#else
x_size_str(cp)
	register char *cp;
#endif /* Xn 1992-07-16 */
{
	register size = 0;
	while (*cp)
		size += x_size(*cp++);
	return size;
}

static int
#if __STDC__
x_size(int c)
#else
x_size(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	if (c=='\t')
		return 4;	/* Kludge, tabs are always four spaces. */
	if (c < ' ' || c == 0x7F) /* ASCII control char */
		return 2;
	return 1;
}

static void
#if __STDC__
x_zots(register char *str)
#else
x_zots(str)
	register char *str;
#endif /* Xn 1992-07-16 */
{
  register int	adj = x_adj_done;

  x_lastcp();
  while (*str && str < xlp && adj == x_adj_done)
    x_zotc(*str++);
}

static void
#if __STDC__
x_zotc(int c)
#else
x_zotc(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	if (c == '\t')  {
		/*  Kludge, tabs are always four spaces.  */
		x_puts("    ");
	} else if (c < ' ' || c == 0x7F)  { /* ASCII */
		x_putc('^');
		x_putc(UNCTRL(c));
	} else
		x_putc(c);
}

static int
#if __STDC__
x_mv_back(int c)
#else
x_mv_back(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (xcp == xbuf)  {
		x_putc(BEL);
		return KSTD;
	}
	x_goto(xcp-1);
	return KSTD;
}

static int
#if __STDC__
x_mv_forw(int c)
#else
x_mv_forw(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (xcp == xep)  {
		x_putc(BEL);
		return KSTD;
	}
	x_goto(xcp+1);
	return KSTD;
}

static int
#if __STDC__
x_search_char(int c)
#else
x_search_char(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	char *cp;

	*xep = '\0';
	if ((c = x_getc()) < 0 ||
	    /* we search forward, I don't know what Korn does */
	    (cp = (xcp == xep) ? NULL : strchr(xcp+1, c)) == NULL &&
	    (cp = strchr(xbuf, c)) == NULL) {
		x_putc(BEL);
		return KSTD;
	}
	x_goto(cp);
	return KSTD;
}

static int
#if __STDC__
x_newline(int c)
#else
x_newline(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_putc('\n');
	x_flush();
	*xep++ = '\n';
	return KEOL;
}

static int
#if __STDC__
x_end_of_text(int c)
#else
x_end_of_text(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
#if 0
	x_store_hist();
#endif
	return KEOL;
}

static int
#if __STDC__
x_beg_hist(int c)
#else
x_beg_hist(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_load_hist(history);
	return KSTD;
}

static int
#if __STDC__
x_end_hist(int c)
#else
x_end_hist(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_load_hist(histptr);
	return KSTD;
}

static int
#if __STDC__
x_prev_com(int c)
#else
x_prev_com(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_load_hist(x_histp-1);
	return KSTD;
}

static int
#if __STDC__
x_next_com(int c)
#else
x_next_com(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_load_hist(x_histp+1);
	return KSTD;
}

static void
#if __STDC__
x_load_hist(register char **hp)
#else
x_load_hist(hp)
	register char **hp;
#endif /* Xn 1992-07-16 */
{
	int	oldsize;

	if (hp < history || hp > histptr) {
		x_putc(BEL);
		return;
	}
	x_histp = hp;
	oldsize = x_size_str(xbuf);
	(void)strcpy(xbuf, *hp);
	xbp = xbuf;
	xep = xcp = xbuf + strlen(*hp);
	xlp_valid = FALSE;
	if (xep > x_lastcp())
	  x_goto(xep);
	else
	  x_redraw(oldsize);
}

static int
#if __STDC__
x_nl_next_com(int c)
#else
x_nl_next_com(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	x_nextcmdp = x_histp + 1;
	return (x_newline(c));
}

static int
#if __STDC__
x_eot_del(int c)
#else
x_eot_del(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	if (xep == xbuf)
		return (x_end_of_text(c));
	else
		return (x_del_char(c));
}

#if 0
static int x_search(), x_match();
#else
static int x_search ARGS((char *, int)), x_match ARGS((char *, char *));
#endif /* Xn 1992-07-16 */

/* reverse incremental history search */
static int
#if __STDC__
x_search_hist(int c)
#else
x_search_hist(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	int offset = -1;	/* offset of match in xbuf, else -1 */
	char pat [256+1];	/* pattern buffer */
	register char *p = pat;
	int (*func)();

	*p = 0;
#if 0
	while (1) {
#else
	for (;;) {
#endif /* Xn 1992-07-20 */
		if (offset < 0) {
			x_puts("\nI-search: ");
			x_zots(pat);
		}
		x_flush();
		if ((c = x_getc()) < 0)
			return KSTD;
		func = x_tab[0][c&CMASK]->xf_func;
		if (c == CTRL('['))
			break;
		else if (func == x_search_hist)
			offset = x_search(pat, offset);
		else if (func == x_del_back)
			continue;	/* todo */
		else if (func == x_insert) {
			/* add char to pattern */
#if 0
			*p++ = c, *p = 0;
#else
			*p++ = (char)c;
			*p = 0;
#endif /* Xn 1992-07-20 */
			if (offset >= 0) {
				/* already have partial match */
				offset = x_match(xbuf, pat);
				if (offset >= 0) {
					x_goto(xbuf + offset + (p - pat) - (*pat == '^'));
					continue;
				}
			}
			offset = x_search(pat, offset);
		} else { /* other command */
			static char push[2];
#if 0
			push[0] = c;
#else
			push[0] = (char)c;
#endif /* Xn 1992-07-20 */
			macroptr = push; /* push command */
			break;
		}
	}
	if (offset < 0)
		x_redraw(-1);
	return KSTD;
}

/* search backward from current line */
static int
#if __STDC__
x_search(char *pat, int offset)
#else
x_search(pat, offset)
	char *pat;
	int offset;
#endif /* Xn 1992-07-16 */
{
	register char **hp;
	int i;

	for (hp = x_histp; --hp >= history; ) {
		i = x_match(*hp, pat);
		if (i >= 0) {
			if (offset < 0)
				x_putc('\n');
			x_load_hist(hp);
			x_goto(xbuf + i + strlen(pat) - (*pat == '^'));
			return i;
		}
	}
	x_putc(BEL);
	x_histp = histptr;
	return -1;
}

/* return position of first match of pattern in string, else -1 */
static int
#if __STDC__
x_match(char *str, char *pat)
#else
x_match(str, pat)
	char *str, *pat;
#endif /* Xn 1992-07-16 */
{
	if (*pat == '^') {
		return (strncmp(str, pat+1, strlen(pat+1)) == 0) ? 0 : -1;
	} else {
		char *q = strstr(str, pat);
		return (q == NULL) ? -1 : q - str;
	}
}

static int
#if __STDC__
x_del_line(int c)
#else
x_del_line(c)
	int c;
#endif /* Xn 1992-07-16 */
{
	int	i, j;

#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	*xep = 0;
	i = xep- xbuf;
	j = x_size_str(xbuf);
	xcp = xbuf;
	x_push(i);
	xlp = xbp = xep = xbuf;
	xlp_valid = TRUE;
	*xcp = 0;
	xmp = NULL;
	x_redraw(j);
	return KSTD;
}

static int
#if __STDC__
x_mv_end(int c)
#else
x_mv_end(c)
	int c;
#endif /* Xn 1992-07-16 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_goto(xep);
	return KSTD;
}

static int
#if __STDC__
x_mv_begin(int c)
#else
x_mv_begin(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_goto(xbuf);
	return KSTD;
}

static int
#if __STDC__
x_draw_line(int c)
#else
x_draw_line(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_redraw(-1);
	return KSTD;

}

void
#if __STDC__
x_redraw(int limit)
#else
x_redraw(limit)
	int limit;
#endif /* Xn 1992-07-17 */
{
	int	i, j;
	char	*cp;
	
	x_adj_ok = 0;
	if (limit == -1)
		x_putc('\n');
	else 
		x_putc('\r');
	x_flush();
	if (xbp == xbuf)
	{
	  pprompt(prompt);
	  x_col = promptlen(prompt);
	}
	x_displen = x_cols - 2 - x_col;
	xlp_valid = FALSE;
	cp = x_lastcp();
	x_zots(xbp);
	if (xbp != xbuf || xep > xlp)
	  limit = x_cols;
	if (limit >= 0)
	{
	  if (xep > xlp)
	    i = 0;			/* we fill the line */
	  else
	    i = limit - (xlp - xbp);

	  for (j = 0; j < i && x_col < (x_cols - 2); j++)
	    x_putc(' ');
	  i = ' ';
	  if (xep > xlp)		/* more off screen */
	  {
	    if (xbp > xbuf)
	      i = '*';
	    else
	      i = '>';
	  }
	  else
	    if (xbp > xbuf)
	      i = '<';
	  x_putc(i);
	  j++;
	  while (j--)
	    x_putc('\b');
	}
	for (cp = xlp; cp > xcp; )
	  x_bs(*--cp);
	x_adj_ok = 1;
	_D_(x_flush();)
	return;
}

static int
#if __STDC__
x_transpose(int c)
#else
x_transpose(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	char	tmp;
#if 1

	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (xcp == xbuf) {
		x_putc(BEL);
		return KSTD;
	} else if (xcp == xep) {
		if (xcp - xbuf == 1) {
			x_putc(BEL);
			return KSTD;
		}
		x_bs(xcp[-1]);
		x_bs(xcp[-2]);
		x_zotc(xcp[-1]);
		x_zotc(xcp[-2]);
		tmp = xcp[-1];
		xcp[-1] = xcp[-2];
		xcp[-2] = tmp;
	} else {
		x_bs(xcp[-1]);
		x_zotc(xcp[0]);
		x_zotc(xcp[-1]);
		tmp = xcp[-1];
		xcp[-1] = xcp[0];
		xcp[0] = tmp;
		x_bs(xcp[0]);
	}
	return KSTD;
}

static int
#if __STDC__
x_literal(int c)
#else
x_literal(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_curprefix = -1;
	return KSTD;
}

static int
#if __STDC__
x_meta1(int c)
#else
x_meta1(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_curprefix = 1;
	return KPREF;
}

static int
#if __STDC__
x_meta2(int c)
#else
x_meta2(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_curprefix = 2;
	return KPREF;
}

static int
#if __STDC__
x_kill(int c)
#else
x_kill(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	int	i;

#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	i = xep - xcp;
	xlp = xcp;
	xlp_valid = TRUE;
	x_push(i);
	x_delete(i);
	return KSTD;
}

static void
#if __STDC__
x_push(int nchars)
#else
x_push(nchars)
	int nchars;
#endif /* Xn 1992-07-17 */
{
	char	*cp;
	cp = alloc((size_t)(nchars+1), AEDIT);
	memmove(cp, xcp, nchars);
	cp[nchars] = 0;
	if (killstack[killsp])
		afree((Void *)killstack[killsp], AEDIT);
	killstack[killsp] = cp;
	killsp = (killsp + 1) % KILLSIZE;
}

static int
#if __STDC__
x_yank(int c)
#else
x_yank(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (killsp == 0)
		killtp = KILLSIZE;
	else
		killtp = killsp;
	killtp --;
	if (killstack[killtp] == 0)  {
		x_puts("\nnothing to yank");
		x_redraw(-1);
		return KSTD;
	}
	xmp = xcp;
	x_ins(killstack[killtp]);
	return KSTD;
}

static int
#if __STDC__
x_meta_yank(int c)
#else
x_meta_yank(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	int	len;
#if 1

	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (x_last_command != x_yank && x_last_command != x_meta_yank)  {
		x_puts("\nyank something first");
		x_redraw(-1);
		return KSTD;
	}
	len = strlen(killstack[killtp]);
	x_goto(xcp - len);
	x_delete(len);
	do  {
		if (killtp == 0)
			killtp = KILLSIZE - 1;
		else
			killtp--;
	}  while (killstack[killtp] == 0);
	x_ins(killstack[killtp]);
	return KSTD;
}

static int
#if __STDC__
x_abort(int c)
#else
x_abort(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	/* x_zotc(c); */
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	xlp = xep = xcp = xbp = xbuf;
	xlp_valid = TRUE;
	*xcp = 0;
	return KINTR;
}

static int
#if __STDC__
x_error(int c)
#else
x_error(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	x_putc(BEL);
	return KSTD;
}

static int
#if __STDC__
x_stuffreset(int c)
#else
x_stuffreset(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#ifdef TIOCSTI
	(void)x_stuff(c);
	return KINTR;
#else
	x_zotc(c);
	xlp = xcp = xep = xbp = xbuf;
	xlp_valid = TRUE;
	*xcp = 0;
	x_redraw(-1);
	return KSTD;
#endif
}

static int
#if __STDC__
x_stuff(int c)
#else
x_stuff(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 0 || defined TIOCSTI
	char	ch = c;
	bool_t	savmode = x_mode(FALSE);

	(void)ioctl(ttyfd, TIOCSTI, &ch);
	(void)x_mode(savmode);
	x_redraw(-1);
#else
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	return KSTD;
}

static void
#if __STDC__
x_mapin(char *cp)
#else
x_mapin(cp)
	char	*cp;
#endif /* Xn 1992-07-17 */
{
	char	*op;

	op = cp;
	while (*cp)  {
		/* XXX -- should handle \^ escape? */
		if (*cp == '^')  {
			cp++;
			if (*cp >= '?')	/* includes '?'; ASCII */
#if 0
				*op++ = CTRL(*cp);
#else
				*op++ = (char)(CTRL(*cp));
#endif /* Xn 1992-07-20 */
			else  {
				*op++ = '^';
				cp--;
			}
		} else
			*op++ = *cp;
		cp++;
	}
	*op = 0;
}

static char *
#if __STDC__
x_mapout(int c)
#else
x_mapout(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	static char buf[8];
	register char *p = buf;

	if (c < ' ' || c == 0x7F)  { /* ASCII */
		*p++ = '^';
#if 0
		*p++ = (c == 0x7F) ? '?' : (c | 0x40);
	} else
		*p++ = c;
#else
		*p++ = (char)((c == 0x7F) ? '?' : (c | 0x40));
	} else
		*p++ = (char)c;
#endif /* Xn 1992-07-20 */
	*p = 0;
	return buf;
}

static void
#if __STDC__
x_print(int prefix, int key)
#else
x_print(prefix, key)
	int prefix, key;
#endif /* Xn 1992-07-17 */
{
	if (prefix == 1)
		shellf("%s", x_mapout(x_prefix1));
	if (prefix == 2)
		shellf("%s", x_mapout(x_prefix2));
	shellf("%s = ", x_mapout(key));
	if (x_tab[prefix][key]->xf_func != x_ins_string)
		shellf("%s\n", x_tab[prefix][key]->xf_name);
	else
		shellf("'%s'\n", x_atab[prefix][key]);
}

void
#if __STDC__
x_bind(char *a1, char *a2, int macro)
#else
x_bind(a1, a2, macro)
	char *a1, *a2;
	int macro;		/* bind -m */
#endif /* Xn 1992-07-17 */
{
	struct x_ftab Const *fp;
	int prefix, key;
	char *sp = NULL;

	if (x_tab == NULL)
		errorf("cannot bind, not a tty\n");

	if (a1 == NULL) {
		for (prefix = 0; prefix < 3; prefix++)
		    for (key = 0; key < 0x80; key++) {
			fp = x_tab[prefix][key];
			if (fp == NULL ||
			    fp->xf_func == x_insert || fp->xf_func == x_error)
				continue;
			x_print(prefix, key);
		    }
		return;
	}

	x_mapin(a1);
	prefix = key = 0;
	for (;; a1++) {
		key = *a1;
		if (x_tab[prefix][key]->xf_func == x_meta1)
			prefix = 1;
		else
		if (x_tab[prefix][key]->xf_func == x_meta2)
			prefix = 2;
		else
			break;
	}

	if (a2 == NULL) {
		x_print(prefix, key);
		return;
	}

	if (*a2 == 0)
		fp = xft_insert;
	else if (!macro) {
		for (fp = x_ftab; fp->xf_func; fp++)
			if (strcmp(fp->xf_name, a2) == 0)
				break;
		if (fp->xf_func == NULL || (fp->xf_flags & XF_NOBIND))
			errorf("%s: no such function\n", a2);
		if (fp->xf_func == x_meta1)
			x_prefix1 = key;
		if (fp->xf_func == x_meta2)
			x_prefix2 = key;
	} else {
		fp = xft_ins_string;
		x_mapin(a2);
		sp = strsave(a2, AEDIT);
	}

	if ((x_tab[prefix][key]->xf_flags & XF_ALLOC) && x_atab[prefix][key])
		afree((Void *)x_atab[prefix][key], AEDIT);
	x_tab[prefix][key] = fp;
	x_atab[prefix][key] = sp;
}

void
#if __STDC__
x_init_emacs(void)
#else
x_init_emacs()
#endif /* Xn 1992-07-17 */
{
	register int i, j;
	struct x_ftab Const *fp;

	ainit(AEDIT);

#if 0
	x_tab = (struct x_ftab ***) alloc(sizeofN(*x_tab, 3), AEDIT);
#else
	x_tab = (struct x_ftab Const *(*)[128]) alloc(sizeofN(*x_tab, 3), AEDIT);
#endif /* Xn 1992-07-16 */
	for (j = 0; j < 128; j++)
		x_tab[0][j] = xft_insert;
	for (i = 1; i < 3; i++)
		for (j = 0; j < 128; j++)
			x_tab[i][j] = xft_error;
	for (fp = x_ftab; fp->xf_func; fp++)
		if (fp->xf_db_char || fp->xf_db_tab)
			x_tab[fp->xf_db_tab][fp->xf_db_char] = fp;

#if 0
	x_atab = (char ***) alloc(sizeofN(*x_atab, 3), AEDIT);
#else
	x_atab = (char *(*)[128]) alloc(sizeofN(*x_atab, 3), AEDIT);
#endif /* Xn 1992-07-16 */
	for (i = 1; i < 3; i++)
		for (j = 0; j < 128; j++)
			x_atab[i][j] = NULL;
}

void
#if __STDC__
x_emacs_keys(int erase, int kill, int werase, int intr, int quit)
#else
x_emacs_keys(erase, kill, werase, intr, quit)
	int erase, kill, werase, intr, quit;
#endif /* Xn 1992-07-17 */
{
	x_tab[0][erase] = xft_erase;
	x_tab[0][kill] = xft_kill;
	x_tab[0][werase] = xft_werase;
	x_tab[0][intr] = xft_intr;
	x_tab[0][quit] = xft_quit;
}

static int
#if __STDC__
x_set_mark(int c)
#else
x_set_mark(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	xmp = xcp;
	return KSTD;
}

static int
#if __STDC__
x_kill_region(int c)
#else
x_kill_region(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	int	rsize;
	char	*xr;

#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (xmp == NULL) {
		x_putc(BEL);
		return KSTD;
	}
	if (xmp > xcp) {
		rsize = xmp - xcp;
		xr = xcp;
	} else {
		rsize = xcp - xmp;
		xr = xmp;
	}
	x_goto(xr);
	x_push(rsize);
	x_delete(rsize);
	xmp = xr;
	return KSTD;
}

static int
#if __STDC__
x_xchg_point_mark(int c)
#else
x_xchg_point_mark(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	char	*tmp;

#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if (xmp == NULL) {
		x_putc(BEL);
		return KSTD;
	}
	tmp = xmp;
	xmp = xcp;
	x_goto( tmp );
	return KSTD;
}

#if 0
static int
#if __STDC__
x_copy_arg(int c)
#else
x_copy_arg(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	char *last;
#if 1

	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	if ((last = strval(local("_"))) && *last)
		x_ins(last);
	return KSTD;
}
#endif

static int
#if __STDC__
x_noop(int c)
#else
x_noop(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	return KNULL;
}

#ifdef SILLY
static int
#if __STDC__
x_game_of_life(int c)
#else
x_game_of_life(c)
	int c;
#endif /* Xn 1992-07-17 */
{
	char	newbuf [256+1];
	register char *ip, *op;
	int	i, len;

#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	i = xep - xbuf;
	*xep = 0;
	len = x_size_str(xbuf);
	xcp = xbp = xbuf;
	memmove(newbuf+1, xbuf, i);
	newbuf[0] = 'A';
	newbuf[i] = 'A';
	for (ip = newbuf+1, op = xbuf; --i >= 0; ip++, op++)  {
		/*  Empty space  */
		if (*ip < '@' || *ip == '_' || *ip == 0x7F)  {
			/*  Two adults, make whoopee */
			if (ip[-1] < '_' && ip[1] < '_')  {
				/*  Make kid look like parents.  */
#if 0
				*op = '`' + ((ip[-1] + ip[1])/2)%32;
#else
				*op = (char)('`' + ((ip[-1] + ip[1])/2)%32);
#endif /* Xn 1992-07-20 */
				if (*op == 0x7F) /* Birth defect */
					*op = '`';
			}
			else
				*op = ' ';	/* nothing happens */
			continue;
		}
		/*  Child */
		if (*ip > '`')  {
			/*  All alone, dies  */
			if (ip[-1] == ' ' && ip[1] == ' ')
				*op = ' ';
			else	/*  Gets older */
#if 0
				*op = *ip-'`'+'@';
#else
				*op = (char)(*ip-'`'+'@');
#endif /* Xn 1992-07-20 */
			continue;
		}
		/*  Adult  */
		/*  Overcrowded, dies */
		if (ip[-1] >= '@' && ip[1] >= '@')  {
			*op = ' ';
			continue;
		}
		*op = *ip;
	}
	*op = 0;
	x_redraw(len);
	return KSTD;
}
#endif

/*
 *	File/command name completion routines
 */

/* type: 0 for list, 1 for completion */

static	XPtrV words;

static void
#if __STDC__
add_stash(char *dirnam, char *name)
#else
add_stash(dirnam, name)
	char *dirnam;	/* directory name, if file */
	char *name;
#endif /* Xn 1992-07-17 */
{
	char *cp;
	register int type = 0;	/* '*' if executable, '/' if directory, else 0 */
	register int len = strlen(name);

	/* determine file type */
	if (dirnam)  {
		struct stat statb;
		char *buf = alloc((size_t)(strlen(dirnam)+len+2), ATEMP);

		if (strcmp(dirnam, ".") == 0)
			*buf = '\0';
		else if (strcmp(dirnam, "/") == 0)
			(void)strcpy(buf, "/");
		else
			(void)strcat(strcpy(buf, dirnam), "/");
		(void)strcat(buf, name);
		if (stat(buf, &statb)==0)
			if (S_ISDIR(statb.st_mode))
				type = '/';
#if _POSIX_SOURCE
			else if (S_ISREG(statb.st_mode) && access(buf, X_OK)==0)
#else
			else if (S_ISREG(statb.st_mode) && access(buf, 01)==0)
#endif /* Xn 1992-07-31 */
				type = '*';
		if (type)
			++len;
		afree((Void *)buf, ATEMP);
	}

	if (len > x_maxlen)
		x_maxlen = len;

	/* stash name for later sorting */
	cp = alloc((size_t)(len+1), ATEMP);
	(void)strcpy(cp = alloc((size_t)(len+1), ATEMP), name);
	if (dirnam && type)  {	/* append file type indicator */
#if 0
		cp[len-1] = type;
#else
		cp[len-1] = (char)type;
#endif /* Xn 1992-07-20 */
		cp[len] = '\0';
	}
	XPput(words, cp);
}

static void
#if __STDC__
list_stash(void)
#else
list_stash()
#endif /* Xn 1992-07-17 */
{
	register char **array, **record;
	int items = 0, tabstop, loc, nrows, jump, offset;

	items = XPsize(words);
	array = (char**) XPptrv(words);
	if (items == 0)
		return;
	qsortp(XPptrv(words), (size_t)XPsize(words), xstrcmp);

	/* print names */
	x_maxlen = (x_maxlen/8 + 1) * 8;	/* column width */
	nrows = (items-1) / (x_cols/x_maxlen) + 1;
	for (offset = 0; offset < nrows; ++offset)  {
		tabstop = loc = 0;
		x_putc('\n');
		for (jump = 0; offset+jump < items; jump += nrows)  {
			if (jump)
				while (loc < tabstop)  {
					x_putc('\t');
					loc = (loc/8 + 1) * 8;
				}
			record = array + (offset + jump);
			x_puts(*record);
			loc += strlen(*record);
			tabstop += x_maxlen;	/* next tab stop */
			afree((Void *)*record, ATEMP);
		}
	}

	afree((Void*)array, ATEMP);
	x_redraw(-1);
}

static int
#if __STDC__
x_comp_comm(int c)
#else
x_comp_comm(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	compl_command(1);
	return KSTD;
}

static int
#if __STDC__
x_list_comm(int c)
#else
x_list_comm(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	compl_command(0);
	return KSTD;
}

static int
#if __STDC__
x_complete(int c)
#else
x_complete(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	compl_dec(1);
	return KSTD;
}

static int
#if __STDC__
x_enumerate(int c)
#else
x_enumerate(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	compl_dec(0);
	return KSTD;
}

static int
#if __STDC__
x_comp_file(int c)
#else
x_comp_file(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	compl_file(1);
	return KSTD;
}

static int
#if __STDC__
x_list_file(int c)
#else
x_list_file(c)
	int c;
#endif /* Xn 1992-07-17 */
{
#if 1
	if (c)
		; /* do nothing */
#endif /* Xn 1992-07-20 */
	compl_file(0);
	return KSTD;
}

static void
#if __STDC__
compl_dec(int type)
#else
compl_dec(type)
	int type;
#endif /* Xn 1992-07-17 */
{
	char	*cp;
	cp = xcp;

	while (cp != xbuf && !iscfs(*cp))
		cp--;
	if (cp == xbuf && strchr(cp, '/') == NULL)
		compl_command(type);
	else
		compl_file(type);
}

static void
#if __STDC__
compl_file(int type)
#else
compl_file(type)
	int type;
#endif /* Xn 1992-07-17 */
{
	char	*str;
	register char *cp, *xp;
	char	*lastp;
	char	*dirnam;
	char	buf [256+1];
	char	bug [256+1];
	DIR    *dirp;
	struct dirent *dp;
	long	loc = -1;
	int	len;
	int	multi = 0;

	str = xcp;
	cp = buf;
	xp = str;
	while (xp != xbuf)  {
		--xp;
		if (iscfs(*xp))  {
			xp++;
			break;
		}
	}
	if (digit(*xp) && (xp[1] == '<' || xp[1] == '>'))
		xp++;
	while (*xp == '<' || *xp == '>')
		xp++;
	if (type)
		while (*xcp && !iscfs(*xcp))
			x_zotc(*xcp++);
	else {
		x_maxlen = 0;
		XPinit(words, 16);
	}
	while (*xp && !iscfs(*xp))
		*cp++ = *xp++;

	*cp = 0;
	strcpy(buf, cp = substitute(buf, DOTILDE));
	afree((Void*)cp, ATEMP);
	lastp = strrchr(buf, '/');
	if (lastp)
		*lastp = 0;

	dirnam = (lastp == NULL) ? "." : (lastp == buf) ? "/" : buf;
	dirp = opendir(dirnam);
	if (dirp == NULL) {
		x_putc(BEL);
		return;
	}

	if (lastp == NULL)
		lastp = buf;
	else
		lastp++;
	len = strlen(lastp);

	while ((dp = readdir(dirp)) != NULL)  {
		cp = dp->d_name;
		if (cp[0] == '.' &&
		    (cp[1] == '\0' || cp[1] == '.' && cp[2] == '\0'))
			continue; /* always ignore . and .. */
		if (strncmp(lastp, cp, len) == 0)
			if (type)  {
				if (loc == -1)  {
					(void)strcpy(bug, cp);
					loc = strlen(cp);
				} else {
					multi = 1;
					loc = strmatch(bug, cp);
					bug[loc] = 0;
				}
			} else
				add_stash(dirnam, cp);
	}
	(void)closedir(dirp);

	if (type) {
		if (loc <= 0)  {
			x_putc(BEL);
			return;
		}
		cp = bug + len;
		x_ins(cp);
		if (!multi)  {
			struct stat statb;
			if (lastp == buf)
				buf[0] = 0;
			else if (lastp == buf + 1)  {
				buf[1] = 0;
				buf[0] = '/';
			}  else
				(void)strcat(buf, "/");
			(void)strcat(buf, bug);
			if (stat(buf, &statb) == 0 && S_ISDIR(statb.st_mode))
				x_ins("/");
			else
				x_ins(" ");
		}
	} else
		list_stash();
}

static void
#if __STDC__
compl_command(int type)
#else
compl_command(type)
	int type;
#endif /* Xn 1992-07-17 */
{
	register struct tbl *tp;
	char	*str;
	char	buf [256+1];
	char	bug [256+1];
	char	*xp;
	char	*cp;
	int  len;
	int  multi;
	int  loc;

	str = xcp;
	cp = buf;
	xp = str;
	while (xp != xbuf)  {
		--xp;
		if (iscfs(*xp))  {
			xp++;
			break;
		}
	}
	if (type)
		while (*xcp && !iscfs(*xcp))
			x_zotc(*xcp++);
	else {
		x_maxlen = 0;
		XPinit(words, 16);
	}
	while (*xp && !iscfs(*xp))
		*cp++ = *xp++;
	*cp = 0;

	len = strlen(buf);
	loc = -1;
	multi = 0;

	for (twalk(&commands); (tp = tnext()) != NULL; ) {
		int	klen;

		if (!(tp->flag&ISSET))
			continue;
		klen = strlen(tp->name);
		if (klen < len)
			continue;
		if (strncmp(buf, tp->name, len) ==0)
			if (type)  {
				if (loc == -1)  {
					(void)strcpy(bug, tp->name);
					loc = klen;
				} else {
					multi = 1;
					loc = strmatch(bug, tp->name);
					bug[loc] = 0;
				}
			} else
				add_stash((char *)0, tp->name);
	}

	if (type)  {
		if (loc <= 0)  {
			x_putc(BEL);
			return;
		}
		cp = bug + len;
		x_ins(cp);
		if (!multi)
			x_ins(" ");
	} else
		list_stash();
}

static int
#if __STDC__
strmatch(register char *s1, register char *s2)
#else
strmatch(s1, s2)
	register char *s1, *s2;
#endif /* Xn 1992-07-17 */
{
	register char *p;

	for (p = s1; *p == *s2++ && *p != 0; p++)
		;
	return p - s1;
}



/* NAME:
 *      x_set_arg - set an arg value for next function
 *
 * DESCRIPTION:
 *      This is a simple implementation of M-[0-9].
 *
 * RETURN VALUE:
 *      KSTD
 */

static int
#if __STDC__
x_set_arg(int c)
#else
x_set_arg(c)
	int c;
#endif /* Xn 1992=07-17 */
{
  if ((x_arg = (c &= CMASK) - '0') < 0 || x_arg > 9)
  {
    x_arg = 1;
    x_putc(BEL);
  }
  return KSTD;
}


/* NAME:
 *      x_prev_histword - recover word from prev command
 *
 * DESCRIPTION:
 *      This function recovers the last word from the previous 
 *      command and inserts it into the current edit line.  If a 
 *      numeric arg is supplied then the n'th word from the 
 *      start of the previous command is used.  
 *      
 *      Bound to M-.
 *
 * RETURN VALUE:
 *      KSTD
 */

static int
#if __STDC__
x_prev_histword(void)
#else
x_prev_histword()
#endif /* Xn 1992-07-17 */
{
  register char *rcp;
  char *cp;
  char **hp;

  hp = x_histp-1;
  if (hp < history || hp > histptr)
  {
    x_putc(BEL);
#if 0
    return;
#else
    return KSTD;
#endif /* Xn 1992-07-16 */
  }
  cp = *hp;
  if (x_last_command != x_set_arg)
  {
    rcp = &cp[strlen(cp) - 1];
    /*
     * ignore white-space after the last word
     */
    while (rcp > cp && iscfs(*rcp))
      rcp--;
    while (rcp > cp && !iscfs(*rcp))
      rcp--;
    if (iscfs(*rcp))
      rcp++;
    x_ins(rcp);
  }
  else
  {
    int c;
    
    rcp = cp;
    /*
     * ignore white-space at start of line
     */
    while (*rcp && iscfs(*rcp))
      rcp++;
    while (x_arg-- > 1)
    {
      while (*rcp && !iscfs(*rcp))
	rcp++;
      while (*rcp && iscfs(*rcp))
	rcp++;
    }
    cp = rcp;
    while (*rcp && !iscfs(*rcp))
      rcp++;
    c = *rcp;
    *rcp = '\0';
    x_ins(cp);
#if 0
    *rcp = c;
#else
    *rcp = (char)c;
#endif /* Xn 1992-07-20 */
  }
  return KSTD;
}

/* NAME:
 *      x_fold_case - convert word to UPPER/lower case
 *
 * DESCRIPTION:
 *      This function is used to implement M-u,M-l and M-c
 *      to upper case, lower case or Capitalize words.
 *
 * RETURN VALUE:
 *      None
 */

static int
#if __STDC__
x_fold_case(int c)
#else
x_fold_case(c)
	int c;
#endif /* Xn 1992-07-17 */
{
  register char	*cp = xcp;
  
  if (cp == xep)
  {
    x_putc(BEL);
    return 0;
  }
  c &= 0137;				/* strip prefixes and case */
  if (x_last_command != x_set_arg)
    x_arg = 1;
  while (x_arg--)
  {
    /*
     * fisrt skip over any white-space
     */
    while (cp != xep && ismfs(*cp))
    {
      cp++;
    }
    /*
     * do the first char on its own since it may be
     * a different action than for the rest.
     */
    if (cp != xep)
    {
      if (c == 'L')			/* M-l */
      {
	if (isupper(*cp))
#if 0
	  *cp = tolower(*cp);
#else
	  *cp = (char)tolower(*cp);
#endif /* Xn 1992-07-20 */
      }
      else				/* M-u or M-c */
      {
	if (islower(*cp))
#if 0
	  *cp = toupper(*cp);
#else
	  *cp = (char)toupper(*cp);
#endif /* Xn 1992-07-20 */
      }
      cp++;
    }
    /*
     * now for the rest of the word
     */
    while (cp != xep && !ismfs(*cp))
    {
      if (c == 'U')			/* M-u */
      {
	if (islower(*cp))
#if 0
	  *cp = toupper(*cp);
#else
	  *cp = (char)toupper(*cp);
#endif /* Xn 1992-07-20 */
      }
      else				/* M-l or M-c */
      {
	if (isupper(*cp))
#if 0
	  *cp = tolower(*cp);
#else
	  *cp = (char)tolower(*cp);
#endif /* Xn 1992-07-20 */
      }
      cp++;
    }
  }
  x_goto(cp);
  return 0;
}

/* NAME:
 *      x_lastcp - last visible char
 *
 * SYNOPSIS:
 *      x_lastcp()
 *
 * DESCRIPTION:
 *      This function returns a pointer to that  char in the 
 *      edit buffer that will be the last displayed on the 
 *      screen.  The sequence:
 *      
 *      for (cp = x_lastcp(); cp > xcp; cp)
 *        x_bs(*--cp);
 *      
 *      Will position the cursor correctly on the screen.
 *
 * RETURN VALUE:
 *      cp or NULL
 */

char *
#if __STDC__
x_lastcp(void)
#else
x_lastcp()
#endif /* Xn 1992-07-17 */
{
  register char *rcp;
  register int i;

  if (!xlp_valid)
  {
    for (i = 0, rcp = xbp; rcp < xep && i < x_displen; rcp++)
      i += x_size(*rcp);
    xlp = rcp;
  }
  xlp_valid = TRUE;
  return (xlp);
}

#endif /* EDIT */

