/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Buffered stdio backend for the POSIX userland. The reskit ships
 *              the FILE model + putc/getc macros (SH/STD/H/STDIO.H) and the
 *              formatter (VPRINTF.C/FPRINTF.C/SPRINTF.C), but the buffer engine
 *              (_iob[], _filbuf, _flsbuf, fopen/fdopen/...) was spliced from a
 *              host libc and is absent -- this file supplies it, over the psxdll
 *              read/write/open/close/lseek syscalls. Classic AT&T _iobuf engine.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <sys/types.h>
#include <stdio.h>          /* the reskit FILE model + flags + macros */

/* POSIX syscalls from psxdll (cdecl). */
int  open(const char *Path, int Flags, ...);
int  close(int Fd);
int  read(int Fd, void *Buf, unsigned Count);
int  write(int Fd, const void *Buf, unsigned Count);
long lseek(int Fd, long Offset, int Whence);

extern void *malloc(size_t Size);
extern void  free(void *Ptr);

/* O_* we need for fopen (values from psx/fcntl.h). */
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0100
#define O_TRUNC  0x0200
#define O_APPEND 0x0008

#ifndef _NFILE
#define _NFILE 64
#endif

/* The stream table. 0/1/2 pre-bound to stdin/stdout/stderr (buffers lazy). */
struct _iobuf _iob[_NFILE] =
{
    { 0, 0, 0, 0, _IOREAD,          0 },   /* stdin  */
    { 0, 0, 0, 0, _IOWRT,           1 },   /* stdout */
    { 0, 0, 0, 0, _IOWRT | _IONBF,  2 },   /* stderr */
};

static int GetBuffer(FILE *f)
{
    if (f->_base != 0)
        return 0;
    if (f->_flag & _IONBF)
        return 0;
    f->_base = (unsigned char *)malloc(BUFSIZ);
    if (f->_base == 0)
    {
        f->_flag |= _IONBF;     /* fall back to unbuffered */
        return 0;
    }
    f->_flag |= _IOMYBUF;
    f->_bufsiz = BUFSIZ;
    f->_ptr = f->_base;
    f->_cnt = 0;
    return 0;
}

int _filbuf(FILE *f)
{
    int n;

    if (!(f->_flag & (_IOREAD | _IORW)))
    {
        f->_flag |= _IOERR;
        return EOF;
    }
    f->_flag |= _IOREAD;
    GetBuffer(f);

    if (f->_flag & _IONBF)
    {
        unsigned char c;
        n = read(f->_file, &c, 1);
        if (n <= 0) { f->_flag |= (n == 0) ? _IOEOF : _IOERR; return EOF; }
        return c;
    }

    n = read(f->_file, f->_base, f->_bufsiz);
    if (n <= 0)
    {
        f->_cnt = 0;
        f->_ptr = f->_base;
        f->_flag |= (n == 0) ? _IOEOF : _IOERR;
        return EOF;
    }
    f->_cnt = n - 1;
    f->_ptr = f->_base + 1;
    return f->_base[0];
}

int _flsbuf(int c, FILE *f)
{
    int n;

    if (!(f->_flag & (_IOWRT | _IORW)))
    {
        f->_flag |= _IOERR;
        return EOF;
    }
    f->_flag |= _IOWRT;

    if (f->_base == 0 && !(f->_flag & _IONBF))
        GetBuffer(f);

    if (f->_flag & _IONBF)
    {
        unsigned char cc = (unsigned char)c;
        if (write(f->_file, &cc, 1) != 1) { f->_flag |= _IOERR; return EOF; }
        f->_cnt = 0;
        return (unsigned char)c;
    }

    /* buffer full (putc drove _cnt negative): flush [_base, _ptr) then buffer c */
    n = (int)(f->_ptr - f->_base);
    f->_ptr = f->_base;
    f->_cnt = f->_bufsiz - 1;
    if (n > 0 && write(f->_file, f->_base, n) != n)
    {
        f->_flag |= _IOERR;
        return EOF;
    }
    *f->_ptr++ = (unsigned char)c;
    return (unsigned char)c;
}

int fflush(FILE *f)
{
    int n;

    if (f == 0)
        return 0;
    if (!(f->_flag & _IOWRT) || f->_base == 0)
        return 0;
    n = (int)(f->_ptr - f->_base);
    f->_ptr = f->_base;
    f->_cnt = f->_bufsiz;
    if (n > 0 && write(f->_file, f->_base, n) != n)
    {
        f->_flag |= _IOERR;
        return EOF;
    }
    return 0;
}

/* Called by exit() -- flush every open write stream. */
void _cleanup(void)
{
    int i;
    for (i = 0; i < _NFILE; i++)
        if (_iob[i]._flag & _IOWRT)
            fflush(&_iob[i]);
}

static FILE *FindStream(void)
{
    int i;
    for (i = 3; i < _NFILE; i++)
        if ((_iob[i]._flag & (_IOREAD | _IOWRT | _IORW)) == 0)
            return &_iob[i];
    return 0;
}

