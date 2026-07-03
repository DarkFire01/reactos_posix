/*
 * command history
 *
 * only implements in-memory history.
 */

static char *RCSid = "$Id: history.c,v 3.3 89/01/27 00:08:27 egisin Exp $";

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#if _POSIX_SOURCE
#include <unistd.h>
#endif /* Xn 1992-07-31 */
#include "sh.h"
#include "lex.h"
#if 1
#include "table.h"
#endif /* Xn 1992-07-20 */

#if 0
char   *histrpl();
#else
extern int command ARGS((register char *)); /* in main.c */
extern int getn ARGS((char *)); /* in misc.c */
char *histrpl ARGS((char *, char *, char *, int));
void histbackup ARGS((void));
#endif /* Xn 1992-07-20 */
char  **current;
int	curpos;

int
#if __STDC__
c_fc(register char **wp)
#else
c_fc(wp)
	register char **wp;
#endif /* Xn 1992-07-17 */
{
	register char *id;
	FILE *f;
	struct temp *tf;
	register char **hp;
	char **hbeg, **hend;
	char *p, *cmd = NULL;
	int lflag = 0, nflag = 0, sflag = 0, rflag = 0, gflag = 0;
	int done = 0;
#if 0
	void histbackup();
#endif /* Xn 1992-07-20 */

	for (wp++; (id = *wp) != NULL && *id++ == '-' && !done; wp++)
		while (*id && !done) {
			switch (*id++) {
			  case 'l':
				lflag++;
				break;
			  case 'n':
				nflag++;
				break;
			  case 'r':
				rflag++;
				break;
			  case 'g':
				gflag++;
				break;
			  case 'e':
#if 0
				if (++wp && (p = *wp)) {
#else
				if (++wp && (p = *wp) != NULL) {
#endif /* Xn 1992-07-20 */
					if (p[0] == '-' && !p[1]) {
						sflag++;
					} else {
						cmd = alloc((size_t)(strlen(p)+4),ATEMP);
						strcpy(cmd, p);
						strcat(cmd, " $_");
					}
				} else
					errorf("argument expected\n");
				id = "";
				break;
			  default:
				wp--;
				done++;
				break;
			}
		}

	if (sflag) {
		char *pat = NULL, *rep = NULL;

		hp = histptr - 1;
		while ((id = *wp++) != NULL) {
			/* todo: multiple substitutions */
			if ((p = strchr(id, '=')) != NULL) {
				pat = id;
				rep = p;
				*rep++ = '\0';
			} else
				hp = histget(id);
		}

		if (hp == NULL || hp < history)
			errorf("cannot find history\n");
		if (pat == NULL)
			strcpy(line, *hp);
		else
			histrpl(*hp, pat, rep, gflag);
		histbackup();
		histsave(line); 
		histpush--; 
		line[0] = '\0';
		return 0;
	}

	if (*wp != NULL) {
		hbeg = histget(*wp++); /* first */
		if (*wp != NULL)
			hend = histget(*wp++); /* last */
		else if (lflag)
			hend = histptr;
		else
			hend = hbeg;
	} else {
		if (lflag)
			hbeg = histptr - 16, hend = histptr;
		else
			hbeg = hend = histptr - 1;
		if (hbeg < history)
			hbeg = history;
	}
	if (hbeg == NULL || hend == NULL)
		errorf("can't find history\n");

	if (lflag)
		f = stdout;
	else {
		nflag++;
		tf = maketemp(ATEMP);
		tf->next = e.temps; e.temps = tf;
		f = fopen(tf->name, "w");
		if (f == NULL)
			errorf("cannot create temp file %s", tf->name);
		setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);
	}

	for (hp = (rflag ? hend : hbeg); rflag ? (hp >= hbeg) : (hp <= hend);
	      rflag ? hp-- : hp++) {
		if (!nflag)
			fprintf(f, "%3d: ", source->line - (int)(histptr-hp));
		fprintf(f, "%s\n", *hp);
	}

	if (lflag)
		return 0;
	else
		fclose(f);

	setstr(local("_"), tf->name);
	if (cmd) {
		command(cmd); /* edit temp file */
		afree(cmd, ATEMP);
	} else
		command("${FCEDIT:-/bin/ed} $_");

	f = fopen(tf->name, "r");
	if (f == NULL)
		errorf("cannot open temp file %s\n", tf->name);
	setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);
	/* we push the editted lines onto the history list */
	while (fgets(line, sizeof(line), f) != NULL) {
		histsave(line); 
		histpush--; 
	}
	line[0] = '\0';
	fclose(f);

	return 0;
}

