/* wm.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

void wm_delete(WINDOW * w);

/*********************************************************************
 * No errors are ever reported, nor should
 * any every occur <<FAMOUS LAST WORD>>.
 * 
 * Whenever an overlay window is created/destroyed register
 * the windows here and call wm_refresh(). Doing wm_refresh()
 * after a delete updates the stacked display. 
 * 
 * This reserves enough slots for each buffer plus
 * three each for the directory and dialog windows,
 * plus a spare just for safety.
 */

#define STACKSZ (MAXBUFFS*2) + 6

static WINDOW *wm[STACKSZ + 1];
static int wmtop = -1;

/*********************************************************************
 * Put a window on the top of the stack. If the
 * pointer is already registered it is deleted first.
 * If you want to move a window to the top level you just add it
 * since wm_add() first does a delete.
 */

void
wm_add(WINDOW * w)
{
	if (w) {
		wm_delete(w);
		wm[++wmtop] = w;
	}
}

/*********************************************************************
 * Delete the window from the stack (nothing
 * is done to the window itself
 */

void
wm_delete(WINDOW * w)
{
	int t;

	if (w) {
		for (t = 0; t <= wmtop; t++) {
			if (wm[t] == w) {
				memmove(&wm[t], &wm[t + 1],
				    sizeof(wm[0]) * (STACKSZ - (t + 1)));
				wmtop--;
				break;
			}
		}
	}
}

/*********************************************************************
 * Select a new buffer. This is called when the mouse is clicked
 * and it is not on the currently active edit screen.
 * We go down the stack, from the top, checking to see if the mouse
 * pointer is on the screen. If it is, we check the buffer pointers
 * in an attempt to match a screen with a buffer.
 */

void
wm_select_from_mouse(void)
{
	int t, n;

	for (t = wmtop; t >= 0; t--) {
		if (wenclose(wm[t], mousey, mousex) == TRUE) {
			for (n = 0; n < num_buffers; n++) {
				if (buffers[n].mw == wm[t] || buffers[n].win == wm[t]) {
					cur_buffer = n;
					bf = &buffers[cur_buffer];
					wm_add(bf->mw);
					wm_add(bf->win);
					retkey(MOUSE);
					return;
				}
			}
		}
	}
}

/*********************************************************************
 * Refresh the stack in bottom to top order
 */

void
wm_refresh(void)
{
	int t;

	wnoutrefresh(stdscr);
	for (t = 0; t <= wmtop; t++) {

		touchwin(wm[t]);
		wnoutrefresh(wm[t]);
	}
}

void
refresh_all(void)
{
	clear();
	wm_refresh();
	doupdate();
}



/* EOF */
