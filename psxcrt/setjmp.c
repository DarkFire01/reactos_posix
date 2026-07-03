/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Minimal i386 setjmp/longjmp for the POSIX userland. We can't use
 *              MSVC's _setjmp3 (SEH-aware, needs the MSVC CRT we don't link), so
 *              save/restore the callee-saved regs + esp/eip directly. No SEH
 *              unwinding -- sufficient for the shell's error-recovery longjmps.
 *              jmp_buf layout: [ebp][ebx][edi][esi][eip][esp].
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */
__declspec(naked) int setjmp(int *env)
{
    __asm {
        mov  ecx, [esp+4]        ; env
        mov  [ecx+0],  ebp
        mov  [ecx+4],  ebx
        mov  [ecx+8],  edi
        mov  [ecx+12], esi
        mov  eax, [esp]          ; return address
        mov  [ecx+16], eax
        lea  eax, [esp+4]        ; caller's esp (after our ret)
        mov  [ecx+20], eax
        xor  eax, eax            ; setjmp returns 0
        ret
    }
}

__declspec(naked) void longjmp(int *env, int val)
{
    __asm {
        mov  ecx, [esp+4]        ; env
        mov  eax, [esp+8]        ; val
        test eax, eax
        jnz  nonzero
        inc  eax                 ; longjmp(env,0) must make setjmp return 1
    nonzero:
        mov  ebp, [ecx+0]
        mov  ebx, [ecx+4]
        mov  edi, [ecx+8]
        mov  esi, [ecx+12]
        mov  edx, [ecx+16]       ; saved eip
        mov  esp, [ecx+20]       ; saved esp
        jmp  edx                 ; resume in setjmp's caller, eax = val
    }
}

/* MSVC lowers a setjmp() call to _setjmp3 (SEH intrinsic) regardless of the
 * declaration; _setjmp is the non-SEH form. Both take the jmp_buf at [esp+4]
 * (the extra _setjmp3 frame-context arg is ignored), so they alias setjmp. */
__declspec(naked) int _setjmp3(int *env, void *ctx)
{
    __asm {
        mov  ecx, [esp+4]
        mov  [ecx+0],  ebp
        mov  [ecx+4],  ebx
        mov  [ecx+8],  edi
        mov  [ecx+12], esi
        mov  eax, [esp]
        mov  [ecx+16], eax
        lea  eax, [esp+4]
        mov  [ecx+20], eax
        xor  eax, eax
        ret
    }
}
