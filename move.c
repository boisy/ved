/* move.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

static WINDOW *feedback;

void resize_window_do(WINDOW * w, WINDOW * mw);
void move_window_do(WINDOW * w, WINDOW * mw);

/*********************************************************
 * These routines manage a small window which is overlayed
 * at the top of the window stack. It displays the current
 * window size or position while resizeing/moving.
 */

static void
make_feedback_window(char *help[], int num)
{
	int t, longest, n, y, x, yz, xz;

	for (t = 0, longest = 0; t < num; t++) {
		n = strlen(help[t]);
		if (n > longest)
			longest = n;
	}

	yz = num + 4;
	xz = longest + 2;
	y = (ysize - yz) / 2;
	x = (xsize - xz) / 2;

	feedback = newwin(yz, xz, y, x);

	if (feedback) {
		wbkgdset(feedback, display_attrs[DIALOG_NORMAL] | ' ');
		werase(feedback);
		for (t = 0; t < num; t++)
			mvwaddstr(feedback, t + 3, 1, help[t]);
		wm_add(feedback);
	}
}

static void
feedback_msg(int y, int x, char sep)
{
	if (feedback) {
		wmove(feedback, 1, 1);
		wprintw(feedback, "%2d %c %-3d", y, sep, x);
	}
}

static void
delete_feedback_window(void)
{
	if (feedback) {
		wm_delete(feedback);
		delwin(feedback);
		refresh_all();
	}
}


/************************************************************
 * Move a window to a new position. The first entry
 * point is for the command key jump table. We need
 * the main routine so we can call for other windows.
 *
 * The menu window (mw) need not exist or move (mw==NULL)
 */


void
move_window(void)
{
#ifdef NCURSES_VERSION
	move_window_do(bf->win, bf->mw);
#endif
}


void
move_window_do(WINDOW * w, WINDOW * mw)
{
#ifdef NCURSES_VERSION
	chtype k;
	int  x, y, xz, yz;
	static char *help[] =
	{
		"Arrow keys move window",
		"T - Top",
		"B - Bottom",
		"C - Centered",
		"L - Flush Left",
		"R - Flush Right"
	};

	make_feedback_window(help, arraysize(help));

	getmaxyx(mw, yz, xz);
	getbegyx(mw, y, x);

	while (1) {
		feedback_msg(y, x, ',');
		wm_refresh();
		doupdate();
		k = ved_getkey(w);
		switch (v_tolower(k)) {
			case CURSOR_UP:
				if (y)
					y--;
				break;

			case CURSOR_DOWN:
				if (y < ysize - yz)
					y++;
				break;

			case CURSOR_LEFT:
				if (x)
					x--;
				break;

			case CURSOR_RIGHT:
				if (x < xsize - xz)
					x++;
				break;

			case 't':
				y = 0;
				break;

			case 'b':
				y = ysize - yz;
				break;

			case 'l':
				x = 0;
				break;

			case 'r':
				x = xsize - xz;
				break;

			case 'c':
				y = (ysize - yz) / 2;
				x = (xsize - xz) / 2;
				break;

			default:
				delete_feedback_window();
				return;
		}
		mvwin(mw, y, x);
		mvwin(w, y + 1, x + get_scroll_indicator_offset());
	}
#endif
}

/************************************************************
 * Resize a window. The first entry 
 * point is for the command key jump table. We need
 * the main routine so we can call for other windows.
 *
 * The menu window (mw) need not exist or move (mw==NULL)
 */


void
resize_window(void)
{

#ifdef NCURSES_VERSION

	int x, y;

	resize_window_do(bf->win, bf->mw);

	getmaxyx(bf->win, y, x);
	bf->ysize = y;
	bf->xsize = x;

	werase(bf->mw);
	create_edit_menu(bf->mw);
	display_filename();
	bf->scrollbar_pos = 1;

	if (bf->wrapwidth)
		bf->wrapwidth = x - 1;
	cursor_change_refresh();

#endif

}


void
resize_window_do(WINDOW * w, WINDOW * mw)
{

#ifdef NCURSES_VERSION

	chtype k;
	int  x, y, xz, yz;

	static char *help[] =
	{
		"Arrow keys size window",
		"M - max size",
		"F - half frame size",
		"V - vertical split",
		"H - horizontal split"
	};

	make_feedback_window(help, arraysize(help));



	getmaxyx(mw, yz, xz);
	getbegyx(mw, y, x);

	while (1) {
		feedback_msg(yz, xz, 'x');
		wm_refresh();
		doupdate();
		k = ved_getkey(w);
		switch (v_tolower(k)) {
			case CURSOR_UP:
				if (yz > MIN_Y_SIZE)
					yz--;
				else
					beep();
				break;

			case CURSOR_DOWN:
				if (yz < ysize)
					yz++;
				else
					beep();
				break;

			case CURSOR_LEFT:
				if (xz > MIN_X_SIZE)
					xz--;
				else
					beep();
				break;

			case CURSOR_RIGHT:
				if (xz < xsize)
					xz++;
				else
					beep();
				break;

			case 'm':
				yz = ysize;
				xz = xsize;
				break;

			case 'f':
				if ((ysize / 2) >= MIN_Y_SIZE)
					yz = ysize / 2;
				if ((xsize / 2) >= MIN_X_SIZE)
					xz = xsize / 2;
				break;

			case 'h':
				if ((ysize / 2) >= MIN_Y_SIZE)
					yz = ysize / 2;
				xz = xsize;
				break;

			case 'v':
				if ((xsize / 2) >= MIN_X_SIZE)
					xz = xsize / 2;
				yz = ysize;
				break;

			default:
				delete_feedback_window();
				return;
		}

		
		if (xz + x > xsize) {
			x = xsize - xz;
			mvwin(mw, y, x);
			mvwin(w, y + 1, x + get_scroll_indicator_offset());
		}
		if (yz + y > ysize) {
			y = ysize - yz;
			mvwin(mw, y, x);
			mvwin(w, y + 1, x + get_scroll_indicator_offset());
		}
		wresize(mw, yz, xz);
		wresize(w, yz - 2, xz - get_scroll_indicator_change());
	}
#endif
}

/* EOF */
