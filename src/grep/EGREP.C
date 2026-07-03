/*
     Hybrid Boyer/Moore/Gosper-assisted 'grep/egrep/fgrep' search, with delta0
     table as in original paper (CACM, October, 1977).  No delta1 or delta2.
     According to experiment (Horspool, Soft. Prac. Exp., 1982), delta2 is of
     minimal practical value.  However, to improve for worst case input,
     integrating the improved Galil strategies (Apostolico/Giancarlo, SIAM. J.
     Comput., Feb. 1986) deserves consideration.

     Method: 	extract longest metacharacter-free string from expression.
		this is done using a side-effect from henry spencer's regcomp().
		use boyer-moore to match such, then pass submatching lines
		to either regexp() or standard 'egrep', depending on certain
		criteria within execstrategy() below.  [this tradeoff is due
		to the general slowness of the regexp() nondeterministic
		machine on complex expressions, as well as the startup time
		of standard 'egrep' on short files.]  alternatively, one may
		change the vendor-supplied 'egrep' automaton to include
		boyer-moore directly.  see accompanying writeup for discussion
		of kanji expression treatment.

		late addition:  apply trickbag for fast match of simple
		alternations (sublinear, in common low-cardinality cases).
		trap fgrep into this lair.

		gnu additions:  -f, newline as |, \< and \> [in regexec()], more
				comments.  inspire better dfa exec() strategy.
				serious testing and help with special cases.

     Algorithm amalgam summary:

		dfa e?grep 		(aho/thompson)
		ndfa regexp() 		(spencer/aho)
		bmg			(boyer/moore/gosper)
		"superimposed" bmg   	(jaw)
		fgrep			(aho/corrasick)

		sorry, but the knuth/morris/pratt machine, horspool's
		"frequentist" code, and the rabin/karp matcher, however cute,
		just don't cut it for this production.

     James A. Woods				Copyright (c) 1986
     NASA Ames Research Center
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if _POSIX_SOURCE
# include <unistd.h>
#endif
#include "regexp.h"		/* must be henry spencer's version */
#if WIN_NT
# include "glob.h"
#endif

#define	MIN(A, B)	((A) > (B) ? (B) : (A))

#ifdef	SLOWSYS
#define read	xread
#endif
/*
 * define existing [ef]?grep program locations below for use by execvp().
 * [execlp() would be used were it not for the possibility of
 * installation-dependent recursion.] 
 */
#ifdef WIN_NT
#define EGREPSTD	"/bin/egrep"
#define	FGREPSTD	"/bin/fgrep"
#endif
#ifndef EGREPSTD
#define	EGREPSTD	"/usr/bin/egrep"
#endif
#ifndef GREPSTD
#define	GREPSTD		"/bin/grep"
#endif
#ifndef FGREPSTD
#define	FGREPSTD	"/usr/bin/fgrep"
#endif

#define BUFSIZE	8192		/* make higher for cray */
#define PATSIZE 6000
#define LARGE 	BUFSIZE + PATSIZE

#define ALTSIZE	100		/* generous? */
#define NALT	7		/* tied to scanf() size in alternate() */
#define NMUSH	6		/* loosely relates to expected alt length */

#define	FIRSTFEW	10	/* Always do FIRSTFEW matches with regexec() */
#define	PUNTPERCENT	5	/* After FIRSTFEW, if PUNTPERCENT of the input
				 * was processed by regexp(), exec std egrep. */
#define NL	'\n'
#define	EOS	'\0'
#define	NONASCII	0200	/* Bit mask for Kanji non-ascii chars */
#define META	"\n^$.[]()?+*|\\"	/* egrep meta-characters */
#define SS2	'\216'		/* EUC Katakana (or Chinese2) prefix */
#define SS3	'\217'		/* EUC Kanji2 (or Chinese3) prefix */

#if __STDC__
extern int getopt (int, char **, const char *);
#else
extern int getopt();
#endif
extern char *optarg;
extern int optind;
extern int optopt;
char *progname;

int cflag, iflag, eflag, fflag, lflag, nflag;	/* SVID flags */
int sflag, hflag;		/* v7, v8, bsd */

