/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     POSIX crt0 -- the startup for NT-4.0-style POSIX executables that
 *              link psxdll. Faithful to the real reskit crt0 in SH.EXE
 *              (__PosixProcessStartup @VA 0x41DEE4 -> mainCRTStartup @0x41DC11):
 *
 *                __PdxInitializeData(&errno, &environ);   (stdcall)
 *                base = __PdxGetCmdLine();                (PEB CommandLine.Buffer)
 *                // base is a base-relative int32 offset table: argv[] then env[],
 *                // each NUL-terminated, followed by the packed strings. Turn each
 *                // offset into an absolute char* IN PLACE (*slot += base).
 *                main(argc, argv);
 *                exit(ret);
 *
 *              The psxss connect + 32 KiB shared-section map + heap are already
 *              done in psxdll's DllMain before we run -- the crt0 must NOT redo
 *              them. envp is delivered via the 'environ' global, not to main().
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* psxdll imports (see psxdll.spec). __PdxInitializeData is stdcall (the real
 * entry pushes 2 args and does not clean the stack afterward). */
void __stdcall __PdxInitializeData(int *ErrnoPtr, char ***EnvironPtr);
void *         __PdxGetCmdLine(void);
void           _exit(int Status);

extern int main(int argc, char **argv);
extern void _cleanup(void);     /* flush open stdio streams (stdio.c) */

/* The two cells psxdll's syscall stubs write through. */
int    errno;
char **environ;

#define ATEXIT_MAX 32
static void (*g_AtExit[ATEXIT_MAX])(void);
static int   g_AtExitCount;

int atexit(void (*Function)(void))
{
    if (g_AtExitCount >= ATEXIT_MAX)
        return -1;
    g_AtExit[g_AtExitCount++] = Function;
    return 0;
}

void exit(int Status)
{
    while (g_AtExitCount > 0)
        g_AtExit[--g_AtExitCount]();
    _cleanup();
    _exit(Status);
    for (;;) { }        /* not reached */
}

void __PosixProcessStartup(void)
{
    char  *base;
    long  *slot;
    int    argc;
    char **argv;

    __PdxInitializeData(&errno, &environ);

    base = (char *)__PdxGetCmdLine();
    argv = (char **)base;

    /* argv: fix offsets in place, count to the NUL terminator */
    argc = 0;
    for (slot = (long *)base; *slot != 0; slot++)
    {
        *slot += (long)base;
        argc++;
    }

    /* skip the argv NUL; the env offset table follows contiguously */
    slot++;
    environ = (char **)slot;
    for (; *slot != 0; slot++)
        *slot += (long)base;

    exit(main(argc, argv));
}
