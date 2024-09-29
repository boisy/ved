/* cmds.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

void
keyboard_abort(void)
{
}

void
hexinput(void)
{
	ungetch(get_hex_input(bf->mw));
}


/*********************************************************************
 * Execute a function from the command jump table.
 * If command is not found -1 returned, else 0.
 */

int
dojump(KEY cmd)
{
	CMDTABLE *tbl = cmdtable;

	for (; tbl->cmdvalue; tbl++) {
		/* see if match with key & fn defined */

		if (tbl->cmdvalue == cmd && tbl->cmdfunc != 0) {
			if (opt_noedit == TRUE && tbl->noedit == TRUE)
				return 0;

			(void)(*tbl->cmdfunc) ();	/* do function */
			return 0;	/* signal success */
		}
	}
	return -1;		/* command not found in table */
}

/* EOF */
