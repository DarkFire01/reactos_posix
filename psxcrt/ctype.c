/*
 * PROJECT:     ReactOS POSIX+ Environment Subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ASCII <ctype.h> for the POSIX userland (the reskit STDC ships none;
 *              programs use these as functions). Plain 7-bit ASCII classification.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */
int isupper(int c)  { return c >= 'A' && c <= 'Z'; }
int islower(int c)  { return c >= 'a' && c <= 'z'; }
int isdigit(int c)  { return c >= '0' && c <= '9'; }
int isalpha(int c)  { return isupper(c) || islower(c); }
int isalnum(int c)  { return isalpha(c) || isdigit(c); }
int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
int isspace(int c)  { return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v'; }
int isprint(int c)  { return c >= 0x20 && c < 0x7f; }
int isgraph(int c)  { return c > 0x20 && c < 0x7f; }
int iscntrl(int c)  { return (c >= 0 && c < 0x20) || c == 0x7f; }
int ispunct(int c)  { return isgraph(c) && !isalnum(c); }
int toupper(int c)  { return islower(c) ? c - 'a' + 'A' : c; }
int tolower(int c)  { return isupper(c) ? c - 'A' + 'a' : c; }
int isascii(int c)  { return (unsigned)c < 0x80; }
int toascii(int c)  { return c & 0x7f; }
