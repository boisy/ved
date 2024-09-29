/* info.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

static int nlines, nwords, atword, atline;

/* This counts the words in an area. Either
 * a block or the entire buffer
 */

static void
count_area(int start, int end)
{
	char state;
	u_char c;

	for (nlines = nwords = state = 0, atword = atline = 1;
	     start <= end; start++) {

		c = bf->buf[start];

		if (start == bf->curpos) {
			atword = nwords + 1;
			atline = nlines + 1;
		}
		if (c == EOL)
			nlines++;

		if (state == 1) {	/* between words.... */
			if (c != SPACE && c != TAB && c != EOL)
				state = 0;
		} else if (c == SPACE || c == TAB || c == EOL) {
			state = 1;
			nwords++;
		}
	}
}

void
buffer_info(void)
{
	sync_x();

	dialog_clear();

	dialog_addline("FILE: %s", (bf->filename != NULL ? bf->filename : "No Name"));

	if (bf->block_start != -1 && bf->block_end != -1) {
		count_area(bf->block_start, bf->block_end);
		dialog_addline("Block size %u, %u Words %u Lines",
		    bf->block_end - bf->block_start + 1, nwords, nlines);
	}
	count_area(0, bf->bsize - 1);
	dialog_addline("File size %u, %u Words %u Lines",
		       bf->bsize, nwords, nlines);

	if (atword > nwords)
		atword = nwords;
	dialog_msg("At offset %u, word %u, line %u:",
		   bf->curpos + 1, atword, atline);
}

/* EOF */
