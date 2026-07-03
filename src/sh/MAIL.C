/*
 * Mailbox checking code by Robert J. Gibson, adapted for PD ksh by
 * John R. MacMillan
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#if 1
#include <time.h>
#endif /* Xn 1992-07-29 */
#include "sh.h"
#include "table.h"
#if 1
#include "tree.h"
#endif /* Xn 1992-07-16 */

#if 0
void	mcheck();	/* check for new mail */
void	mbset();	/* set $MAIL */
void	mpset();	/* set $MAILPATH */
void	mprint();	/* print a list of mail messages */
#else
void	mcheck ARGS((void));	/* check for new mail */
void	mbset ARGS((register char *));	/* set $MAIL */
void	mpset ARGS((register char *));	/* set $MAILPATH */
void	mprint ARGS((void));	/* print a list of mail messages */
#endif /* Xn 1992-07-17 */

#define MBMESSAGE	"you have mail in $_"

typedef struct mbox {
	struct mbox    *mb_next;	/* next mbox in list */
	char	       *mb_path;	/* path to mail file */
	char	       *mb_msg;		/* to announce arrival of new mail */
	off_t	        mb_size;	/* size of mail file (bytes) - Xn 1992-07-29 */
} mbox_t;

struct mailmsg {
	char		*msg;		/* Text of message */
	struct mailmsg	*next;		/* Next message */
};

/*
 * $MAILPATH is a linked list of mboxes.  $MAIL is a treated as a
 * special case of $MAILPATH, where the list has only one node.  The
 * same list is used for both since they are exclusive.
 */

static mbox_t  *mplist = NULL;
static mbox_t  mbox = { NULL, NULL, NULL, 0 };
static long	mlastchkd = 0;	/* when mail was last checked */
static struct mailmsg *mmsgs = NULL;	/* Messages to be printed */
#if 0
static void	munset();		/* free mlist and mval */
static mbox_t  *mballoc();		/* allocate a new mbox */
static void	maddmsg();
#else
static void	munset ARGS((register mbox_t *));		/* free mlist and mval */
static mbox_t  *mballoc ARGS((char *, char *));		/* allocate a new mbox */
static void	maddmsg ARGS((mbox_t *));
#endif /* Xn 1992-07-17 */

void
#if __STDC__
mcheck(void)
#else
mcheck()
#endif /* Xn 1992-07-17 */
{
	register mbox_t	*mbp;
	long		 now;
	struct tbl	*vp, mailcheck;
	struct stat	 stbuf;

	vp = global("MAILCHECK");
	if (!(vp->flag & ISSET) || strint(&mailcheck, vp) == NULL)
		return;

	if (mlastchkd == 0)
		mlastchkd = time((long *)0);

	if ((now=time((long *)0)) - mlastchkd >= mailcheck.val.i) {
		mlastchkd = now;
		
		vp = global("MAILPATH");
		if (vp && (vp->flag & ISSET))
			mbp = mplist;
		else if ((vp = global("MAIL")) != NULL && (vp->flag & ISSET)) /* Xn 1992-07-29 */
			mbp = &mbox;
		else
			mbp = NULL;

		while (mbp) {
			if (stat(mbp->mb_path, &stbuf) == 0 &&
			    (stbuf.st_mode & S_IFMT) == S_IFREG) {
				if (mbp->mb_size < stbuf.st_size)
					maddmsg( mbp );
				mbp->mb_size = stbuf.st_size;
			} else {
				/*
				 * Some mail readers remove the mail
				 * file if all mail is read.  If file
				 * does not exist, assume this is the
				 * case and set size to zero.
				 */
				mbp->mb_size = 0;
			}
			mbp = mbp->mb_next;
		}
	}
}

void
#if __STDC__
mbset(register char *p)
#else
mbset(p)
	register char	*p;
#endif /* Xn 1992-07-17 */
{
	struct stat	stbuf;

	if (mbox.mb_msg)
		afree((Void *)mbox.mb_msg, APERM);
	mbox.mb_path = p;
	mbox.mb_msg = NULL;
	if (stat(p,&stbuf) == 0 && (stbuf.st_mode & S_IFMT) == S_IFREG)
		mbox.mb_size = stbuf.st_size;
	else
		mbox.mb_size = 0;
}

void
#if __STDC__
mpset(register char *mptoparse)
#else
mpset(mptoparse)
	register char	*mptoparse;
#endif /* Xn 1992-07-17 */
{
	register mbox_t	*mbp;
	register char	*mpath, *mmsg, *mval;

	munset( mplist );
	mplist = NULL;
	mval = strsave(mptoparse, APERM);
	while (mval) {
		mpath = mval;
		if ((mval = strchr(mval, ':')) != NULL) {
			*mval ='\0', mval++;
		}
		if ((mmsg = strchr(mpath, '?')) != NULL) {
			*mmsg = '\0', mmsg++;
		}
		mbp = mballoc(mpath, mmsg);
		mbp->mb_next = mplist;
		mplist = mbp;
	}
}

static void
#if __STDC__
munset(register mbox_t *mlist)
#else
munset(mlist)
	register mbox_t	*mlist;
#endif /* Xn 1992-07-17 */
{
	register mbox_t	*mbp;

	while (mlist != NULL) {
		mbp = mlist;
		mlist = mbp->mb_next;
		if (!mlist)
			afree((Void *)mbp->mb_path, APERM);
		afree((Void *)mbp, APERM);
	}
}

static mbox_t *
#if __STDC__
mballoc(char *p, char *m)
#else
mballoc(p, m)
	char	*p;
	char	*m;
#endif /* Xn 1992-07-17 */
{
	struct stat	stbuf;
	register mbox_t	*mbp;

	mbp = (mbox_t *)alloc(sizeof(mbox_t), APERM);
	mbp->mb_next = NULL;
	mbp->mb_path = p;
	mbp->mb_msg = m;
	if (stat(mbp->mb_path, &stbuf) == 0 &&
	   (stbuf.st_mode & S_IFMT) == S_IFREG) {
		mbp->mb_size = stbuf.st_size;
	} else {
		mbp->mb_size = 0;
	}
	return(mbp);
}

void
#if __STDC__
mprint(void)
#else
mprint()
#endif /* Xn 1992-07-17 */
{
	struct mailmsg *mm;

	while ((mm = mmsgs) != NULL) {
		shellf( "%s\n", mm->msg );
		fflush(shlout);
		afree((Void *)mm->msg, APERM);
		mmsgs = mm->next;
		afree((Void *)mm, APERM);
	}
}

static void
#if __STDC__
maddmsg(mbox_t *mbp)
#else
maddmsg(mbp)
	mbox_t *mbp;
#endif /* Xn 1992-07-17 */
{
	struct mailmsg	*message;
	struct tbl	*vp;

	message = (struct mailmsg *)alloc(sizeof(struct mailmsg), APERM);
	setstr((vp = typeset("_", LOCAL, 0)), mbp->mb_path);

	if (mbp->mb_msg)
#if 0
		message->msg = strsave(substitute(mbp->mb_msg,0),APERM);
#else
		message->msg = strsave(substitute((Const char *)mbp->mb_msg,0),APERM);
#endif /* Xn 1992-07-16 */
	else
		message->msg = strsave(substitute(MBMESSAGE,0),APERM);

	unset(vp);
	message->next = mmsgs;
	mmsgs = message;
}