int firstflag;			/* Stop at first match */
int grepflag;			/* Called as "grep" */
int fgrepflag;			/* Called as "fgrep" */
int egrepflag;			/* Called as neither "grep" nor "fgrep" */
int altflag;			/* Simple alternation in pattern */
int boyonly;			/* No regexp needed -- all simple */
int flushflag;
int grepold, egrepold, fgrepold;

int nalt;			/* Number of alternatives */
int nsuccess;			/* 1 for match, 2 for error */
int altmin;			/* Minimum length of all the alternate
				 * strings */
int firstfile;			/* argv index of first file argument */
long nmatch;			/* Number of matches in this file */
long incount, counted;		/* Amount of input consumed */
long rxcount;			/* Bytes of input processed by regexec() */
int boyfound;			/* accumulated partial matches (tripped by
				 * FIRSTFEW) */
int prevmatch;			/* next three lines aid fast -n */
long nline, prevnline;
char *prevloc;

regexp *rspencer;
char *pattern;
char *patboy;			/* Pattern for simple Boyer-Moore */
char *patfile;			/* Filename containing pattern(s) */

int delta0[256];		/* Boyer-Moore algorithm core */
char cmap[256];			/* Usually 0-255, but if -i, maps upper to
				 * lower case */
char str[BUFSIZE + 2];
int nleftover;
char linetemp[BUFSIZE];
char altpat[NALT][ALTSIZE];	/* alternation component storage */
int altlen[NALT];
short altset[NMUSH + 1][256];
char preamble[200];		/* match prefix (filename, line no.) */

int fd;
#if __STDC__
char *pfile (const char *);
void egsecute (const char *);
void chimaera (const char *, char *);
char *linesave (char *, register int);
char *submatch (const char *, char *, const char *, register const char *,
	register char *, int);
char *kanji (register const char *, register const char *, register char *);
void gosper (const char *);
char *gotamatch (register const char *, register char *);
char *fold (const char *);
int strindex (const char *, const char *);
char *grepxlat (const char *);
char *alternate (char *);
void execstrategy (const char *);
int nlcount (const char *, const char *);
char *isolate (const char *);
char *savematch (register char *);
void flushmatches (void);
void oops (const char *);
void kernighan (char **);
# if WIN_NT
extern int globulate (int, int, char **);
extern void deglobulate (void);
extern int globulated_argc;
extern char **globulated_argv;
# endif
#else
char *
grepxlat(), *fold(), *pfile(), *alternate(), *isolate();
#endif
char **args;
#if WIN_NT
pid_t ppid;
int globulation;
#endif

int
#if __STDC__
main (int argc, char **argv)
#else
main(argc, argv)
	int argc;
	char *argv[];
