# Shared helper for the reskit POSIX userland (public-domain / BSD tools). All
# sources are copied in-tree under each app's directory -- no external references.
#
#   add_posix_app(<name> <src1.c> <src2.c> ...)   # sources local to the app dir
#
# Each app: subsystem 7 (IMAGE_SUBSYSTEM_POSIX_CUI), our sdk/include/psx POSIX
# headers winning over the Win32 crt/psdk, the shared reskit CRT/BSD headers that
# live with psxcrt, links the psxdll import library (binds by name to the
# slipstreamed psxdll.dll at runtime), and is slipstreamed to the CD /bin.
# /FIsys/types.h force-defines pid_t/off_t before the reskit headers; /WX-
# silences 1990s warnings.

# Shared in-tree CRT/BSD headers (copied from posixsrc/SH/STD/H + include/{bsd,df});
# they live with the psxcrt runtime sources one level up.
set(POSIX_CRT_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/../psxcrt)

function(add_posix_app _name)
    # CMake target names must be globally unique, and several POSIX tool names
    # (find, grep, ...) collide with ReactOS's Win32 commands. Use a distinct
    # 'posix_<name>' TARGET but emit the plain '<name>.exe' so the binary keeps its
    # POSIX name.  Build one with e.g. `ninja posix_find`.
    set(_target posix_${_name})

    set(_srcs "")
    foreach(_f ${ARGN})
        list(APPEND _srcs ${CMAKE_CURRENT_SOURCE_DIR}/${_f})
    endforeach()

    add_executable(${_target} ${_srcs})
    set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_name})

    target_compile_definitions(${_target} PRIVATE
        WIN_NT STDC_HEADERS _POSIX_SOURCE DIRENT
        STACK_DIRECTION=-1 __STDC__ _POSIX_)

    # Force-include sys/types.h (pid_t/off_t) and sys/cdefs.h (the BSD __P((...))
    # prototype macro) before the reskit headers -- many use them without including
    # the header that defines them.
    target_compile_options(${_target} PRIVATE /WX- /FIsys/types.h /FIsys/cdefs.h)

    target_include_directories(${_target} BEFORE PRIVATE
        ${REACTOS_SOURCE_DIR}/sdk/include/psx
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${POSIX_CRT_INCLUDE})

    set_module_type(${_target} module)
    set_subsystem(${_target} posix)
    set_entrypoint(${_target} __PosixProcessStartup)
    target_link_libraries(${_target} psxcrt)    # POSIX crt0 + libc
    add_importlibs(${_target} psxdll)

    add_cd_file(TARGET ${_target} DESTINATION reactos/bin FOR all)
endfunction()
