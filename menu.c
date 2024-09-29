/* menu.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

static int pull_positions[25];
static int pull_sizes[25];

/*********************************************************************
 * Get name (label) for a menu item in a pulldown.
 * 
 * The MENUITEM can have a label in it or a pointer to
 * a function which returns the label. This is useful for
 * dynamic items, but it means we have to do a lookup for
 * the text each time it's used.
 */

static char *
menu_getitem_name(MENUITEM * i)
{
	if (i->name != NULL)
		return i->name;

	if (i->printfunc == NULL)
		return "Unknown";	/* just to be safe! */
	return (*i->printfunc) ();
}

/*********************************************************************
 * Print the name of a pulldown menu in the menu bar
 */

static void
menu_printitem(WINDOW * w, MENU * m, int sel)
{
	wmove(w, 0, pull_positions[sel]);
	wprintw(w, "%-.*s", pull_sizes[sel], m->pls[sel].title);
}

/*********************************************************************
 * Print a single item in a pulldown.
 * The caller sets the display color/mode before calling
 */

static void
display_item(WINDOW * w, int y, char *s, int len)
{
	wmove(w, y, 1);
	for (; *s && *s != '\n'; len--)
		waddch(w, *s++);
	while (len-- > 0)
		waddch(w, ' ');
}

/*********************************************************************
 * Open a pulldown (scrollable) menu.
 * RETURN:
 *     -1 abort (abort key, unknow key, or mouseclick in invalid region)
 *     -2 left-arrow pressed
 *     -3 right-arrow pressed
 *      0... function value selected
 */

int
pulldown_menu(int ypos, int xpos, MENUPULL * pm, u_char normal_attr, u_char hi_attr)
{
	int maxlen, sel, t, x, y, topitem, botitem;
	WINDOW *w;
	KEY k;
	char *s;
	int old_curs_set;

	/* get length of longest item */

	for (t = 0, maxlen = 0; t < pm->numitems; t++) {
		if ((x = strlen(menu_getitem_name(&pm->items[t]))) > maxlen)
			maxlen = x;
	}
	maxlen += 2;

	/* create a window for the pulldown */

	topitem = 0;
	botitem = pm->numitems - 1;

	while (ypos && (botitem + ypos + 1) > ysize)
		ypos--;
	while ((botitem > 1) && (botitem + ypos + 1) >= ysize)
		botitem--;

	while (xpos && (xpos + maxlen + 3) >= xsize)
		xpos--;
	while ((maxlen > 1) && (xpos + maxlen + 3) >= xsize)
		maxlen--;

	w = newwin(botitem + 2, maxlen + 2, ypos, xpos);
	if (w == NULL)
		vederror("Unable to create menu pulldown");

	last_menu_xpos = xpos;

	werase(w);
	keypad(w, TRUE);
	meta(w, TRUE);

	old_curs_set = curs_set(0);

	wbkgdset(w, display_attrs[normal_attr] | ' ');
	box(w, 0, 0);
	wmove(w, 0, 0);
	waddch(w, ACS_VLINE);
	wmove(w, 0, maxlen + 1);
	waddch(w, ACS_VLINE);

	/* display the menu items in the pulldown */

	sel = 0;

      LOOP:
	for (t = topitem; t <= botitem; t++)
		display_item(w, t - topitem, menu_getitem_name(&pm->items[t]), maxlen);


	/* wait for a valid keypress/selection */

	while (1) {

		wbkgdset(w, display_attrs[hi_attr] | ' ');
		s = menu_getitem_name(&pm->items[sel]);
		display_item(w, sel - topitem, s, maxlen);
		wbkgdset(w, display_attrs[normal_attr] | ' ');
		wrefresh(w);
		k = v_tolower(ved_getkey(w));

		display_item(w, sel - topitem, s, maxlen);

		wrefresh(w);

		for (t = 0; t < pm->numitems; t++) {
			if (k == v_tolower(pm->items[t].shortcut)) {
				sel = t;
				goto EXIT;
			}
		}

		switch (k) {

			case CURSOR_LEFT:
				sel = -2;
				goto EXIT;

			case CURSOR_RIGHT:
				sel = -3;
				goto EXIT;

			case CURSOR_UP:
			
				if (sel==0) {
					while(sel != pm->numitems-1) {
						sel++;
						if(sel>botitem) {
							topitem++;
							botitem++;
						}
					}
					goto LOOP;
				}
				
				if (sel != 0)
					sel--;
				if (sel < topitem) {
					topitem--;
					botitem--;
					goto LOOP;
				}
				break;

			case CURSOR_DOWN:
			
				if(sel == pm->numitems-1) {
					while(sel != 0) {
						sel--;
						if(sel < topitem) {
							topitem--;
							botitem--;
						}
					}
					goto LOOP;
				}
				
				if (sel < pm->numitems - 1)
					sel++;
				if (sel > botitem) {
					topitem++;
					botitem++;
					goto LOOP;
				}
				break;

#ifdef NCURSES_VERSION
			case MOUSE:
				if (wenclose(w, mousey, mousex) == TRUE) {
					getbegyx(w, y, x);
					sel = mousey - y;
				} else {
					retkey(MOUSE);
					sel = -1;
				}
				goto EXIT;
#endif

			case '\n':
				goto EXIT;

			default:
				sel = -1;
				goto EXIT;

		}
	}

      EXIT:

	/* delete the pulldown menu */

	if (old_curs_set != ERR)
		curs_set(old_curs_set);

	delwin(w);
	wm_refresh();

	return sel;
}