#endif
{
	int c;
	int errflag = 0;

	args = argv;

#if WIN_NT
	ppid = getppid();
	if ((progname = strrchr(argv[0], (ppid == (pid_t) 1) ? '\\' : '/')) != 0)
#else
	if ((progname = strrchr(argv[0], '/')) != 0)
#endif
		progname++;
	else
		progname = argv[0];
	if (strcmp(progname, "grep") == 0)
		grepflag++;
	else if (strcmp(progname, "fgrep") == 0)
		fgrepflag++;
	else
		egrepflag++;

	while ((c = getopt(argc, argv, "EFbchie:f:lnsvwxy1?")) != EOF) {
		switch (c) {
		case 'E':
			if (!grepflag)
				++errflag;
			++egrepflag;
			continue;
		case 'F':
			if (!grepflag)
				++errflag;
			++fgrepflag;
			continue;
		case 'f':
			fflag++;
			patfile = optarg;
			continue;
		case 'b':
		case 'v':
			egrepold++;	/* boyer-moore of little help here */
			continue;
		case 'c':
			cflag++;
			continue;
		case 'e':
			eflag++;
			pattern = optarg;
			continue;
		case 'h':
			hflag++;
			continue;
		case '1':	/* Stop at very first match */
			firstflag++;	/* spead freaks only */
			continue;
		case 'i':
			iflag++;
			continue;
		case 'l':
			lflag++;
			continue;
		case 'n':
			nflag++;
			continue;
		case 's':
			sflag++;
			continue;
		case 'w':
		case 'y':
			if (egrepflag || fgrepflag)
				errflag++;
			grepold++;
			continue;
		case 'x':	/* needs more work, like -b above */
			if (!fgrepflag)
				errflag++;
			fgrepold++;
			continue;
		case '?':
			errflag++;
		}
	}
	if (errflag || ((argc <= optind) && !fflag && !eflag)) {
		if (grepflag)
oops("usage: grep [-E | -F] [-bcihlnsvwy] [-e] pattern [file ...]");
		else if (fgrepflag)
oops("usage: fgrep [-bcilnv] {-f patfile | [-e] strings} [file ...]");
		else		/* encourage SVID options, though we provide
				 * others */
oops("usage: egrep [-bcilnv] {-f patfile | [-e] pattern} [file ...]");
	}
	if (grepflag && (egrepflag || fgrepflag))
		--grepflag;
#if WIN_NT
	if (ppid == (pid_t) 1) /* if parent is CMD.EXE */
	{
		globulation = globulate(optind, argc, argv);
#if 0
		(void) printf("globulate: %d\n", c);
#endif
		if (globulation == 0)
		{
			argc = globulated_argc;
			argv = globulated_argv;
			args = argv;
#if 0
			for (c = 0; c < argc; ++c)
				(void) printf("[%s] ", argv[c]);
			(void) printf("NULL\n");
#endif
		}
	}
#endif
	if (fflag)
		pattern = pfile(patfile);
	else if (!eflag)
		pattern = argv[optind++];

	if ((argc - optind) <= 1)	/* Filename invisible given < 2 files */
		hflag++;
	if (pattern[0] == EOS)
		oops("empty expression");
	/*
	 * 'grep/egrep' merger -- "old" grep is called to handle: tagged
	 * exprs \( \), word matches \< and \>, -w and -y options, char
	 * classes with '-' at end (egrep bug?), and patterns beginning with
	 * an asterisk (don't ask why). otherwise, characters meaningful to
	 * 'egrep' but not to 'grep' are escaped; the entire expr is then
	 * passed to 'egrep'. 
	 */
	if (grepflag && !grepold) {
		if (strindex(pattern, "\\(") >= 0 ||
		    strindex(pattern, "\\<") >= 0 ||
		    strindex(pattern, "\\>") >= 0 ||
		    strindex(pattern, "-]") >= 0 ||
		    pattern[0] == '*')	/* grep bug */
			grepold++;
		else
			pattern = grepxlat(pattern);
	}
	if (grepold || egrepold || fgrepold)
		kernighan(argv);

	if (iflag)
		strcpy(pattern, fold(pattern));
	/*
	 * If the pattern is a plain string, just run boyer-moore. If it
	 * consists of meta-free alternatives, run "superimposed" bmg.
	 * Otherwise, find best string, and compile pattern for regexec(). 
	 */
	if (strpbrk(pattern, META) == NULL) {	/* do boyer-moore only */
		boyonly++;
		patboy = pattern;
	} else {
		if ((patboy = alternate(pattern)) != NULL)
			boyonly++;
		else {
			if ((patboy = isolate(pattern)) == NULL)
				kernighan(argv);	/* expr too involved */
#ifndef NOKANJI
			for (c = 0; pattern[c] != EOS; c++)
				if (pattern[c] & NONASCII)	/* kanji + meta */
					kernighan(argv);
#endif
			if ((rspencer = regcomp(pattern)) == NULL)
				oops("regcomp failure");
		}
	}
	gosper(patboy);		/* "pre-conditioning is wonderful"
				 * -- v. strassen */

	if ((firstfile = optind) >= argc) {
		/* Grep standard input */
		if (lflag) {	/* We don't know its name! */
#if WIN_NT
			if (ppid == (pid_t) 1 && globulation == 0)
				deglobulate();
#endif
			exit(1);
		}
		egsecute((char *) NULL);
	} else {
		while (optind < argc) {
			egsecute(argv[optind]);
			optind++;
			if (firstflag && (nsuccess == 1))
				break;
		}
	}
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	return (nsuccess == 2) ? 2 : (nsuccess == 0);
}

