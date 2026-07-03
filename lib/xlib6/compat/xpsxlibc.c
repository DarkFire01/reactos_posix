/*
 * xpsxlibc.c -- small libc/libm/runtime pieces the POSIX userland lacks that the R1
 * Xlib + the ico demo need: ffs (BSD), random/srandom, sin/cos (range-reduced Taylor,
 * adequate for a spinning polyhedron), and the MSVC float->long helpers (_ftol /
 * _ftol2 / _ftol2_sse; the value arrives on the x87 stack, result in EDX:EAX).
 * MIT/X license (see copyright.h). Part of the ReactOS psx libX11 port.
 */

#define XPSX_PI   3.14159265358979323846

/* ---- BSD bit/rng ---------------------------------------------------------------- */

int ffs(int mask)
{
    int bit;
    if (mask == 0)
        return 0;
    for (bit = 1; (mask & 1) == 0; bit++)
        mask = (int)((unsigned)mask >> 1);
    return bit;
}

static unsigned long _psx_rand_state = 1;

void srandom(unsigned int seed)
{
    _psx_rand_state = seed ? seed : 1;
}

long random(void)
{
    _psx_rand_state = _psx_rand_state * 1103515245UL + 12345UL;
    return (long)((_psx_rand_state >> 16) & 0x7fffffffUL);
}

/* ---- sin/cos (range reduction to [-pi,pi] + Taylor) ----------------------------- */

static double _psx_reduce(double x)
{
    /* subtract the nearest integer multiple of 2*pi */
    double t = x / (2.0 * XPSX_PI);
    long n = (long)(t >= 0.0 ? t + 0.5 : t - 0.5);
    return x - (double)n * (2.0 * XPSX_PI);
}

double sin(double x)
{
    double x2;
    x = _psx_reduce(x);
    x2 = x * x;
    /* x - x^3/6 + x^5/120 - x^7/5040 + x^9/362880 */
    return x * (1.0 - x2 * (1.0 / 6.0 - x2 * (1.0 / 120.0
             - x2 * (1.0 / 5040.0 - x2 * (1.0 / 362880.0)))));
}

double cos(double x)
{
    return sin(x + XPSX_PI / 2.0);
}

/* ---- more <math.h> for the Xt clients (xeyes: atan2/hypot; see math.h) ----------- */

double fabs(double x)
{
    return x < 0.0 ? -x : x;
}

double sqrt(double x)
{
    /* Newton-Raphson; plenty for widget geometry */
    double g;
    int i;

    if (x <= 0.0)
        return 0.0;
    g = x > 1.0 ? x : 1.0;
    for (i = 0; i < 40; i++)
        g = 0.5 * (g + x / g);
    return g;
}

double hypot(double x, double y)
{
    return sqrt(x * x + y * y);
}

static double _psx_atan(double x)
{
    /* atan via argument reduction to |x|<=1 then a rational approximation */
    if (x > 1.0)
        return XPSX_PI / 2.0 - _psx_atan(1.0 / x);
    if (x < -1.0)
        return -XPSX_PI / 2.0 - _psx_atan(1.0 / x);
    /* Pade-style approximation, good to ~1e-6 on [-1,1] */
    {
        double x2 = x * x;
        return x * (0.9998660 + x2 * (-0.3302995 + x2 * (0.1801410
                 + x2 * (-0.0851330 + x2 * 0.0208351))));
    }
}

double atan(double x)
{
    return _psx_atan(x);
}

double atan2(double y, double x)
{
    if (x > 0.0)
        return _psx_atan(y / x);
    if (x < 0.0) {
        if (y >= 0.0)
            return _psx_atan(y / x) + XPSX_PI;
        return _psx_atan(y / x) - XPSX_PI;
    }
    /* x == 0 */
    if (y > 0.0)
        return XPSX_PI / 2.0;
    if (y < 0.0)
        return -XPSX_PI / 2.0;
    return 0.0;
}

double floor(double x)
{
    long n = (long)x;
    return (x < 0.0 && x != (double)n) ? (double)(n - 1) : (double)n;
}

double ceil(double x)
{
    long n = (long)x;
    return (x > 0.0 && x != (double)n) ? (double)(n + 1) : (double)n;
}

double fmod(double x, double y)
{
    long n;
    if (y == 0.0)
        return 0.0;
    n = (long)(x / y);
    return x - (double)n * y;
}

/* ---- MSVC float->long runtime helpers ------------------------------------------- */

static short _psx_cw, _psx_cw_trunc;

__declspec(naked) void _ftol(void)
{
    __asm
    {
        fnstcw   _psx_cw                 ; save FPU control word
        mov      ax, _psx_cw
        or       ah, 0Ch                 ; RC = 11 -> truncate toward zero
        mov      _psx_cw_trunc, ax
        fldcw    _psx_cw_trunc
        push     ecx
        push     ecx                     ; 8 bytes of scratch
        fistp    qword ptr [esp]         ; store int64, pops ST(0)
        fldcw    _psx_cw                 ; restore control word
        pop      eax
        pop      edx
        ret
    }
}

__declspec(naked) void _ftol2(void)     { __asm { jmp _ftol } }
__declspec(naked) void _ftol2_sse(void) { __asm { jmp _ftol } }
