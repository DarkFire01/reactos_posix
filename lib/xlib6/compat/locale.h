/*
 * locale.h -- psx shim: the subsystem CRT has no locale support; X_LOCALE
 * builds route everything through Xlib's own implementation (<X11/Xlocale.h>
 * maps setlocale -> _Xsetlocale and defines the LC_* constants).
 * MIT/X license (see copyright.h).
 */
#ifndef _PSX_LOCALE_H_
#define _PSX_LOCALE_H_

#include <X11/Xlocale.h>

/* struct lconv + localeconv(): psxcrt has neither, but Motif's Scale.c reads
 * localeconv()->decimal_point / thousands_sep for numeric formatting.  We
 * return fixed C-locale values (the only locale the subsystem supports). */
#ifndef _PSX_LCONV_DEFINED
#define _PSX_LCONV_DEFINED
struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
};

static __inline struct lconv *localeconv(void)
{
    static struct lconv _psx_c_lconv;
    _psx_c_lconv.decimal_point  = (char *)".";
    _psx_c_lconv.thousands_sep  = (char *)"";
    _psx_c_lconv.grouping       = (char *)"";
    _psx_c_lconv.int_curr_symbol   = (char *)"";
    _psx_c_lconv.currency_symbol   = (char *)"";
    _psx_c_lconv.mon_decimal_point = (char *)"";
    _psx_c_lconv.mon_thousands_sep = (char *)"";
    _psx_c_lconv.mon_grouping      = (char *)"";
    _psx_c_lconv.positive_sign     = (char *)"";
    _psx_c_lconv.negative_sign     = (char *)"";
    _psx_c_lconv.int_frac_digits = 127;
    _psx_c_lconv.frac_digits     = 127;
    _psx_c_lconv.p_cs_precedes   = 127;
    _psx_c_lconv.p_sep_by_space  = 127;
    _psx_c_lconv.n_cs_precedes   = 127;
    _psx_c_lconv.n_sep_by_space  = 127;
    _psx_c_lconv.p_sign_posn     = 127;
    _psx_c_lconv.n_sign_posn     = 127;
    return &_psx_c_lconv;
}
#endif /* _PSX_LCONV_DEFINED */

#endif /* _PSX_LOCALE_H_ */