/******************************/
/* Back up over last histsave */
/******************************/
void
#if __STDC__
histbackup(void)
#else
histbackup()
#endif /* Xn 1992-07-17 */
{
	static int last_line = -1;

	if (histptr > history && last_line != source->line) { 
		source->line--;
		afree((Void*)*histptr, APERM);
		histptr--;
		last_line = source->line;
	}
}

/*
 * save command in history
 */
void
#if __STDC__
histsave(char *cmd)
#else
histsave(cmd)
	char *cmd;
#endif /* Xn 1992-07-17 */
{
	register char **hp = histptr;
	char *cp;

	if (++hp >= history + HISTORY) { /* remove oldest command */
		afree((Void*)*history, APERM);
		for (hp = history; hp < history + HISTORY - 1; hp++)
			hp[0] = hp[1];
	}
	*hp = strsave(cmd, APERM);
	if ((cp = strchr(*hp, '\n')) != NULL)
		*cp = '\0';
	histptr = hp;
}

/*
 * get pointer to history given pattern
 * pattern is a number or string
 */
char **
#if __STDC__
histget(char *str)
#else
histget(str)
	char *str;
#endif /* Xn 1992-07-17 */
{
	register char **hp = NULL;

	if (*str == '-')
		hp = histptr + getn(str);
	else
	if (digit(*str))
		hp = histptr + (getn(str) - source->line);
	else 
	if (*str == '?') {	/* unanchored match */
		for (hp = histptr-1; hp >= history; hp--)
			if (strstr(*hp, str+1) != NULL)
				break;
	} else {		/* anchored match */
#if 0
		for (hp = histptr; hp >= history; hp--)
#else
		for (hp = histptr-1; hp >= history; hp--)
#endif /* Xn 1993-06-24 */
			if (strncmp(*hp, str, strlen(str)) == 0)
				break;
	}

	return (history <= hp && hp <= histptr) ? hp : NULL;
}

char *
#if __STDC__
histrpl(char *s, char *pat, char *rep, int global)
#else
histrpl(s, pat, rep, global)
	char *s;
	char *pat, *rep;
	int global;
