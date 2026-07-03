/* NAME:
 *      edit.h - globals for edit modes
 *
 * DESCRIPTION:
 *      This header defines various global edit objects.
 *
 * SEE ALSO:
 *      
 *
 * AMENDED:
 *      91/09/17  23:28:04  (sjg)
 *
 * RELEASED:
 *      91/11/09  15:36:18  v1.3
 *
 * SCCSID:
 *      @(#)edit.h  1.3  91/09/17  23:28:04  (sjg)
 *
 */

/* some useful #defines */
#ifdef EXTERN
# define _I_(i) = i
#else
# define _I_(i)
# define EXTERN extern
# define EXTERN_DEFINED
#endif

#define	BEL		0x07

/*
 * The following are used for my horizontal scrolling stuff
 */
EXTERN	char   *xbuf;		/* beg input buffer */
EXTERN	char   *xend;		/* end input buffer */
EXTERN char    *xcp;		/* current position */
EXTERN char    *xep;		/* current end */
EXTERN char    *xbp;		/* start of visible portion of input buffer */
EXTERN char    *xlp;		/* last char visible on screen */
EXTERN int	x_adj_ok;
/*
 * we use x_adj_done so that functions can tell 
 * whether x_adjust() has been called while they are active.
 */
EXTERN int	x_adj_done;

EXTERN int	x_cols;
EXTERN int	x_col;
EXTERN int	x_displen;
EXTERN int	x_arg;		/* general purpose arg */

EXTERN int	x_do_init;		/* set up tty modes */
EXTERN int	ed_erase, ed_kill, ed_werase, ed_intr, ed_quit;

#if defined(__STDC__) || defined(PROTO)
# define _P_(p) p
#else
# define _P_(p) ()
#endif		/* __STDC__ || PROTO */
#ifdef DEBUG
# define _D_(x) x
#else
# define _D_(x)
#endif

#if 0
/****  edit.c  ****/
int             x_read      _P_((int fd, char *buf, size_t len));
int             x_getc      _P_((void));
void            x_flush     _P_((void));
void            x_adjust    _P_((void));
void            x_putc      _P_((int c));
int             x_debug_info _P_((void));
void            x_puts      _P_((char *s));
void            x_init      _P_((void));
bool_t          x_mode      _P_((bool_t onoff));
bool_t          x_mode      _P_((bool_t onoff));
int             promptlen   _P_((char *cp));

/****  emacs.c  ****/
void            x_redraw    _P_((int limit));
char*		x_lastcp    _P_((void));
#else
/****  edit.c  ****/
int             x_read      _P_((int __fd, Void *__buf, size_t __len));
int             x_getc      _P_((void));
void            x_flush     _P_((void));
void            x_adjust    _P_((void));
void            x_putc      _P_((int __c));
#ifdef DEBUG
int             x_debug_info _P_((void));
#endif
void            x_puts      _P_((register char *__s));
void            x_init      _P_((void));
bool_t          x_mode      _P_((bool_t __onoff));
int             promptlen   _P_((register char *__cp));

/****  emacs.c  ****/
void            x_redraw    _P_((int __limit));
char		*x_lastcp   _P_((void));
#endif /* Xn 1992-07-20 */
EXTERN int xlp_valid _I_(0);
  
/* This lot goes at the END */
/* be sure not to interfere with anyone else's idea about EXTERN */
#ifdef EXTERN_DEFINED
# undef EXTERN_DEFINED
# undef EXTERN
#endif
/*
 * Local Variables:
 * version-control:t
 * comment-column:40
 * End:
 */
