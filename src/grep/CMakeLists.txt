# grep -- reskit POSIX grep. EGREP.C is the shared main (grep/egrep/fgrep built
# from it via defines); grep.c has no main; timer.c/try.c are aux/test drivers.
add_posix_app(grep
    egrep.c
    grep.c
    glob.c
    getopt.c
    regerror.c
    regexp.c
    regsub.c
    )