char *
#if __STDC__
pfile (const char *pfname)
#else
pfile(pfname)			/* absorb expression from file */
	char *pfname;
#endif
{
	FILE *pf;
	struct stat patstat;
	static char *pat;

	if ((pf = fopen(pfname, "r")) == NULL)
		oops("can't read pattern file");
	if (fstat(fileno(pf), &patstat) != 0)
		oops("can't stat pattern file");
	if (patstat.st_size > PATSIZE) {
		if (fgrepflag) {	/* defer to unix version */
			fgrepold++;
			return "dummy";
		} else
			oops("pattern file too big");
	}
#if __STDC__
	if ((pat = malloc((size_t) patstat.st_size + 1)) == NULL)
		oops("out of memory to read pattern file");
	if (patstat.st_size !=
	    (off_t) fread(pat, sizeof(char), (size_t) patstat.st_size + 1, pf))
		oops("error reading pattern file");
#else
	if ((pat = malloc((unsigned) patstat.st_size + 1)) == NULL)
		oops("out of memory to read pattern file");
	if (patstat.st_size !=
	    fread(pat, sizeof(char), patstat.st_size + 1, pf))
		oops("error reading pattern file");
#endif
	(void) fclose(pf);

	pat[patstat.st_size] = EOS;
	if (pat[patstat.st_size - 1] == NL)	/* NOP for egrep; helps grep */
		pat[patstat.st_size - 1] = EOS;

	if (nlcount(pat, &pat[patstat.st_size]) > NALT) {
		if (fgrepflag)
			fgrepold++;	/* "what's it all about, alfie?" */
		else
			egrepold++;
	}
	return (pat);
}

void
#if __STDC__
egsecute (const char *file)
#else
egsecute(file)
	char *file;
#endif
{
	if (file == NULL)
		fd = 0;
#ifndef O_RDONLY
# define O_RDONLY 0
#endif
	else if ((fd = open(file, O_RDONLY)) <= 0) {
		fprintf(stderr, "%s: can't open %s\n", progname, file);
		nsuccess = 2;
		return;
	}
	chimaera(file, patboy);

	if (!boyonly && !flushflag && file != NULL)
		flushmatches();
	if (file != NULL)
		close(fd);
}

void
#if __STDC__
chimaera (const char *file, char *pat)
#else
chimaera(file, pat)		/* "reach out and boyer-moore search someone" */
	char *file, *pat;	/* -- soon-to-be-popular bumper sticker */
#endif
{
	register char *k, *strend, *s;
	register int j, count;
	register int *deltazero = delta0;
	int patlen = altmin;
	char *t;
#if !__STDC__
	char *gotamatch(), *kanji(), *linesave(), *submatch();
#endif

	nleftover = boyfound = flushflag = 0;
	nline = 1L;
	prevmatch = 0;
	nmatch = counted = rxcount = 0L;

	while ((count = (int) read(fd, str + nleftover, BUFSIZE - nleftover)) > 0) {

		counted += count;
		strend = linesave(str, count);

		for (k = str + patlen - 1; k < strend;) {
			/*
			 * for a large class of patterns, upwards of 80% of
			 * match time is spent on the next line.  we beat
			 * existing microcode (vax 'matchc') this way. 
			 */
			while ((k += deltazero[*(unsigned char *) k]) < strend);
			if (k < (str + LARGE))
				break;
			k -= LARGE;

			if (altflag) {
				/*
				 * Parallel Boyer-Moore.  Check whether each
				 * of the previous <altmin> chars COULD be
				 * from one of the alternative strings. 
				 */
				s = k - 1;
				j = altmin;
				while (altset[--j][(unsigned char)
					     cmap[*(unsigned char *) s--]]);
				/*
				 * quick test fails. in this life, compare
				 * 'em all.  but, a "reverse trie" would
				 * attenuate worst case (linear w/delta2?). 
				 */
				if (--j < 0) {
					count = nalt - 1;
					do {
						s = k;
						j = altlen[count];
						t = altpat[count];

						while
							(cmap[*(unsigned char *) s--]
							 == t[--j]);
						if (j < 0)
							break;
					}
					while (count--);
				}
			} else {
				/* One string -- check it */
				j = patlen - 1;
				s = k - 1;
				while (cmap[*(unsigned char *) s--] == pat[--j]);
			}
			/*
			 * delta-less shortcut for literati. short shrift for
			 * genetic engineers? 
			 */
			if (j >= 0) {
				k++;	/* no match; restart next char */
				continue;
			}
			k = submatch(file, pat, str, strend, k, count);
			if (k == NULL)
				return;
		}
		if (nflag) {
			if (prevmatch)
				nline = prevnline + nlcount(prevloc, k);
			else
				nline = nline + nlcount(str, k);
			prevmatch = 0;
		}
		strncpy(str, linetemp, nleftover);
	}
	if (cflag) {
		/* Bug from old grep: -c overrides -h.  We fix the bug. */
		if (!hflag)
			printf("%s:", file);
		printf("%ld\n", nmatch);
	}
}

