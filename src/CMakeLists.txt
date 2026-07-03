# POSIX userland (reskit BSD/PD utilities) -- sources copied in-tree,
# built against psxdll, slipstreamed to /bin.
include(${CMAKE_CURRENT_SOURCE_DIR}/posixapp.cmake)

add_subdirectory(sh)
add_subdirectory(ls)
add_subdirectory(cp)
add_subdirectory(mkdir)
add_subdirectory(rmdir)
add_subdirectory(rm)
add_subdirectory(cat)
add_subdirectory(touch)
add_subdirectory(grep)


# WIP -- not yet building (excluded from the ISO for now):
#   find : DOS findfirst (struct ffblk) in wildcard.c, needs <sys/mount.h>, its
#          own in-tree fnmatch.c, getopt externs, BLOCK_SIZE.
#   vi   : elvis -- pervasive struct-stat/_win_st confusion, <sgtty.h>, curses.
# add_subdirectory(find)
# add_subdirectory(vi)
