/* cursor.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

/* static function prototypes */

static int get_top_screen_line(void);
static int get_bottom_screen_line(void);
void cursor_startword(void);
void cursor_startnword(void);
void cursor_up(void);
void cursor_lineend(void);
void cursor_pageup(void);
void cursor_down(void);
void cursor_pagedown(void);
void cursor_linestart(void);
void cursor_endword(void);

/*********************************************************************
 * Fix the cursor X position so that it matches 
 * the line position. When moving around the srcreen
 * the x position may point off the line or in
 * the middle of a tab, etc. However, the buffer
 * position is correct. So, this adjusts the screen
 * position to match the buffer.
 * 
 * All we so is move to the start of the line and
 * then advance until we're back where we started.
 */

void
sync_x(void)
{
	int x, end, z;

	if (bf->xsync == FALSE) {
		x = bf->x;

		cursor_linestart();
		end = bf->linestarts[bf->curline + 1] - 1;

		while (bf->curpos < end) {
			z = csize(bf->buf[bf->curpos], bf->x);
			if ((bf->x + z) > x)
				break;
			bf->x += z;
			bf->curpos++;
		}
	}
	bf->xsync = TRUE;
}

/*********************************************************************
 * Simple cursor movement
 */

void
cursor_left(void)
{
	u_char c;

	sync_x();		/* resolve x-screen pos to real x */

	if (bf->x == 0) {
		if (bf->curpos == 0)
			return;	/* at buffer start...do nada */

		cursor_up();	/* move to end of prior line */
		cursor_lineend();
	} else {
		/* we are in a line of data. */
		/* 1. dec the buffer pointer */
		/* 2. dec the screen x pointers */

		bf->curpos--;

		c = bf->buf[bf->curpos];

		if (c == TAB) {
			bf->xsync = FALSE;
			bf->x--;
			sync_x();
		} else
			bf->x -= csize(bf->buf[bf->curpos], bf->x);
	}

}

void
cursor_right(void)
{
	if (bf->curpos == bf->bsize - 1)
		return;		/* at eof... */

	sync_x();

	/* if at end of line... */

	if (bf->linestarts[bf->curline + 1] == bf->curpos + 1) {
		cursor_linestart();
		cursor_down();
	} else {
		bf->x += csize(bf->buf[bf->curpos], bf->x);
		bf->curpos++;
	}
}


void
cursor_up(void)
{
	if (bf->curline == 0)
		return;

	if (opt_noedit == TRUE) {
		bf->curline -= bf->y;
		bf->y = 0;
		bf->curpos = bf->linestarts[bf->curline];
	}
	bf->curline--;
	bf->curpos = bf->linestarts[bf->curline];

	if (bf->y > 0) {
		bf->y--;
	} else {
		wscrl(bf->win, -1);
		display_line(bf->y, bf->curline);
	}

	bf->xsync = FALSE;
}

void
cursor_down(void)
{
	if (bf->curline >= bf->numlines - 1)
		return;		/* at end of buffer */

	if (opt_noedit) {
		while (bf->y < bf->ysize - 1) {
			bf->curline++;
			bf->curpos = bf->linestarts[bf->curline];
			bf->y++;
		}
	}
	bf->curline++;		/* bump to start of next line */
	bf->curpos = bf->linestarts[bf->curline];

	if (bf->y < bf->ysize - 1) {	/* just adjust screen loc */
		bf->y++;
	} else {		/* scroll up data */
		wscrl(bf->win, 1);
		display_line(bf->y, bf->curline);
	}

	bf->xsync = FALSE;
}

void
cursor_lineend(void)
{
	int t;

	cursor_linestart();
	t = bf->linestarts[bf->curline + 1] - bf->linestarts[bf->curline];

	while (--t) {
		bf->x += csize(bf->buf[bf->curpos], bf->x);
		bf->curpos++;
	}
	bf->xsync = TRUE;
}

void
cursor_linestart(void)
{
	bf->curpos = bf->linestarts[bf->curline];
	bf->x = 0;
	bf->xsync = TRUE;
}

/*********************************************************************
 * Longer cursor movements
 */

void
cursor_filestart(void)
{
	cursor_moveto(0);
}


void
cursor_fileend(void)
{
	cursor_moveto(bf->bsize - 1);
}