/*********************************************************************
 * Create a new menu bar. Menu bars are separate windows from the main
 * screens. We did them as derived windows at one point, but the ncurses
 * code has problems when you start to move main windows with subwindows,
 * and even more problems when you start to do resizing. So, a menu bar
 * is printed in the frame (a full window 2 lines/rows larger that the
 * edit/dir window). Just remember to move/resize both the main window
 * and it's frame at the same time, and to re-create the menu after
 * a resize.
 *
 * After creating the bar, the the names of the pulldowns are printed.
 *
 * One big assumption is made...the last menu item is 'help' and that should
 * be right justified.
 *
 */

void
menu_create(WINDOW * mw, MENU * m)
{
	int t, x, n, spacing, max, mp, xz, yz;

	getmaxyx(mw, yz, xz);

	n = m->numpulls;
	for (t = 0, x = 0; t < n; t++) {
		pull_sizes[t] = strlen(m->pls[t].title);
		x += pull_sizes[t];
	}

	spacing = 1;
	if (xz - (x + (n * spacing)) > n * 2)
		spacing = 2;

	if (xz - (x + (n * spacing)) > n * 2)
		spacing = 3;

	while ((x + (n * spacing)) > xz) {
		for (t = 0, mp = -1, max = 0; t < n; t++) {
			if (pull_sizes[t] > max) {
				max = pull_sizes[t];
				mp = t;
			}
		}
		if (mp > -1) {
			x -= 1;
			pull_sizes[mp] -= 1;
		} else
			vederror("Unable to display menus");
	}

	for (t = 0, x = 1; t < n - 1; t++) {
		pull_positions[t] = x;
		x += pull_sizes[t] + spacing;
	}
	t = n - 1;
	pull_positions[t] = xz - pull_sizes[t] - 1;

	wmove(mw, 0, 0);
	wclrtoeol(mw);
	for (t = 0; t < n; t++)
		menu_printitem(mw, m, t);
}


/*********************************************************************
 * Loop entered when menu bar is made active. We check
 * keys/mouse and open the pulldowns
 * 
 * If key is NOT 0 then the pulldown menu assoicated with the key
 * is displayed.
 * 
 * If the key is MOUSE then a mouse click in the title line brought
 * us here...we see if the mouse was on a valid item.
 */

void
menu_line_do(WINDOW * w, MENU * m, KEY key)
{
	int sel, t, n, x, y;
	int old_curs_set;

	old_curs_set = curs_set(0);

	menu_create(w, m);
	sel = 0;


	/* determine initial pulldown to select. This is only done if 
	 * key != 0 (a shortcut key or a mouse click).
	 */

	if (key) {
		
#ifdef NCURSES_VERSION
		if (key == MOUSE) {
			getbegyx(w, y, x);
			if (mousey - y > 0 || mousex - x < 1 ||
			    wenclose(w, mousey, mousex - 1) == FALSE)
				goto EXIT;
			x = mousex - x;
		} else
#endif
			x = 0;

		for (sel = 0; sel < m->numpulls; sel++) {
			if (key == MOUSE) {
				if ((x >= pull_positions[sel]) &&
				    (x < pull_positions[sel] + pull_sizes[sel])) {
					key = '\n';
					break;
				}
			} else if (m->pls[sel].shortcut == key) {
				key = '\n';
				break;
			}
		}
		if (key != '\n')
			goto EXIT;
		retkey(key);
	}
	/* hilight a menubar entry. Pull it down on a downarrow or CR */

	while (1) {
		wbkgdset(w, display_attrs[m->attr_highlight] | ' ');
		menu_printitem(w, m, sel);
		wbkgdset(w, display_attrs[m->attr_normal] | ' ');
		wm_refresh();
		doupdate();
		key = ved_getkey(w);
		menu_printitem(w, m, sel);
		wm_refresh();
		doupdate();

		/* convert a keypress to a shortcut */

		for (n = 0; n < m->numpulls; n++) {
			if (m->pls[n].shortcut == v_tolower(key)) {
				sel = n;
				key = '\n';
				break;
			}
		}

		switch (key) {
			case KEYBOARD_ABORT:
				goto EXIT;

			case CURSOR_LEFT:
				if (sel > 0)
					sel--;
				break;

			case CURSOR_RIGHT:
				if (sel < m->numpulls - 1)
					sel++;
				break;

			case '\n':
			case CURSOR_DOWN:
				getbegyx(w, y, x);

				/* activate the menu */

				t = pulldown_menu(y + 1, x + pull_positions[sel],
					    &m->pls[sel], m->attr_normal,
						  m->attr_highlight);

				/* t == 0..n, activate selection */

				if (t >= 0) {
					(void)(*m->pls[sel].items[t].func) ();
					goto EXIT;
				}
				/* left arrow */

				if (t == -2) {
					retkey(CURSOR_DOWN);
					retkey(CURSOR_LEFT);
				}
				/* right arrow */

				else if (t == -3) {
					retkey(CURSOR_DOWN);
					retkey(CURSOR_RIGHT);
				}
				/* anything else...mouse in invalid
				 * region or non-valid key
				 */

				else
					goto EXIT;
				break;

			default:
				goto EXIT;
		}

	}

      EXIT:
	if (old_curs_set != ERR)
		curs_set(old_curs_set);
}

/* EOF */