char *
#if __STDC__
linesave (char *str, register int count)
#else
linesave(str, count)		/* accumulate partial line at end of buffer */
	char str[];
	register int count;
#endif
{
	register int j;

	count += nleftover;
	if (count != BUFSIZE && fd != 0)
		str[count++] = NL;	/* insurance for broken last line */
	str[count] = EOS;
	for (j = count - 1; str[j] != NL && j >= 0;)
		j--;
	/*
	 * break up these lines: long line (> BUFSIZE), last line of file, or
	 * short return from read(), as from tee(1) input 
	 */
	if (j < 0 && (count == (BUFSIZE - nleftover))) {
		str[count++] = NL;
		str[count] = EOS;
		linetemp[0] = EOS;
		nleftover = 0;
		return (str + count);
	} else {
		nleftover = count - j - 1;
		strncpy(linetemp, str + j + 1, nleftover);
		return (str + j);
	}
}

/*
 * Process partial match. First check for mis-aligned Kanji, then match line
 * against full compiled r.e. if statistics do not warrant handing off to
 * standard egrep. 
 */
char *
#if __STDC__
submatch (const char *file, char *pat, const char *str,
	register const char *strend, register char *k, int altindex)
#else
submatch(file, pat, str, strend, k, altindex)
	char file[], pat[], str[];
	register char *strend, *k;
	int altindex;
#endif
{
	register char *s;
	char *t, c;

	t = k;
	s = ((altflag) ? k - altlen[altindex] + 1 : k - altmin + 1);
#ifndef NOKANJI
# if WIN_NT
	if (altflag)
		c = altpat[altindex][0];
	else
		c = pat[0];
# else
	c = ((altflag) ? altpat[altindex][0] : pat[0]);
# endif
	if (c & NONASCII)
		if ((s = kanji(str, s, k)) == NULL)
			return (++k);	/* reject false kanji */
#endif
	do;
	while (*s != NL && --s >= str);
	k = s + 1;		/* now at line start */

	if (boyonly)
		return (gotamatch(file, k));

	incount = counted - (strend - k);
	if (boyfound++ == FIRSTFEW)
		execstrategy(file);

	s = t;
	do
		rxcount++;
	while (*s++ != NL);
	*--s = EOS;
	/*
	 * "quick henry -- the flit" (after theodor geisel) 
	 */
	if (regexec(rspencer, ((iflag) ? fold(k) : k)) == 1) {
		*s = NL;
		if (gotamatch(file, k) == NULL)
			return (NULL);
	}
	*s = NL;
	return (s + 1);
}

/*
 * EUC code disambiguation -- scan backwards to first 7-bit code, while
 * counting intervening 8-bit codes.  If odd, reject unaligned Kanji pattern. 
 * SS2/3 checks are for intermixed Japanase Katakana or Kanji2. 
 */
char *
#if __STDC__
kanji (register const char *str, register const char *s, register char *k)
#else
kanji(str, s, k)
	register char *str, *s, *k;
