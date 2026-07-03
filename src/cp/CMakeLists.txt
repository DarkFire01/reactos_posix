# cp -- reskit POSIX 'cp', sources copied in-tree from posixsrc/CP.
add_posix_app(cp
    cp.c
    cpglob.c
    glob.c
    path.c
    )

target_compile_definitions(posix_cp PRIVATE MAXBSIZE=65536)
