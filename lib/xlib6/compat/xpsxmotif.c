/*
 * xpsxmotif.c -- link-time compat glue for libXm2 (OpenMotif on the ReactOS
 * POSIX subsystem).  Supplies:
 *   (1) two globals that live in the excluded PrintS.c (XmPrintShell),
 *   (2) no-op Xprint (Xp*) stubs -- CDE has no printing; the callers are gated
 *       behind XpQueryExtension(), which we report as "extension absent",
 *   (3) CRT functions psxcrt lacks: the Latin-1/16-bit wide-char converters,
 *       strtod, strcasecmp/strncasecmp, and popen/pclose (no shell pipes here).
 * MIT-licensed (ReactOS POSIX subsystem).
 */
#include <X11/Intrinsic.h>          /* Display, XContext, Cardinal, Bool */
#include <X11/extensions/Print.h>   /* XPContext */
#include <stddef.h>
#include <ctype.h>

/* --- (1) XmPrintShell globals (defined in the excluded PrintS.c) ----------- */
XContext _XmPrintScreenToShellContext = 0;
Cardinal _XmPrintShellCounter = 0;

/* --- (2) Xprint client stubs (no Xprint server; report absent) ------------- */
Bool XpQueryExtension(Display *dpy, int *event_base, int *error_base)
{
    (void)dpy; if (event_base) *event_base = 0; if (error_base) *error_base = 0;
    return False;                       /* -> Motif skips all print scaling */
}
XPContext XpGetContext(Display *dpy)            { (void)dpy; return (XPContext)0; }
char *XpGetOneAttribute(Display *dpy, XPContext c, int type, char *attr)
{ (void)dpy; (void)c; (void)type; (void)attr; return (char *)0; }
int XpSetImageResolution(Display *dpy, XPContext c, int res, int *prev)
{ (void)dpy; (void)c; (void)res; if (prev) *prev = 0; return 0; }

/* --- (3a) wide-char converters: Latin-1, 16-bit wchar_t (our X_LOCALE) ------ */
size_t mbstowcs(wchar_t *dst, const char *src, size_t n)
{
    size_t i = 0;
    for (; src[i] && i < n; i++) if (dst) dst[i] = (unsigned char)src[i];
    if (dst && i < n) dst[i] = 0;
    return i;
}
size_t wcstombs(char *dst, const wchar_t *src, size_t n)
{
    size_t i = 0;
    for (; src[i] && i < n; i++) if (dst) dst[i] = (char)(src[i] & 0xFF);
    if (dst && i < n) dst[i] = 0;
    return i;
}
int wctomb(char *s, wchar_t wc)
{
    if (s == 0) return 0;               /* not state-dependent */
    *s = (char)(wc & 0xFF);
    return 1;
}
int mbtowc(wchar_t *pwc, const char *s, size_t n)
{
    if (s == 0) return 0;
    if (n == 0) return -1;
    if (pwc) *pwc = (unsigned char)*s;
    return *s ? 1 : 0;
}

/* --- (3b) strtod: parse [+-]ddd[.ddd][(e|E)[+-]ddd] ------------------------- */
double strtod(const char *nptr, char **endptr)
{
    const char *s = nptr;
    double val = 0.0, scale = 1.0;
    int neg = 0, any = 0;

    while (*s == ' ' || *s == '\t' || *s == '\n') s++;
    if (*s == '+' || *s == '-') { neg = (*s == '-'); s++; }
    for (; *s >= '0' && *s <= '9'; s++) { val = val * 10.0 + (*s - '0'); any = 1; }
    if (*s == '.') {
        s++;
        for (; *s >= '0' && *s <= '9'; s++) { val = val * 10.0 + (*s - '0'); scale *= 10.0; any = 1; }
    }
    val /= scale;
    if (any && (*s == 'e' || *s == 'E')) {
        const char *e = s + 1;
        int eneg = 0, exp = 0, edig = 0;
        if (*e == '+' || *e == '-') { eneg = (*e == '-'); e++; }
        for (; *e >= '0' && *e <= '9'; e++) { exp = exp * 10 + (*e - '0'); edig = 1; }
        if (edig) {
            double p = 1.0;
            while (exp--) p *= 10.0;
            if (eneg) val /= p; else val *= p;
            s = e;
        }
    }
    if (endptr) *endptr = (char *)(any ? s : nptr);
    return neg ? -val : val;
}

/* --- (3c) BSD case-insensitive compares ------------------------------------ */
int strcasecmp(const char *a, const char *b)
{
    for (;; a++, b++) {
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
}
int strncasecmp(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
    return 0;
}

/* --- (3d) popen/pclose: no shell pipes on the subsystem -------------------- */
FILE *popen(const char *cmd, const char *mode) { (void)cmd; (void)mode; return (FILE *)0; }
int   pclose(FILE *fp) { (void)fp; return -1; }
