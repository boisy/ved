/* mouse.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

/*********************************************************
 * A mouse click during editing comes here. 
 */

void
mouse(void)
{
	
#ifdef NCURSES_VERSION

	int y, x, lp, rp;

	/* 1. If the cursor is on the current edit screen, we move the
	 *    cursor to that location.
	 */

	if (wenclose(bf->win, mousey, mousex) == TRUE) {

		getbegyx(bf->win, y, x);

		y = mousey - y;
		x = mousex - x;

		while ((bf->y < y) && (bf->curline < bf->numlines - 1))
			cursor_down();

		while (bf->y > y)
			cursor_up();

		bf->x = x;
		bf->xsync=FALSE;
		sync_x();
		return;
	}
	/* 2. See if mouse is on menu bar or in scroll areas. */

	if (wenclose(bf->mw, mousey, mousex) == TRUE) {

		getbegyx(bf->mw, y, x);
		y = mousey - y;
		x = mousex - x;

		/* In scroll areas... */

		switch(get_scroll_side_value()) {
			case 1:
				lp=0;
				rp=-1;
				break;
				
			case 2:
				lp=-1;
				rp=bf->xsize;
				break;
				
			case 3:
				lp=0;
				rp=bf->xsize+1;
				break;
				
			default:
				lp=-1;
				rp=-1;
				break;
		}
		
		
		if (x == lp || x == rp) {

			if (y == 0)
				return cursor_pageup();

			if (y == bf->ysize + 1)
				return cursor_pagedown();

			cursor_moveto(bf->linestarts[bf->numlines * (y - 1) /
				(bf->ysize-2)]);
			return cursor_linestart();
		}
		menu_line_do(bf->mw, get_edit_menu_ptr(), MOUSE);
		return;
	}
	
	/* 3. We activate the buffer the pointer is on. If successful
	 *    MOUSE is returned the keyboard stack.
	 */

	wm_select_from_mouse();

#endif

}

/* EOF */