#endif
{
	register int j = 0;

	for (s--; s >= str; s--) {
		if (*s == SS2 || *s == SS3 || (*s & NONASCII) == 0)
			break;
		j++;
	}
#ifndef CHINESE
	if (*s == SS2)
		j -= 1;
#endif /* CHINESE */
	return ((j & 01) ? NULL : k);
}

/*
 * Compute "Boyer-Moore" delta table -- put skip distance in delta0[c] 
 */
void
#if __STDC__
gosper (const char *pattern)
#else
gosper(pattern)
	char *pattern;		/* ... HAKMEM lives ... */
#endif
{
	register int i, j;
	unsigned char c;

	/* Make one-string case look like simple alternatives case */
	if (!altflag) {
		nalt = 1;
		altmin = altlen[0] = strlen(pattern);
		strcpy(altpat[0], pattern);
	}
	/* For chars that aren't in any string, skip by string length. */
	for (j = 0; j < 256; j++) {
		delta0[j] = altmin;
		cmap[j] = (char) j;	/* Sneak in initialization of cmap */
	}

	/* For chars in a string, skip distance from char to end of string. */
	/* (If char appears more than once, skip minimum distance.) */
	for (i = 0; i < nalt; i++)
		for (j = 0; j < altlen[i] - 1; j++) {
			c = altpat[i][j];
			delta0[c] = MIN(delta0[c], altlen[i] - j - 1);
			if (iflag && islower((int) c))
				delta0[toupper((int) c)] = delta0[c];
		}

	/* For last char of each string, fall out of search loop. */
	for (i = 0; i < nalt; i++) {
		c = altpat[i][altlen[i] - 1];
		delta0[c] = LARGE;
		if (iflag && islower((int) c))
			delta0[toupper((int) c)] = LARGE;
	}
	if (iflag)
		for (j = 'A'; j <= 'Z'; j++)
			cmap[j] = (char) tolower((int) j);
}

/*
 * Print, count, or stop on full match. Result is either the location for
 * continued search, or NULL to stop. 
 */
char *
#if __STDC__
gotamatch (register const char *file, register char *s)
#else
gotamatch(file, s)
	register char *file, *s;
#endif
{
#if !__STDC__
	char *savematch();
#endif
	int squirrel = 0;	/* nonzero to squirrel away FIRSTFEW matches */

	nmatch++;
	nsuccess = 1;
	if (!boyonly && boyfound <= FIRSTFEW && file != NULL)
		squirrel = 1;

	if (sflag)
		return (NULL);	/* -s usurps all flags (unlike some versions) */
	if (cflag) {		/* -c overrides -l, we guess */
		do;
		while (*s++ != NL);
	} else if (lflag) {
		puts(file);
		return (NULL);
	} else {
		if (!hflag)
			if (!squirrel)
				printf("%s:", file);
			else
				sprintf(preamble, "%s:", file);
		if (nflag) {
			if (prevmatch)
				prevnline = prevnline + nlcount(prevloc, s);
			else
				prevnline = nline + nlcount(str, s);
			prevmatch = 1;

			if (!squirrel)
				printf("%ld:", prevnline);
			else
				sprintf(preamble + strlen(preamble),
					"%ld:", prevnline);
		}
		if (!squirrel) {
			do
				putchar(*s);
			while (*s++ != NL);
		} else
			s = savematch(s);

		if (nflag)
			prevloc = s - 1;
	}
	return ((firstflag && !cflag) ? NULL : s);
}

char *
#if __STDC__
fold (const char *line)
#else
fold(line)
	char *line;
#endif
{
	static char fline[BUFSIZE];
#if __STDC__
	register const char *s;
	register char *t = fline;
#else
	register char *s, *t = fline;
#endif

	for (s = line; *s != EOS; s++)
#if WIN_NT
		if (isupper((int) *s))
			*t++ = (char) tolower((int) *s);
		else
			*t++ = *s;
#else
		*t++ = (isupper((int) *s) ? (char) tolower((int) *s) : *s);
#endif
	*t = EOS;
	return (fline);
}

