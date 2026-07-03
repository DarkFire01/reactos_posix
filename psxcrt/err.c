/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem  --  LICENSE: MIT
 * PURPOSE:     perror + the sys_errlist/sys_nerr tables + strerror for the
 *              userland. NOTE: the BSD err/warn family lives in a SEPARATE object
 *              (warn.c) on purpose -- several reskit utilities (cat/cp/ls/rm)
 *              define their own err(), so keeping err/warn out of this object lets
 *              a util pull perror/strerror without dragging in a colliding err().
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */
extern int  errno;
extern int  write(int, const void *, unsigned);
extern unsigned long strlen(const char *);

char *sys_errlist[] = {
    "Success", "Operation not permitted", "No such file or directory",
    "No such process", "Interrupted system call", "Input/output error",
    "No such device or address", "Argument list too long", "Exec format error",
    "Bad file descriptor", "No child processes", "Resource temporarily unavailable",
    "Cannot allocate memory", "Permission denied", "Bad address",
    "Block device required", "Device or resource busy", "File exists",
    "Invalid cross-device link", "No such device", "Not a directory",
    "Is a directory", "Invalid argument", "Too many open files in system",
    "Too many open files", "Inappropriate ioctl for device", "Text file busy",
    "File too large", "No space left on device", "Illegal seek",
    "Read-only file system", "Too many links", "Broken pipe",
    "Numerical argument out of domain", "Numerical result out of range",
    "Resource deadlock avoided", "File name too long", "No locks available",
    "Function not implemented", "Directory not empty",
    "Too many levels of symbolic links", "Unknown error", "Unknown error"
};
int sys_nerr = (int)(sizeof(sys_errlist) / sizeof(sys_errlist[0]));

char *strerror(int e)
{
    if (e >= 0 && e < sys_nerr) return sys_errlist[e];
    return "Unknown error";
}

void perror(const char *s)
{
    char *m = strerror(errno);
    if (s && *s) { write(2, s, (unsigned)strlen(s)); write(2, ": ", 2); }
    write(2, m, (unsigned)strlen(m));
    write(2, "\n", 1);
}
