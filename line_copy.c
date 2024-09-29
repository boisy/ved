/* line_copy.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

/*****************************************************************
   The copy_line() and get_line() functions permit multiple copies
   of a single line. The line is marked with the GET_LINE command 
   (default==ESC-9) which buffers the current screen line;
   the COPY_LINE command (default==ESC-0) copies from the buffer
   to the current position.

*/

static u_char *copy_linebuff = NULL;
static int copy_linesize = 0;

void
get_cline(void)
{
	int end, start;

	start = bf->curpos;	/* do the save from the cursor */

	cursor_lineend();
	end = bf->curpos;	/* end of the line */

	copy_linesize = end - start + 1;

	copy_linebuff = realloc(copy_linebuff, copy_linesize);

	if (copy_linebuff == NULL)
		vederror("Unable to get memory for line copy");

	memcpy(copy_linebuff, &bf->buf[start], copy_linesize);

	cursor_moveto(start);
}

void
get_line(void)
{
	int orig = bf->curpos;

	cursor_linestart();
	get_cline();
	cursor_moveto(orig);
}

void
copy_line(void)
{
	if (copy_linesize > 0) {
		openbuff(copy_linesize, bf->curpos);
		memcpy(&bf->buf[bf->curpos], copy_linebuff, copy_linesize);
		cursor_change_refresh();
	}
}


/* EOF */
