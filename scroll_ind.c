/* scroll_ind.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

/* What to do with the scroll indicator...
 * bit 0 = left scroll enabled
 * bit 1 = right scroll enabled
 */
 
 
static u_char scroll_indicator;


/* Names for RC file */

static NAMETABLE  scroll_side_name[] =
{
	{"None", 0},
	{"Left", 1},
	{"Right", 2},
	{"Both", 3},
	{0}
};

/**************************************************
 * Set the value for the scroll indicator from
 * a string (left, right, etc). Returns -1 if
 * not a valid string option.
 */
 
int
set_scroll_indicator(char *s)
{
	int n, orig;

	orig=scroll_indicator;	/* remember orig value */

				
	n=lookup_value(s, scroll_side_name);
	
	/* update current displays if there was a change */
	
	if(n>=0) {
		scroll_indicator=n;
		if( (num_buffers > 0) && (orig != scroll_indicator) )
				terminal_refresh();
	}

	return n;
}

/**************************************************
 * Return the name of the setting (None, Left, etc)
 */
 

char *
get_scroll_side_name(void)
{
	return lookup_name(scroll_indicator, scroll_side_name);
}

/**************************************************
 * Return the current value of the setting.
 */
 

int
get_scroll_side_value(void)
{
	return scroll_indicator;
}

/****************************************************************
 * Calculate the offsets and size change needed for the edit screen
 * based on the scroll_indicator.
 * Scroll_indicator can have the following values:
 *    0 - off
 *    1 - left side
 *    2 - right side
 *    3 - left and right side.
 */

/* This returns the x offset needed...1 if the left scroll is enabled */
 
int 
get_scroll_indicator_offset(void)
{
	return scroll_indicator & 1;
}

/* Return the number of scroll bars: 0, 1 or 2 */

int 
get_scroll_indicator_change(void)
{
	if(scroll_indicator == 0)
		return 0;
	else if(scroll_indicator==3)
		return 2;
	else return 1;
}
	

/*********************************************************
 * Erase old indicator and display new one. It might be 
 * nicer to only do this if the indicator has moved, but
 * this way ensures that a proper one is always displayed.
 * Besides, nucrses is smart enuf to know if no change has
 * been made....
 */

void
update_scroll_pointer(int lnum)
{
	
	int pos, ls[2], t;
	
	pos = 1 + (bf->ysize * lnum / bf->numlines);

	switch(scroll_indicator) {
		case 0:
			ls[0] = -1;
			ls[1] = -1;
			break;
			
		case 1:
			ls[0] = 0;
			ls[1] = -1;
			break;
			
		case 2:
			ls[0] = -1;
			ls[1] = bf->xsize;
			break;
			
		case 3:
			ls[0] = 0;
			ls[1] = bf->xsize + 1;
			break;
	}
	
	
	for(t=0; t<2; t++) {
		if(ls[t]>=0) {
			mvwaddch(bf->mw, bf->scrollbar_pos, ls[t], ' ' );
			mvwaddch(bf->mw, pos, ls[t], '|' );
			
			/* This is necessary to workaround an optimization problem
			 * in ncurses. If the frame window and the edit window are
			 * the same color, setting both left and right indicators
			 * causes the edit line to be cleared. Adding the refresh
			 * seems to interupt the optimization (I'm guessing) and
			 * all works. So, don't delete the following line even
			 * though it looks like it isn't needed.
			 */
			
			wnoutrefresh(bf->mw);
		}
	}
	
	bf->scrollbar_pos = pos;
}


/* EOF */

