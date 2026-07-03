/*
	xnglob.c

	The globulate function below takes an NT command line and expands
	wildcards according to POSIX.2 rules. It returns 0 on success, or -1
	on failure. The reason that *argv is always globbed on is to convert
	any backslashes in the program name to slashes. The deglobulate
	function below it merely frees the memory associated with the
	previously run globulate.
*/

#include <stdlib.h>
#include <string.h>
#include "glob.h"

static signed int globulated_argi;
signed int globulated_argc;
char **globulated_argv;

signed int globulate (signed int argi, signed int argc, char **argv)
{
	glob_t gl;
	signed int ret, a;

	if (argi <= 0 || argc < argi || argv == NULL || (ret = glob(*argv, GLOB_NOCHECK, NULL, &gl)) != 0)
		ret = -1;
	else
	{
		for (a = argi; a < argc; ++a)
		{
			if ((ret = glob(argv[a], GLOB_NOCHECK|GLOB_APPEND|GLOB_SHQUOTE, NULL, &gl)) != 0)
			{
				ret = -1;
				goto globfree_gl;
			}
		}
		if ((globulated_argv = malloc((argi + gl.gl_pathc + 1) * sizeof *globulated_argv)) == NULL)
		{
			ret = -1;
			goto globfree_gl;
		}
		if ((*globulated_argv = malloc(strlen(*(gl.gl_pathv)) + 1)) == NULL)
		{
			ret = -1;
			goto globfree_gl;
		}
		(void) strcpy(*globulated_argv, *(gl.gl_pathv));
		for (a = 1; a < argi; ++a)
			globulated_argv[a] = argv[a];
		globulated_argc = gl.gl_pathc - 1;
		for (a = 0; a < globulated_argc; ++a)
		{
			if ((globulated_argv[a + argi] = malloc(strlen(gl.gl_pathv[a + 1]) + 1)) == NULL)
			{
				while (a--)
					free(globulated_argv[a + argi]);
				free(*globulated_argv);
				ret = -1;
				goto globfree_gl;
			}
			(void) strcpy(globulated_argv[a + argi], gl.gl_pathv[a + 1]);
		}
		globulated_argv[a + argi] = NULL;
		globulated_argc += argi;
		globulated_argi = argi;
		ret = 0;
globfree_gl:
		globfree(&gl);
	}
	return ret;
}

void deglobulate (void)
{
	for (globulated_argi = globulated_argc - globulated_argi; globulated_argi--; )
		free(globulated_argv[--globulated_argc]);
	free(*globulated_argv);
	free(globulated_argv);
}
