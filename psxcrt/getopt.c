/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem  --  LICENSE: MIT
 * PURPOSE:     getopt(3) for the POSIX userland (option parsing). Standard
 *              single-char getopt with optarg/optind/opterr/optopt.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */
#include <stdio.h>
extern char *strchr(const char *, int);
extern int   strcmp(const char *, const char *);

int   opterr = 1;
int   optind = 1;
int   optopt = 0;
char *optarg = 0;

int getopt(int argc, char *const argv[], const char *optstring)
{
    static int optpos = 1;
    const char *oli;

    if (optpos == 1)
    {
        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
            return -1;
        if (strcmp(argv[optind], "--") == 0) { optind++; return -1; }
    }

    optopt = argv[optind][optpos];
    oli = strchr(optstring, optopt);
    if (optopt == ':' || oli == 0)
    {
        if (opterr) fprintf(stderr, "%s: illegal option -- %c\n", argv[0], optopt);
        if (argv[optind][++optpos] == '\0') { optind++; optpos = 1; }
        return '?';
    }

    if (oli[1] == ':')      /* option takes an argument */
    {
        if (argv[optind][optpos + 1] != '\0')
            optarg = (char *)&argv[optind][optpos + 1];
        else if (++optind < argc)
            optarg = argv[optind];
        else
        {
            optpos = 1;
            if (opterr) fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], optopt);
            return (optstring[0] == ':') ? ':' : '?';
        }
        optind++;
        optpos = 1;
    }
    else if (argv[optind][++optpos] == '\0')
    {
        optind++;
        optpos = 1;
    }
    return optopt;
}
