/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     C heap (malloc/free/calloc/realloc) for the POSIX userland. The
 *              reskit ships no malloc -- it comes from psxdll's Heap* exports
 *              (HeapAlloc/HeapFree/HeapReAlloc are forwarded to ntdll Rtl*Heap,
 *              GetProcessHeap returns PEB->ProcessHeap, the standard NT process
 *              heap). Heap handle = GetProcessHeap().
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <sys/types.h>      /* size_t */

/* psxdll / ntdll heap imports (stdcall). */
void * __stdcall GetProcessHeap(void);
void * __stdcall HeapAlloc(void *Heap, unsigned long Flags, size_t Size);
int    __stdcall HeapFree(void *Heap, unsigned long Flags, void *Ptr);
void * __stdcall HeapReAlloc(void *Heap, unsigned long Flags, void *Ptr, size_t Size);

#define HEAP_ZERO_MEMORY 0x00000008

void *malloc(size_t Size)
{
    return HeapAlloc(GetProcessHeap(), 0, Size ? Size : 1);
}

void *calloc(size_t Count, size_t Size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (Count * Size) ? (Count * Size) : 1);
}

void *realloc(void *Ptr, size_t Size)
{
    if (Ptr == 0)
        return malloc(Size);
    if (Size == 0)
    {
        HeapFree(GetProcessHeap(), 0, Ptr);
        return 0;
    }
    return HeapReAlloc(GetProcessHeap(), 0, Ptr, Size);
}

void free(void *Ptr)
{
    if (Ptr != 0)
        HeapFree(GetProcessHeap(), 0, Ptr);
}
