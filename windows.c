/* windows.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

/********************************************************
 * Display next/prior buffer/window
 */

void
windows_next(void)
{
	cur_buffer++;
	if (cur_buffer > num_buffers - 1)
		cur_buffer = 0;

	bf = &buffers[cur_buffer];
	wm_add(bf->mw);
	wm_add(bf->win);

	wm_refresh();
}

void
windows_prior(void)
{
	cur_buffer--;
	if (cur_buffer < 0)
		cur_buffer = num_buffers - 1;

	bf = &buffers[cur_buffer];
	wm_add(bf->mw);
	wm_add(bf->win);

	wm_refresh();
}

/**************************************************************
 * Create a new window and display it. No data in this window!
 */

void
windows_new(void)
{
	dialog_clear();
	if (num_buffers >= MAXBUFFS ) {
		vederror("You can only allocate %d buffers!", MAXBUFFS);
		return;
	}
	make_buffer();
	cur_buffer = num_buffers - 1;
	bf = &buffers[cur_buffer];
	set_filename(NULL);
	create_edit_menu(bf->mw);
	openbuff(1, 0);
	bf->buf[0] = EOL;
	wrap_buffer(bf->wrapwidth, 0);
	display_page(bf->curline);
	wm_refresh();
	bf->changed = FALSE;
}

/******************************************************************
 * Create a new window and grab a file.
 */

void
windows_file(void)
{
	windows_new();
	file_new();
}

/******************************************************************
 * Select a window from a pulldown menu.
 */

void
windows_raise(void)
{
	MENUITEM names[MAXBUFFS + 1];
	MENUPULL pull;
	static char *unnamed = "Not Named";
	int t, n, y, x;

	if (num_buffers <= 1)
		return;		/* can't select if only one buffer */

	/* create a pulldown menu with the names of the buffers */

	for (t = 0, x = 0; t < num_buffers; t++) {
		names[t].name = buffers[t].filename;
		if (names[t].name == NULL || names[t].name[0] == 0)
			names[t].name = unnamed;
		n = strlen(names[t].name);
		if (n > x)
			x = n;
		names[t].shortcut = t + '0';
		names[t].printfunc = NULL;
		names[t].func = NULL;
	}

	pull.title = NULL;
	pull.items = names;
	pull.numitems = num_buffers;
	pull.shortcut = 0;

	getbegyx(bf->win, y, x);
	t = pulldown_menu(y, last_menu_xpos, &pull,
			  EDITOR_MENU, EDITOR_MENU_HIGHLIGHT);

	if (t >= 0) {
		cur_buffer = t;
		bf = &buffers[cur_buffer];
		wm_add(bf->mw);
		wm_add(bf->win);
	}
	wm_refresh();
}

/*****************************************************************
 * Display as many windows as possible on the screen
 */

void
windows_showall(void)
{

#ifdef NCURSES_VERSION

	int ycells, xcells, n, y, x, yz, xz, numdisp, tx, ty;

	dialog_clear();

	for (ycells = 1, xcells = 1, numdisp = 1;;) {
		n = 0;
		if (numdisp < num_buffers && ysize / ycells >= MIN_Y_SIZE) {
			ycells++;
			numdisp = ycells * xcells;
			n = 1;
		}
		if (numdisp < num_buffers && xsize / xcells >= MIN_X_SIZE) {
			xcells++;
			numdisp = xcells * ycells;
			n = 1;
		}
		if (numdisp >= num_buffers || n == 0)
			break;
	}

	for (n = 0, x = 0, y = 0, yz = ysize / ycells, xz = xsize / xcells; n < num_buffers; n++) {

		bf = &buffers[n];

		ty = yz;
		tx = xz;

		if (y + ty + yz > ysize)
			ty = ysize - y;
		if (x + tx + xz > xsize)
			tx = xsize - x;

		if (n == num_buffers - 1) {
			ty = ysize - y;
			tx = xsize - x;
		}
		wresize(bf->mw, ty, tx);
		
		
		wresize(bf->win, ty - 2, tx - get_scroll_indicator_change());
		bf->ysize = ty - 2;
		bf->xsize = tx - get_scroll_indicator_change();
		mvwin(bf->mw, y, x);
		mvwin(bf->win, y + 1, x + get_scroll_indicator_offset());
		create_edit_menu(bf->mw);
		display_filename();

		cursor_change_refresh();

		wm_add(bf->mw);
		wm_add(bf->win);

		x += tx;
		if (x + tx > xsize) {
			x = 0;
			y += ty;
			if (y + yz > ysize)
				break;
		}
	}

	if (cur_buffer > n)
		cur_buffer = n;

	bf = &buffers[cur_buffer];
	wm_add(bf->mw);
	wm_add(bf->win);
	refresh_all();

	if (n < num_buffers - 1)
		dialog_msg("Only %d of %d buffers displayed!", n + 1, num_buffers);
		
#endif

}

/* EOF */
