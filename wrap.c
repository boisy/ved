/* wrap.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"


/*********************************************************************
 * Manage a line starts[] buffer. This is dynamic array of ints
 * with each entry pointing to the start of a line.
 * Note that the first line is always 0 (start of file)
 * and the last line points one position past the eof.
 */

#define INCREASE 1000

static inline void
increase_linestarts(void)
{
	int *tmp;

	tmp = (int *)realloc(bf->linestarts, sizeof(int) *
			         (bf->linestarts_size + INCREASE));
	if (tmp == NULL)
		vederror("Cannot increase the line-wrap pointers."
			 " You REALLY need more memory!");

	bf->linestarts = tmp;
	bf->linestarts_size += INCREASE;
}

#undef INCREASE


/**********************************************
 * Set a line pointer. If the new pointer is
 * too large for the current array, a new array
 * is created.
 */

static void
add_ptr(int line, int offset)
{
	if (line >= bf->linestarts_size)
		increase_linestarts();
	bf->linestarts[line] = offset;
}

/**********************************************
 * Line wrap the buffer from the specified point (line)
 * to the end of the buffer. It is important that the
 * caller ensures that the pointer to the current line
 * is correct...if not, then call with line==0.
 */


void
wrap_buffer(int width, int line)
{
	u_char *s, *e;
	int offset, last_space, startpos, x;

	/* Before doing anything, set pointer #0. This, btw, 
	 * will also create the linestarts[] if it doesn 't already
	 * exist.
	 */

	add_ptr(0, 0);


	/* Set some counters. Note that the start of the buffer is 
	 * set to one before the buffer to account for the initial 
	 * increments...
	 */

	if (line == 0) {
		startpos = 0;
		line = 1;
	} else {
		startpos = bf->linestarts[line];
		line++;
	}

	/* We absolutely depend on the file ending with an EOL. If
	 * there isn't one there, add it now!
	 */

	if (bf->buf[bf->bsize - 1] != EOL) {
		openbuff(1, bf->bsize - 1);
		bf->buf[bf->bsize - 1] = EOL;
	}
	offset = startpos - 1;
	s = bf->buf + offset;
	e = bf->buf + bf->bsize - 1;
	last_space = 0;
	x = 0;


	/* now wrap at (hopefully) warp speed */

	while (1) {
		offset++;
		s++;

		/* the EOF is always a line end, and our signal to exit 
		 * having the test as >= instead of == is mostly a sanity 
		 * check...it should never happen that e is past the EOF
		 */

		if (s >= e) {
			add_ptr(line, offset + 1);
			break;
		}
		/* an EOL is aways a line end */

		if (*s == EOL) {
			add_ptr(line++, offset + 1);
			last_space = 0;
			x = 0;
			continue;
		}
		if (width) {
			x += csize(*s, x);
			if (x > width) {
				if (last_space)
					offset = last_space;
				s = bf->buf + offset;
				add_ptr(line++, offset + 1);
				last_space = 0;
				x = 0;
			} else if ((*s == SPACE) || (*s == TAB))
				last_space = offset;
		}
	}

	bf->numlines = line;	/* set number of lines in file */
}

/***************************************************
 * Text for the wordwrap option text in a pulldown
 */

char *
menutext_wordwrap(void)
{
	return (bf->wrapwidth == 0) ? "Wordwrap on" : "Wordwrap off";
}

/****************************************************
 * Set the word wrap from menu
 */

void
toggle_wordwrap(void)
{
	if (bf->wrapwidth == 0)
		bf->wrapwidth = bf->xsize - 1;
	else
		bf->wrapwidth = 0;

	cursor_change_refresh();
}

/* EOF */
