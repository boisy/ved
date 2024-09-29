/* shell.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

/*********************************************************
 * Execute a sub shell
 *
 */


void
doshell(void)
{
	int err;
	char *p;

	/* get the shell param. note that an empty string is okay, we
	 * abort if you hit abort.
	 */

	dialog_clear();
	p = dialog_entry("SHELL:");
	if (p == NULL)
		goto EXIT;

	endwin();

	/* If nothing is entered at the prompt we fork an
	 * interactive shell. This is $SHELL if defined or
	 * '/bin/sh' if not.
	 */

	if (*p == '\0') {
		p = getenv("SHELL");
		if (p == NULL)
			p = "/bin/sh";
	}
	/* do the command */

	err = system(p);

	if (err && errno)
		fprintf(stderr, "Error from shell '%s'\n", strerror(errno));

	printf("Press <ENTER> to continue");
	getchar();

	refresh();

    EXIT:
    	return NULL;
}

/* EOF */