int
#if __STDC__
strindex (const char *s, const char *t)
#else
strindex(s, t)			/* the easy way, as in K&P, p. 192 */
	char *s, *t;
#endif
{
	int i, n;

	n = strlen(t);
	for (i = 0; s[i] != '\0'; i++)
		if (strncmp(s + i, t, n) == 0)
			return (i);
	return (-1);
}

char *
#if __STDC__
grepxlat (const char *pattern)		/* grep pattern meta conversion */
#else
grepxlat(pattern)		/* grep pattern meta conversion */
	char *pattern;
#endif
{
#if __STDC__
	register const char *p;
	register char *s;
#else
	register char *p, *s;
#endif
	static char newpat[BUFSIZE];

	for (s = newpat, p = pattern; *p != EOS;) {
		if (*p == '\\') {	/* skip escapes ... */
			*s++ = *p++;
			if (*p)
				*s++ = *p++;
		} else if (*p == '[') {	/* ... and char classes */
			while (*p != EOS && *p != ']')
				*s++ = *p++;
		} else if (strchr("+?|()", *p) != NULL) {
			*s++ = '\\';	/* insert protection */
			*s++ = *p++;
		} else
			*s++ = *p++;
	}
	*s = EOS;
	return (newpat);
}

/*
 * Test for simple alternation.  Result is NULL if it's not so simple, or is
 * a pointer to the first string if it is. Warning:  sscanf size is a
 * fixpoint, beyond which the speedup linearity starts to break down.  In the
 * wake of the elegant aho/corrasick "trie"-based fgrep, generalizing
 * altpat[] to arbitrary size is not useful. 
 */
char *
#if __STDC__
alternate (char *regexpr)
#else
alternate(regexpr)
	char *regexpr;
#endif
{
	register i, j, min;
	unsigned char c;
	char oflow[100];

	if (fgrepflag && strchr(regexpr, '|'))
		return (NULL);

	i = strlen(regexpr);
	for (j = 0; j < i; j++)
		if (regexpr[j] == NL)
			regexpr[j] = '|';

	if (!fgrepflag) {
		if (strchr(regexpr, '|') == NULL || regexpr[0] == '|')
			return (NULL);
		if (strpbrk(regexpr, "^$.[]()?+*\\") != NULL
		    || strindex(regexpr, "||") >= 0)
			return (NULL);
	}
	oflow[0] = EOS;
	nalt = sscanf(regexpr, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]",
		      altpat[0], altpat[1], altpat[2], altpat[3], altpat[4], altpat[5], altpat[6], oflow);

	if (nalt < 1 || oflow[0] != EOS)
		return (NULL);

	altmin = NMUSH;
	for (j = 0; j < nalt; j++) {
		min = altlen[j] = strlen(altpat[j]);
		if (min > ALTSIZE)
			return (NULL);
		altmin = MIN(altmin, min);
	}
	if (nalt > 1) {		/* build superimposed "pre-match" sets per
				 * char */
		altflag++;
		for (j = 0; j < nalt; j++)
			for (i = 0; i < altmin; i++) {
				c = altpat[j][altlen[j] - altmin + i];
				altset[i + 1][c] = 1;	/* offset for sentinel */
			}
	}
	return (altpat[0]);
}

/*
 * Grapple with the dfa (std egrep) vs. ndfa (regexp) tradeoff. Criteria to
 * determine whether to use dfa-based egrep:  We do FIRSTFEW matches with
 * regexec().  Otherwise, if Boyer-Moore up to now matched more than
 * PUNTPERCENT of the input, and there is sufficient bulk remaining to
 * justify justify a process exec, do old *grep, presuming that its greater
 * speed at regular expressions will pay us back over this volume.  At
 * FIRSTFEW, dump the saved matches collected by savematch(). They are saved
 * so that a "PUNT" can "rewind" to ignore them.  Stdin is problematic,
 * since it's hard to rewind. 
 */

#define CTHRESH	50000

void
#if __STDC__
execstrategy (const char *file)
#else
execstrategy(file)
	char *file;