void
cursor_pagebottom(void)
{
	if (bf->y == bf->ysize - 1)
		cursor_pagedown();
	else {
		while (bf->curline < bf->numlines && bf->y < bf->ysize - 1)
			cursor_down();
	}
}

void
cursor_pagetop(void)
{
	if (bf->y == 0)
		cursor_pageup();
	else {
		while (bf->y)
			cursor_up();
	}
}

void
cursor_pageup(void)
{
	int top;

	top = bf->curline - bf->y;	/* top line on screen */
	if (top <= 0)
		return;		/* already at top... */

	top -= bf->ysize - 1;
	bf->curline = (top >= 0) ? top : 0;	/* new top of page */
	bf->curpos = bf->linestarts[bf->curline];
	bf->y = 0;
	display_page(bf->curline);
}

void
cursor_pagedown(void)
{
	int bott;

	bott = bf->curline + (bf->ysize - bf->y);	/* last line on page */
	if (bott > bf->numlines)
		return;		/* no more to display */

	bf->curline = bott - 1;	/* new top */
	bf->curpos = bf->linestarts[bf->curline];
	bf->y = 0;

	display_page(bf->curline);

}

/*********************************************************************
 * Word movements
 */

/*********************************************************************
*      Move to the start of the next/previous word.
 * Here a word is a seriesof non-blanks.
 */

void
cursor_startnword(void)
{
	u_char c;

	cursor_endword();

	c = bf->buf[bf->curpos];

	if (v_isblank(c) || c == EOL) {		/* in a blank space */
		cursor_endword();	/* move to end of blanks */
		cursor_right();	/* return at char after blanks */
		return;
	}
	cursor_right();		/* skip to blank after current */
	cursor_endword();	/* skip blanks */
	cursor_right();		/* start of next word    */
}

void
cursor_startpword(void)
{
	u_char c;

	cursor_startword();

	c = bf->buf[bf->curpos];

	if (v_isblank(c) || c == EOL) {
		cursor_startword();
		cursor_left();
		cursor_startword();
		return;
	}
	cursor_left();
	cursor_startword();	/* start of blank in front of current */
	cursor_left();		/* end of prior word */
	cursor_startword();	/* start of prior */
}

/*********************************************************************
 * Move cursor to start/end of the current word. A word is defined 
 * as non - blank space following a blank;
 * or a series of blanks(including EOLs).
 */

void
cursor_endword(void)
{
	int c, in;

	sync_x();
	
	in = bf->buf[bf->curpos];

	if (v_isblank(in) || in == EOL) {
		while (1) {
			if(bf->curpos >= bf->bsize-1)
				return;
			c = (int)bf->buf[bf->curpos + 1];
			if (v_isblank(c) || c == EOL)
				cursor_right();
			else
				break;
		}
	} else {
		while (1) {
			if(bf->curpos >= bf->bsize-1)
				return;	
			c =  bf->buf[bf->curpos + 1];
			if (v_isblank(c) || c == EOL)
				break;
			cursor_right();
		}
	}
}

void
cursor_startword(void)
{
	int c, in;

	sync_x();
	
		
	in = (int)bf->buf[bf->curpos];

	if (v_isblank(in) || in == EOL) {
		while (1) {
			if(bf->curpos <=0)
				return;
			c = (int)bf->buf[bf->curpos - 1];
			if (v_isblank(c) || c == EOL)
				cursor_left();
			else
				break;
		}
	} else {
		while (1) {
			if(bf->curpos <= 0)
				return;
			c = (int)bf->buf[bf->curpos - 1];
			if (v_isblank(c) || c == EOL )
				break;
			cursor_left();
		}
	}
}

void
cursor_startpp(void)
{
	cursor_linestart();

	while (1) {
		if ((bf->curpos == 0) || (bf->buf[bf->curpos - 1] == EOL))
			break;
		cursor_up();
	}

}

void
cursor_endpp(void)
{
	while (1) {
		cursor_lineend();
		if ((bf->buf[bf->curpos] == EOL) || (bf->curpos >= bf->bsize - 1))
			break;
		cursor_right();
	}

}

/*********************************************************************
 * Jump to a matching {}, [], <>, ()
 * We have to be carefull...can't just move the cursor
 * position since the match might fail. 
 */