FILE *fdopen(int fd, const char *mode)
{
    FILE *f = FindStream();
    if (f == 0)
        return 0;
    f->_cnt = 0;
    f->_ptr = f->_base = 0;
    f->_bufsiz = 0;
    f->_file = (char)fd;
    f->_flag = 0;
    if (*mode == 'r') f->_flag = _IOREAD;
    else if (*mode == 'w' || *mode == 'a') f->_flag = _IOWRT;
    if (mode[1] == '+' || (mode[1] == 'b' && mode[2] == '+'))
        f->_flag = _IORW;
    return f;
}

FILE *fopen(const char *name, const char *mode)
{
    int flags = 0, fd;
    if (*mode == 'r')      flags = (mode[1] == '+') ? O_RDWR : O_RDONLY;
    else if (*mode == 'w') flags = ((mode[1] == '+') ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
    else if (*mode == 'a') flags = ((mode[1] == '+') ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
    else return 0;
    fd = open(name, flags, 0666);
    if (fd < 0)
        return 0;
    return fdopen(fd, mode);
}

int fclose(FILE *f)
{
    int r = 0;
    if (f == 0 || !(f->_flag & (_IOREAD | _IOWRT | _IORW)))
        return EOF;
    fflush(f);
    if (close(f->_file) < 0)
        r = EOF;
    if (f->_flag & _IOMYBUF)
        free(f->_base);
    f->_base = f->_ptr = 0;
    f->_cnt = f->_bufsiz = 0;
    f->_flag = 0;
    return r;
}

int fputc(int c, FILE *f)  { return putc(c, f); }
int fgetc(FILE *f)         { return getc(f); }

/* stdio.h defines these as macros; provide real functions too (undef first). */
#undef putchar
#undef getchar
int putchar(int c)         { return putc(c, stdout); }
int getchar(void)          { return getc(stdin); }

int fputs(const char *s, FILE *f)
{
    while (*s)
        if (putc(*s++, f) == EOF)
            return EOF;
    return 0;
}

int puts(const char *s)
{
    if (fputs(s, stdout) == EOF || putc('\n', stdout) == EOF)
        return EOF;
    return 0;
}

char *fgets(char *s, int n, FILE *f)
{
    char *p = s;
    int c;
    while (--n > 0 && (c = getc(f)) != EOF)
    {
        *p++ = (char)c;
        if (c == '\n')
            break;
    }
    if (p == s && c == EOF)
        return 0;
    *p = '\0';
    return s;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *f)
{
    size_t total = size * nmemb, got = 0;
    unsigned char *p = (unsigned char *)ptr;
    int c;
    if (total == 0)
        return 0;
    while (got < total && (c = getc(f)) != EOF)
        p[got++] = (unsigned char)c;
    return got / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f)
{
    size_t total = size * nmemb, put = 0;
    const unsigned char *p = (const unsigned char *)ptr;
    if (total == 0)
        return 0;
    while (put < total)
        if (putc(p[put], f) == EOF)
            break;
        else
            put++;
    return put / size;
}

int ungetc(int c, FILE *f)
{
    if (c == EOF || f->_base == 0)
        return EOF;
    if (f->_ptr <= f->_base)
        return EOF;
    *--f->_ptr = (unsigned char)c;
    f->_cnt++;
    f->_flag &= ~_IOEOF;
    return (unsigned char)c;
}

int fseek(FILE *f, long offset, int whence)
{
    fflush(f);
    f->_cnt = 0;
    f->_ptr = f->_base;
    f->_flag &= ~_IOEOF;
    return (lseek(f->_file, offset, whence) < 0) ? -1 : 0;
}

long ftell(FILE *f)
{
    long pos = lseek(f->_file, 0, 1 /* SEEK_CUR */);
    if (pos < 0)
        return -1;
    if (f->_flag & _IOREAD)
        pos -= f->_cnt;
    else if ((f->_flag & _IOWRT) && f->_base)
        pos += (f->_ptr - f->_base);
    return pos;
}

void rewind(FILE *f)      { fseek(f, 0L, 0 /* SEEK_SET */); f->_flag &= ~_IOERR; }

int setvbuf(FILE *f, char *buf, int mode, size_t size)
{
    fflush(f);
    if (f->_flag & _IOMYBUF)
        free(f->_base);
    f->_flag &= ~(_IOMYBUF | _IONBF | _IOLBF);
    if (mode == _IONBF) { f->_flag |= _IONBF; f->_base = f->_ptr = 0; f->_bufsiz = 0; }
    else { f->_base = f->_ptr = (unsigned char *)buf; f->_bufsiz = (int)size; f->_cnt = 0;
           if (mode == _IOLBF) f->_flag |= _IOLBF; }
    return 0;
}

void setbuf(FILE *f, char *buf)
{
    setvbuf(f, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}
