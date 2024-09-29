/* sigwinch.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include <signal.h>
#include <sys/ioctl.h>

void
sigwinch_handler(int n)
{
	keyboard_kill_pending();
	retkey(TERMINAL_REFRESH);
	signal(SIGWINCH, sigwinch_handler);
}

void
terminal_refresh(void)
{
	int n, yz, xz, yp, xp;
	int oldx, oldy;
	
#ifdef NCURSES_VERSION

	struct winsize size;

	oldx = xsize;
	oldy = ysize;
	
		
	if (ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) {
		resizeterm(size.ws_row, size.ws_col);

		ysize = LINES;
		xsize = COLS;

	}
#endif
	/* This is not really very nice, but I don't see how
	 * else to handle a situation where the terminal gets
	 * to be too small.
	 */

	if (xsize < MIN_X_SIZE || ysize < MIN_Y_SIZE) {
		deathtrap(SIGABRT);
	}
	
	if (num_buffers) {
		for (n = 0; n < num_buffers; n++) {

			bf = &buffers[n];
			
#ifdef NCURSES_VERSION

			getbegyx(bf->mw, yp, xp);
			getmaxyx(bf->mw, yz, xz);

			if (xz >= xsize)
				xz = xsize;
			if (yz >= ysize)
				yz = ysize;
			if (yp + yz >= ysize)
				yp = ysize - yz;
			if (xp + xz >= xsize)
				xp = xsize - xz;

			/* We resize with checking to see if the old
			 * size changed. This is good, it is possible
			 * that the resizeterminal() above changed
			 * the window sizes on us as well...in which
			 * case bf->x/ysize could be wrong. Resizing
			 * to the same size does no harm...nor does
			 * moving to the same location.
			 */

			wresize(bf->mw, yz, xz);
			mvwin(bf->mw, yp, xp);
			
			wresize(bf->win, yz - 2, xz - get_scroll_indicator_change());
			mvwin(bf->win, yp + 1, xp + get_scroll_indicator_offset());

			bf->ysize = yz - 2;
			bf->xsize = xz - get_scroll_indicator_change();
#endif
			werase(bf->mw);
			create_edit_menu(bf->mw);
			display_filename();
			bf->scrollbar_pos = 1;

			if (bf->wrapwidth)
				bf->wrapwidth = bf->xsize - 1;
			cursor_change_refresh();
		}
		bf = &buffers[cur_buffer];
	}
	refresh_all();
}

/* EOF */
