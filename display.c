/* display.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

/* display/refresh buffer display */

#include "ved.h"
#include "attrs.h"

void display_line_part(int y, int x, u_char * bff, int offset, int count);
void display_page(int line);
void redisplay(void);
void display_line(int ypos, int line);

/*********************************************************************
 * Scroll left/right
 */

void
scroll_left(void)
{
	if (bf->xoffset) {
		bf->xoffset -= scroll_jumpsize;
		if (bf->xoffset < 0)
			bf->xoffset = 0;
		if (opt_noedit == TRUE)
			redisplay();
	}
}

void
scroll_right(void)
{
	bf->xoffset += scroll_jumpsize;
	if (opt_noedit == TRUE)
		redisplay();
}



/*********************************************************************
 * Set the horizontal display offset register. This is called
 * whenever we need to actually display the data...for now when
 * a cursor is displayed and when the horizontal scroll commands
 * are given.
 */


void
set_xoffset(void)
{
	if (bf->x < bf->xoffset)
		bf->xoffset = bf->x;
	else if (bf->x >= bf->xoffset + (bf->xsize - 1))
		bf->xoffset = bf->x - bf->xsize + 1;
	if (bf->xoffset != bf->xoffset_last)
		redisplay();
}

void
redisplay(void)
{
	display_page(bf->curline - bf->y);
}

void
display_page(int line)
{
	int lastline, y;

	lastline = line + bf->ysize;
	if (lastline > bf->numlines)
		lastline = bf->numlines;

	werase(bf->win);

	for (y = 0; line < lastline;)
		display_line(y++, line++);

	wnoutrefresh(bf->win);

	bf->dirtyscreen = FALSE;
}

void
display_line(int ypos, int line)
{
	int s, e;

	wmove(bf->win, ypos, 0);
	wclrtoeol(bf->win);

	s = bf->linestarts[line];
	e = bf->linestarts[line + 1];
	display_line_part(ypos, 0, bf->buf, s, e - s);
}

void
display_line_part(int y, int x, u_char * bff, int offset, int count)
{
	DISTAB *d;
	int n, t, maxx, firstx, incr;
	u_char c;
	chtype attr;
	FLAG inblock, inhilite;

	maxx = bf->xsize + (bf->xoffset - 1);
	firstx = bf->xoffset;

	for (; count--; offset++) {

		c = bff[offset];

		if (c == EOL && opt_nocr_display)
			c = ' ';

		d = &display_tab[c];

		if (c == TAB) {
			incr = 0;
			n = tabplus(x);
		} else {
			incr = 1;
			n = d->count;
		}

		attr = display_attrs[d->a_area];

		inblock = bf->block_start != -1 && offset <= bf->block_end &&
		    offset >= bf->block_start;

		inhilite = bf->hilite_start != -1 && offset <= bf->hilite_end &&
		    offset >= bf->hilite_start;

		if ((inblock || inhilite) && (inblock != inhilite)) {
			if (d->a_area == EDITOR_NORMAL)
				attr = display_attrs[EDITOR_HIGHLIGHT];
			else
				attr = display_attrs[EDITOR_NORMAL];
		}
		for (t = 0; n--; t += incr) {
			if (x >= firstx && x < maxx)
				mvwaddch(bf->win, y, x - firstx, (d->chars[t] | attr));
			x++;
		}
	}
	bf->xoffset_last = bf->xoffset;
}



void
hilight(int pos, int count)
{
	KEY k;

	bf->hilite_start = pos;
	bf->hilite_end = pos + (count - 1);
	redisplay();
	wmove(bf->win, bf->y, bf->x);
	doupdate();

	k = ved_getkey(bf->win);
	retkey(k);

	bf->hilite_start = -1;
	bf->hilite_end = -1;
	redisplay();
}

/* EOF */
