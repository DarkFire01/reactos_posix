# sh -- the NT Resource Kit POSIX shell (pdksh/MKS lineage), built for the POSIX
# subsystem (IMAGE_SUBSYSTEM_POSIX_CUI = 7) and linked against the psxdll import
# library. At runtime it binds by name to the slipstreamed psxdll.dll.
#
# Sources copied in-tree from posixsrc/SH/SH (including the SH.C dos_to_posix/
# posix_to_dos malloc(strlen()+1) off-by-one fix).  Referenced here with the
# lowercase '.c' spelling so MSVC's C (not C++) front end compiles the K&R code;
# the on-disk names keep the reskit's UPPERCASE.C form (same trick as src/ls).
# The shared reskit CRT/BSD headers (STDIO.H with the fileno/_fileno lvalue
# macros, STDARG.H, STDDEF.H, ...) come from ../../psxcrt, same as every other
# util (they replace the old external posixsrc/SH/STD/H reference; our
# sdk/include/psx headers still win first for fcntl/limits/stdlib/time/unistd).

list(APPEND SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/alloc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/c_ksh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/c_sh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/c_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/edit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/emacs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eval.c
    ${CMAKE_CURRENT_SOURCE_DIR}/exec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/getopts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/history.c
    ${CMAKE_CURRENT_SOURCE_DIR}/io.c
    ${CMAKE_CURRENT_SOURCE_DIR}/jobs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mail.c
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/misc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/trace.c
    ${CMAKE_CURRENT_SOURCE_DIR}/trap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tree.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ulimit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/var.c
    ${CMAKE_CURRENT_SOURCE_DIR}/version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/vi.c)

# Faithful to the reskit SH MAKEFILE's compile flags. (KSH/EMACS/VI/JOBS are
# selected by config.h -- do NOT redefine them on the command line or /WX
# turns the C4005 redefinition warning into an error.)
add_definitions(
    -DWIN_NT -DSTDC_HEADERS -D_POSIX_SOURCE -DDIRENT
    -DSTACK_DIRECTION=-1 -D__STDC__ -D_POSIX_)

add_executable(sh ${SOURCE})

# This is 1990s reskit C: it trips many W3 warnings (implicit decls, missing
# returns, int/ptr). Don't let /WX turn them into hard errors while porting.
target_compile_options(sh PRIVATE /WX-)

# sh.h declares 'pid_t kshpid' but neither it nor config.h pulls <sys/types.h>;
# force-include our POSIX sys/types.h first in every file so pid_t/uid_t/off_t
# are defined before sh.h is parsed (resolves to sdk/include/psx, first on -I).
target_compile_options(sh PRIVATE /FIsys/types.h)

# POSIX-app include environment: our POSIX SDK headers and sh's own headers MUST
# win over ReactOS's Win32 CRT/PSDK copies (which the module build injects), so
# <unistd.h>/<sys/types.h>/<signal.h>/<fcntl.h> resolve to the POSIX versions.
# BEFORE prepends these ahead of the global Win32 include dirs.
# POSIX_CRT_INCLUDE is set by ../posixapp.cmake (included by src/CMakeLists.txt).
target_include_directories(sh BEFORE PRIVATE
    ${REACTOS_SOURCE_DIR}/sdk/include/psx
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${POSIX_CRT_INCLUDE})

# A POSIX-subsystem image; entry = psxcrt's crt0 (__PosixProcessStartup ->
# __PdxInitializeData -> __PdxGetCmdLine argv/env unpack -> main -> exit).
set_module_type(sh module)
set_subsystem(sh posix)
set_entrypoint(sh __PosixProcessStartup)

# The POSIX C runtime (crt0 + libc) then the psxdll import library (ordinal-1:1
# with the real psxdll.dll). psxcrt supplies __PosixProcessStartup, malloc,
# printf, ...; psxdll supplies the syscalls + Heap*/_exit it calls.
target_link_libraries(sh psxcrt)
add_importlibs(sh psxdll)

# Slipstreamed to the CD /bin (both lowercase 'sh' and, via mklinks, the uppercase
# name the userland expects). Runs against the slipstreamed psxdll.dll on the ISO.
add_cd_file(TARGET sh DESTINATION reactos/bin FOR all)