#endif
{
	struct stat stbuf;
	int pctmatch;
	long cremain;

	pctmatch = (int) ((100 * rxcount) / incount);
	if (!grepflag && pctmatch > PUNTPERCENT && file != NULL) {
		fstat(fd, &stbuf);
		cremain = stbuf.st_size - incount;
		if (cremain > CTHRESH)
			kernighan(args);
	}
	if (file != NULL)
		flushmatches();
}

int
#if __STDC__
nlcount (const char *bstart, const char *bstop)		/* flail interval to totalize newlines. */
#else
nlcount(bstart, bstop)		/* flail interval to totalize newlines. */
	char *bstart, *bstop;
#endif
{
#if __STDC__
	register const char *s = bstart;
	register const char *t = bstop;
#else
	register char *s = bstart;
	register char *t = bstop;
#endif
	register int count = 0;

	do {			/* loop unroll for older architectures */
		if (*t == NL)	/* ... ask ames!jaw for sample code */
			count++;
	} while (t-- > s);

	return (count);
}

char *
#if __STDC__
isolate (const char *regexpr)		/* isolate longest metacharacter-free string */
#else
isolate(regexpr)		/* isolate longest metacharacter-free string */
	char *regexpr;
#endif
{
	char *dummyexpr;

	/*
	 * We add (.)* because Henry's regcomp only figures regmust if it
	 * sees a leading * pattern.  Foo! 
	 */
#if __STDC__
	dummyexpr = malloc((size_t) strlen(regexpr) + 5);
#else
	dummyexpr = malloc((unsigned) strlen(regexpr) + 5);
#endif
	sprintf(dummyexpr, "(.)*%s", regexpr);
	if ((rspencer = regcomp(dummyexpr)) == NULL)
		kernighan(args);
	return (rspencer->regmust);
}

char *matches[FIRSTFEW];
static int mcount = 0;

char *
#if __STDC__
savematch (register char *s)			/* horde matches during statistics gathering */
#else
savematch(s)			/* horde matches during statistics gathering */
	register char *s;
#endif
{
	char *p;
	char *start = s;
	int msize = 0;
	int psize = strlen(preamble);

	while (*s++ != NL)
		msize++;
	*--s = EOS;

#if __STDC__
	p = malloc((size_t) msize + 1 + psize);
#else
	p = malloc((unsigned) msize + 1 + psize);
#endif
	strcpy(p, preamble);
	strcpy(p + psize, start);
	matches[mcount++] = p;

	preamble[0] = 0;
	*s = NL;
	return (s);
}

void
#if __STDC__
flushmatches (void)
#else
flushmatches()
#endif
{
	int n;

	flushflag = 1;
	for (n = 0; n < mcount; n++)
		printf("%s\n", matches[n]);
	mcount = 0;
}

void
#if __STDC__
oops (const char *message)
#else
oops(message)
	char *message;
#endif
{
	static const char usage[] = "usage: ";

	if (strncmp(message, usage, sizeof usage - 1) == 0)
		fprintf(stderr, "%s\n", message);
	else
		fprintf(stderr, "%s: %s\n", progname, message);
#if WIN_NT
	if (ppid == (pid_t) 1 && globulation == 0)
		deglobulate();
#endif
	exit((optopt == '?') ? 0 : 2);
}

void
#if __STDC__
kernighan (char **args)			/* "let others do the hard part ..." */
#else
kernighan(args)			/* "let others do the hard part ..." */
	char *args[];
#endif
{
	/*
	 * We may have already run grep on some of the files; remove them
	 * from the arg list we pass on.  Note that we can't delete them
	 * totally because the number of file names affects the output
	 * (automatic -h). 
	 */
	/* better would be fork/exec per punted file -- jaw */

	while (firstfile && optind > firstfile)
		args[firstfile++] = "/dev/null";

	fflush(stdout);

	if (grepflag)
		execvp(GREPSTD, args), oops("can't exec old 'grep'");
	else if (fgrepflag)
		execvp(FGREPSTD, args), oops("can't exec old 'fgrep'");
	else
		execvp(EGREPSTD, args), oops("can't exec old 'egrep'");
}
