# ls -- reskit POSIX 'ls', sources copied in-tree from posixsrc/LS.
add_posix_app(ls
    bstat.c
    cmp.c
    glob.c
    ls.c
    lsglob.c
    print.c
    util.c
    )

# BSD ls reports sizes in 512-byte blocks (usually from <sys/param.h>).
target_compile_definitions(posix_ls PRIVATE BLOCK_SIZE=512)