void
cursor_match(void)
{
	int nestcount, direction, pos;
	u_char c, startchar, targ;

	/*      First see if the cursor is on a valid brace character. If not, we */
	/*      just do a return. Else, we set the search direction for the matching */
	/*      brace and the matching brace character. */

	sync_x();
	pos = bf->curpos;

	switch (c = bf->buf[pos]) {
		case '{':
			direction = 1;
			targ = '}';
			break;
		case '(':
			direction = 1;
			targ = ')';
			break;
		case '[':
			direction = 1;
			targ = ']';
			break;
		case '<':
			direction = 1;
			targ = '>';
			break;
		case '}':
			direction = -1;
			targ = '{';
			break;
		case '>':
			direction = -1;
			targ = '<';
			break;
		case ')':
			direction = -1;
			targ = '(';
			break;
		case ']':
			direction = -1;
			targ = '[';
			break;

		default:
			return;
	}

	/*      Now look for the match.  */

	for (startchar = c, nestcount = 0;;) {
		pos += direction;

		if (pos >= 0 && pos < bf->bsize)
			c = bf->buf[pos];
		else
			vederror("No matching '%c' found", targ);

		if (c == targ) {	/* target brace found, */
			if (!nestcount) {	/* no nesting, redisplay */
				cursor_moveto(pos);	/* just move to the new pos, exit */
				return;
			}
			nestcount--;	/* count down nesting... */
		} else if (c == startchar)
			nestcount++;	/* nested brace, countup */
	}
}


/*********************************************************************
 * Move to a specified offset position
 */

int
get_line_number(int pos)
{
	int *lns = bf->linestarts;
	int low, mid, high;

	low = 0;
	high = bf->numlines;

	while (low <= high) {
		mid = (low + high) / 2;

		if (pos < lns[mid])
			high = mid - 1;
		else if (pos >= lns[mid + 1])
			low = mid + 1;
		else {
			return mid;
		}
	}

	vederror("Internal error in %s, attempt to move out of buffer.",
		 __FUNCTION__);

	return -1;		/* UNREACHABLE, supresses a gcc warning */
}

static inline int
get_top_screen_line(void)
{
	int t;

	t = bf->curline - bf->y;
	return t;
}

static inline int
get_bottom_screen_line(void)
{
	int t;

	t = bf->curline + (bf->ysize - bf->y) - 1;
	if (t > bf->numlines - 1)
		t = bf->numlines - 1;

	return t;
}

int
cursor_moveto(int pos)
{

	int newline, top, bottom, t;

	if (pos >= bf->bsize - 1)
		pos = bf->bsize - 1;

	newline = get_line_number(pos);

	top = get_top_screen_line();
	bottom = get_bottom_screen_line();

	bf->curline = newline;
	bf->curpos = bf->linestarts[bf->curline];
	bf->x = 0;
	bf->xsync = TRUE;

	/* 1. If destination is on the screen and the screen image
	 * is accurate, we just need to set the cursor to the correct
	 * line
	 */

	if (bf->dirtyscreen == FALSE && (newline >= top && newline <= bottom))
		bf->y = bf->curline - top;


	/* 2. If the destination is on screen and the screen image is
	 * NOT accurate redisplay the screen from the top and then move
	 * the cursor
	 */

	else if (bf->dirtyscreen == TRUE && (newline >= top && newline <= bottom))
		redisplay();

	/* 3. Do a compete display, putting the cursor in the middle
	 * of the page.
	 */

	else {
		t = bf->ysize / 2;
		if (t > bf->curline)
			t = bf->curline;
		bf->y = t;
		display_page(bf->curline - t);
	}

	/* Now move to the current position. The old position
	 * might be before or after the current, so do it both ways
	 */


	while (bf->curpos < pos)
		cursor_right();
	while (bf->curpos > pos)
		cursor_left();
}

void
cursor_change_refresh(void)
{
	int pos;

	/* Force position to be inside buffer, prob not needed */

	if (bf->curpos > bf->bsize - 1)
		bf->curpos = bf->bsize - 1;
	pos = bf->curpos;

	wrap_buffer(bf->wrapwidth, 0);

	/* Get the line number for cursor position */

	bf->curline = get_line_number(bf->curpos);

	/* Make sure that the screen y is not past buffer end */

	while (bf->y > bf->curline)
		bf->y--;

	/* Set line and pos to top of screen */

	bf->curline -= bf->y;
	bf->y = 0;
	bf->curpos = bf->linestarts[bf->curline];

	/* Display page */

	display_page(bf->curline);

	/* Move to original pos */

	while (bf->curpos < pos)
		cursor_right();
}

/* EOF */
