/*
	getopt.c

	modified public-domain AT&T getopt(3)
*/

#include <stdio.h>
#include <string.h>

#ifdef _POSIX_SOURCE
# include <unistd.h>
#else
# define STDERR_FILENO 2
typedef signed int ssize_t;
# ifdef __STDC__
extern ssize_t write (int __fildes, const void *__buf, size_t __nbyte);
# else
extern ssize_t write ();
# endif
#endif

int opterr = 1;
int optind = 1;
int optopt;
char *optarg;

#ifdef __STDC__
static void ERR (char **argv, char *s, char c)
#else
static void ERR (argv, s, c)
char **argv, *s, c;
#endif
{
	char errbuf[2];

	if (opterr)
	{
		errbuf[0] = c;
		errbuf[1] = '\n';
		(void) write(STDERR_FILENO, argv[0], strlen(argv[0]));
		(void) write(STDERR_FILENO, s, strlen(s));
		(void) write(STDERR_FILENO, errbuf, sizeof errbuf);
	}
}

#ifdef __STDC__
int getopt (int argc, char **argv, const char *opts)
#else
int getopt (argc, argv, opts)
int argc;
char **argv, *opts;
#endif
{
	static int sp = 1, error = (int) '?';
	static char sw = '-', eos = '\0', arg = ':';
	register char c, *cp;

	if (sp == 1)
		if (optind >= argc || argv[optind][0] != sw
		|| argv[optind][1] == eos)
			return EOF;
		else if (strcmp(argv[optind], "--") == 0)
		{
			optind++;
			return EOF;
		}
	c = argv[optind][sp];
	optopt = (int) c;
	if (c == arg || (cp = strchr(opts, c)) == NULL)
	{
		ERR(argv, ": illegal option - ", c);
		if (argv[optind][++sp] == eos)
		{
			optind++;
			sp = 1;
		}
		return error;
	}
	else if (*++cp == arg)
	{
		if (argv[optind][sp + 1] != eos)
			optarg = &argv[optind++][sp + 1];
		else if (++optind >= argc)
		{
			ERR(argv, ": option requires an argument - ", c);
			sp = 1;
			return error;
		}
		else
			optarg = argv[optind++];
		sp = 1;
	}
	else
	{
		if (argv[optind][++sp] == eos)
		{
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return (int) c;
}
