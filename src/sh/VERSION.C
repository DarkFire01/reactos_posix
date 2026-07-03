/*
 * value of $KSH_VERSION
 */

#ifndef lint
static char *RCSid = "$Id: version.c,v 3.3 89/03/27 15:52:29 egisin Exp $";
static char *sccs_id = "@(#)version.c	4.1 91/11/09 14:55:16 (sjg)";
#endif

#include <stddef.h>
#include <setjmp.h>
#include "sh.h"

char ksh_version [] =
	"KSH_VERSION=@(#)PD KSH v4.1 91/11/09 14:55:16";

/***
$Log:	version.c,v $
Version  4.0  91/11/09  sjg
distribution
Revision 3.3  89/03/27  15:52:29  egisin
distribution

Revision 3.2  88/12/14  20:10:41  egisin
many fixes

Revision 3.1  88/11/03  09:18:36  egisin
alpha distribution

Revision 1.3  88/10/20  17:34:03  egisin
added @(#) to ksh_version

Revision 1.2  88/09/27  19:01:58  egisin
fix version.c

Revision 1.1  88/09/27  18:59:06  egisin
Initial revision
***/

