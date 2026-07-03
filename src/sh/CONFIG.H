/*
 * Configuration file for the PD ksh
 */

#ifndef	_CONFIG_H
#define	_CONFIG_H

/*
 * Builtin edit modes
 */

#define	EMACS				/* EMACS-like mode */
#define	VI				/* vi-like mode */

/*
 * Various compilation options.  They are defined to 1 because
 * the original code uses #if a lot, and some versions of cpp
 * seem to choke on #if NOTDEF
 */

/* #define	JOBS			/* job control */
/* #define	FASCIST			/* Fascist getopts */
#define	SHARPBANG			/* Hack to handle #! */
#define	SILLY			/* Game of life in EMACS mode */
/* #define	SWTCH			/* Handle SWTCH for shl(1) */

#endif	/* _CONFIG_H */