#endif /* Xn 1992-07-17 */
{
	char *s1, *p, *last = NULL;
	int len = strlen(pat);

	if (strlen(s) - strlen(pat) + strlen(rep) >= LINE)
		errorf("substitution too long\n");
	line[0] = '\0';
	p = line;
#if 0
	while (s1 = strstr(s, pat)) {
#else
	while ((s1 = strstr(s, pat)) != NULL) {
#endif /* Xn 1992-07-20 */
		strncpy(p, s, s1 - s);		/* first part */
		strcpy(p + (s1 - s), rep);	/* replacement */
		s = s1 + len;
		last = s1;
		p = strchr(p, 0);
		if (!global)
			s = "";
	}
	if (last)
		strcpy(p, last + len);		/* last part */
	else
		errorf("substitution failed\n");
	return line;
}

#if 0

/* History file management routines (by DPK@BRL) */

void
#if __STDC__
hist_init(void)
#else
hist_init()
#endif /* Xn 1992-07-17 */
{
	register struct namnod *n;
	int fd;

	if (hist_fd >= 0 || (flags&oneflg))
		return;
	if ((n = findnam(histname)) == (struct namnod *)0
	 || n->namval == (char *)0)
		return;
	if ((fd = open(n->namval, O_RDWR)) >= 0) {
		hist_load(fd);
		(void)fcntl(fd, F_SETFL, O_APPEND);
	}
	hist_fd = fd;
}

void
#if __STDC__
hist_finish(void)
#else
hist_finish()
#endif /* Xn 1992-07-17 */
{
	if (hist_fd >= 0)
		(void)close(hist_fd);
	hist_fd = -1;
}

void
#if __STDC__
hist_record(char *buf, int len)
#else
hist_record(buf, len)
	char *buf;
	int len;
#endif /* Xn 1992-07-17 */
{
	if (hist_fd >= 0)
		(void)write(hist_fd, buf, (unsigned)len);
}

void
#if __STDC__
hist_load(int fd)
#else
hist_load(fd)
	int fd;
#endif /* Xn 1992-07-17 */
{
#if 0
	extern long	lseek();
#endif /* Xn 1992-07-31 */
	struct stat sb;
	char *x;
	register char *cmdp, *end;
	register int	len;
	register int	i;

	if (fstat(fd, &sb) < 0 || sb.st_size <= 0)
		return;
	if (x = alloc((unsigned)(sb.st_size+1))) {
#if _POSIX_SOURCE
		(void)lseek(fd, (off_t)0, SEEK_SET);
		if ((len = read(fd, x, (size_t)sb.st_size)) <= 0) {
			free((struct blk *)x);
			return;
		}
#else
		(void)lseek(fd, 0L, 0);
		if ((len = read(fd, x, (unsigned)sb.st_size)) <= 0) {
			free((struct blk *)x);
			return;
		}
#endif /* Xn 1992-07-31 */
		x[len] = 0;
		end = x;
		for (;;) {
			while(*end == NL)
				end++;		/* Skip NL */
			if (*end == 0)
				break;
			cmdp = end;
			while(*end && *end != NL)
				end++;	/* Goto NL */
			if (*end == 0)
				break;
			if ((len = (end - cmdp)) < 2)
				continue;
			if (len >= BUFSIZ)
				len = BUFSIZ - 1;		/* Protection */
			i = curhist % NHISTORY;
			if(histbuf[i])
				free((struct blk *)histbuf[i]);
			histbuf[i] = alloc((unsigned)(len+1));
			(void)strncpy(histbuf[i], cmdp, len);
			histbuf[i][len] = 0;
			curhist++;
			histpc=curhist;
		}
		free((struct blk *)x);
	}
	return;
}

#endif

/*
 * Return the current position.
 */
char **
#if __STDC__
histpos(void)
#else
histpos()
#endif /* Xn 1992-07-17 */
{
	return current;
}

int
#if __STDC__
histN(void)
#else
histN()
#endif /* Xn 1992-07-17 */
{
	return curpos;
}

int
#if __STDC__
histnum(int n)
#else
histnum(n)
	int n;
#endif /* Xn 1992-07-17 */
{
	int	last = histptr - history;

	if (n < 0 || n >= last) {
		current = histptr;
		curpos = last;
		return last;
	}  else {
		current = &history[n];
		curpos = n;
		return n;
	}
}

/*
 * This will become unecessary if histget is modified to allow
 * searching from positions other than the end, and in either 
 * direction.
 */
char *
#if __STDC__
findhist(int start, int fwd, char *str)
#else
findhist(start, fwd, str)
	int	start;
	int	fwd;
	char 	*str;
#endif /* Xn 1992-07-17 */
{
	char 	**hp = NULL;
	int	 pos = start;
	char	 *line, *last;

	/* XXX check that we are valid after this */
	if (fwd)
		pos++;
	else
		pos--;
	histnum(pos);
	line = *histpos();
	do {
		last = line;
		if (strstr(line, str) != 0) {
			/* keep position current */
			return (line);
		}
		if (fwd)
			pos++;
		else
			pos--;
		histnum(pos);
		line = *histpos();
	} while (line && *line && line != last && pos>0);

	histnum(start);
	if (pos <= 0)
		return (char*)-1; /* TODO */
	return NULL;
}
