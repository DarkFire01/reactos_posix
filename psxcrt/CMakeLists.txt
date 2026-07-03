# libpsxcrt -- the POSIX C runtime for NT-4.0-style POSIX executables.
#
# Hand-written: crt0 (__PosixProcessStartup entry), the C heap over psxdll's
# Heap* exports, the buffered-stdio engine (_iob/_filbuf/_flsbuf/fopen/...), and
# the small stdlib pieces (getenv/atoi/qsort/...). Compiled from the reskit
# (public-domain) SH/STD/STDC sources: the string/mem family + the printf
# formatter. Every from-source POSIX app links this static lib; it resolves
# against the slipstreamed real psxdll.dll at runtime (Heap*, _exit, read/write).

list(APPEND SOURCE
    crt0.c
    heap.c
    stdio.c
    misc.c
    ctype.c
    setjmp.c
    getopt.c
    err.c
    warn.c
    timeconv.c
    lstat.c
    # reskit string / memory (public domain, compiled as-is). Lowercase .c so MSVC
    # compiles them as C (the K&R definitions are rejected by the C++ front end that
    # the uppercase .C extension selects).
    strlen.c strcpy.c strncpy.c strcat.c strncat.c strcmp.c strncmp.c
    strchr.c strrchr.c strstr.c strpbrk.c strspn.c strcspn.c strtok.c
    memcpy.c memmove.c memset.c memcmp.c memchr.c
    # reskit stdio formatter (printf/fprintf/vprintf/vfprintf/sprintf) + our sscanf
    vprintf.c fprintf.c sprintf.c scanf.c
    # BSD support layer (FTS tree-walker, fnmatch, + BSDPSX helpers) for the
    # BSD-derived utilities: ls / cp / rm / find / grep / vi.
    fts.c fnmatch.c strmode.c setmode.c snprintf.c
    getwd.c index.c rindex.c bcopy.c bzero.c pwcache.c
    utimes.c mknod.c scandir.c)

add_library(psxcrt ${SOURCE})

# _BSD selects the reskit STDC's BSD variant -- notably the printf/fprintf/sprintf
# definitions are gated behind '#if _V7 || _BSD'.
target_compile_definitions(psxcrt PRIVATE
    WIN_NT STDC_HEADERS _POSIX_SOURCE DIRENT __STDC__ _POSIX_ _BSD)

# Same POSIX header environment as the apps: our psx headers + the reskit CRT
# headers here win over the Win32 crt/psdk; force-include sys/types.h for pid_t.
target_compile_options(psxcrt PRIVATE /WX- /FIsys/types.h)
target_include_directories(psxcrt BEFORE PRIVATE
    ${REACTOS_SOURCE_DIR}/sdk/include/psx
    ${CMAKE_CURRENT_SOURCE_DIR})

# psxcrt calls into psxdll (Heap*, _exit, read/write/open/close/lseek); the apps
# that link psxcrt already import psxdll, so no import is needed on the lib itself.
