/* help.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

WINDOW *w, *wf;


static void
dohelp(int which, char *header)
{
	int t, n, ys, xs, yp, xp, maxkey, maxcomment, maxname;

	for (t = 0, ys = 2, maxkey = 0, maxcomment = 0, maxname = 0; t < NUMCMDS; t++) {
		if (cmdtable[t].menu == which) {
			ys++;
			n = strlen(cmdtable[t].cmdname);
			if (n > maxname)
				maxname = n;
			n = strlen(cmdtable[t].comment);
			if (n > maxcomment)
				maxcomment = n;
			n = strlen(cmdtable[t].keyname);
			if (n > maxkey)
				maxkey = n;
		}
	}

	xs = maxkey + maxcomment + maxname + 3;
	while (xs > COLS && maxcomment > 1) {
		xs--;
		maxcomment--;
	}

	yp = (LINES - ys) / 2;
	xp = (COLS - xs) / 2;

	if ((ys + 2 <= LINES) && (xs + 2 < COLS) && ((wf = newwin(ys + 2, xs + 2, yp - 1, xp - 1)) != NULL)) {
		wbkgdset(wf, display_attrs[DIR_MENU] | ' ');
		werase(wf);
		wrefresh(wf);
	}
	if (ys > LINES)
		vederror("Ysize of screen too small for help. Need %d lines", ys);

	w = newwin(ys, xs, yp, xp);
	if (w == NULL) {
		if (wf)
			delwin(wf);
		vederror("Can't allocate %dx%d window for help", ys, xs);
	}
	wbkgdset(w, display_attrs[DIR_NORMAL] | ' ');
	scrollok(w, FALSE);
	keypad(w, TRUE);
	meta(w, TRUE);

	werase(w);

	t = strlen(header);
	if (t > xs)
		t = xs;
	n = (xs - t) / 2;
	wprintw(w, "%*s%-*s\n\n", n, "", t, header);
	for (t = 0; t < NUMCMDS; t++) {
		if (cmdtable[t].menu == which) {
			wprintw(w, "%*s %-*s %-.*s\n", maxname, cmdtable[t].cmdname,
				maxkey, cmdtable[t].keyname,
				maxcomment, cmdtable[t].comment);
		}
	}
	wrefresh(w);
	wgetch(w);

	delwin(w);
	if (wf != NULL)
		delwin(wf);
	w = NULL;
	wf = NULL;
	wm_refresh();
}

void
help_block(void)
{
	dohelp(HELPSECT_BLOCK, "Block commands");
}

void
help_cursor(void)
{
	dohelp(HELPSECT_CURSOR, "Cursor movement commands");
}

void
help_delete(void)
{
	dohelp(HELPSECT_DELETE, "Deletion commands");
}

void
help_display(void)
{
	dohelp(HELPSECT_DISPLAY, "Display commands");
}

void
help_edit(void)
{
	dohelp(HELPSECT_EDIT, "Editing commands");
}

void
help_file(void)
{
	dohelp(HELPSECT_FILE, "File commands");
}

void
help_search(void)
{
	dohelp(HELPSECT_FIND, "Search/replace commands");
}

void
help_macros(void)
{
	dohelp(HELPSECT_MACROS, "Macro commands");
}

void
help_menus(void)
{
	dohelp(HELPSECT_MENU, "Help commands");
}

void
help_misc(void)
{
	dohelp(HELPSECT_MISC, "Miscellaneous commands");
}

void
help_spell(void)
{
	dohelp(HELPSECT_SPELL, "Spelling commands");
}

/* EOF */
