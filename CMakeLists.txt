# ReactOS sets CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON, which hoists include
# dirs living inside the reactos source/binary tree to the front of every -I list.
# This repo's dirs are OUTSIDE that tree, so with it ON our BEFORE-prepended POSIX
# include dirs (psxcrt headers, xlib6/motif headers) sink BEHIND the global Win32
# crt/psdk includes and the wrong <stdio.h>/<wchar.h>/... win.  Turn it off for
# everything defined in this subtree so target include order is honored as written.
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE OFF)

# im too lazy to fix gcc
if(MSVC)
    add_subdirectory(psxcrt)      # static lib 'psxcrt' (crt0 + libc)
    add_subdirectory(src)         # the userland utilities -> reactos/bin
    add_subdirectory(lib/xlib6)   # libX11r6/Xt/Xm + ico/xeyes/dthello/hellomotif
endif()

# ---- runtime data -> the CD -----------------------------------------------------------
# Same contract as the old reactos/modules/{bin,etc,x86} slipstream blocks: after
# adding or removing files under bin/ or etc/, re-run `cmake .` in the build dir
# so the glob is re-evaluated, then `ninja bootcd`.

# etc/** -> CD-root /etc (the POSIX view's /etc): profile, termcap, the PSX
# launchers, and the X11 runtime data (locale DB, XKeysymDB).
file(GLOB_RECURSE _PSX_ETC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/etc ${CMAKE_CURRENT_SOURCE_DIR}/etc/*)
foreach(item ${_PSX_ETC_FILES})
    add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/etc/${item} DESTINATION etc NO_CAB NAME_ON_CD ${item} FOR all)
endforeach()

# bin/** -> CD-root /bin (prebuilt POSIX binaries) -- EXCEPT posix.exe: the real,
# unmodified MS NT 4.0 session leader must land in %SystemRoot%\system32 next to
# psxss.exe (the registry registration in hivesys.inf names the subsystem there,
# and psxss launches posix.exe per session).
file(GLOB_RECURSE _PSX_BIN_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/bin ${CMAKE_CURRENT_SOURCE_DIR}/bin/*)
foreach(item ${_PSX_BIN_FILES})
    if(item STREQUAL "posix.exe")
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bin/${item} DESTINATION reactos/system32 NO_CAB NAME_ON_CD ${item} FOR all)
    else()
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bin/${item} DESTINATION bin NO_CAB NAME_ON_CD ${item} FOR all)
    endif()
endforeach()
