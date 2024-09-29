/* dialog.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

#define MAXLEN 100

void dialog_window_delete(void);

static WINDOW *wf, *w;
static int xs, ys,		/* total size of window, includes border */
    xp, yp;			/* top left corner of window */

static int old_curs_set = ERR;

static FLAG dialog_need_clear = FALSE;

static u_char buffer[MAXLEN + 1];
static int homey, homex;


/************************************************************
 * Return the size of a char. Printable ascii ($20..$7f) is
 * printed as a single char; others are 2 char hex values
 */

static inline int
dcsize(int c)
{
	return (c < ' ' || c >= 0x7f) ? 2 : 1;
}

/*****************************************************
 * We have a special getkey for dialogs. This lets 
 * dialogs move windows, handle a terminal-resize
 * and expand macros.
 */

static KEY
dialog_getkey(void)
{
	KEY k;

      LOOP:
	k = ved_getkey(w);


	/* UGLY! I don't like the fact that there is a separate 'table'
	 * here, but don't know how else to handle it. Other commands are
	 * not supported in dialogs, so we only do a limited subset....
	 * move, resize and macros.
	 */

	if (k == MOVE_WINDOW) {
		move_window_do(w, wf);
		goto LOOP;

	} else if (k == TERMINAL_REFRESH) {
		dialog_window_delete();
		if (w) {
			delwin(w);
			w = NULL;
		}
		if (wf) {
			delwin(wf);
			wf = NULL;
		}
		retkey(TERMINAL_REFRESH);
		return KEYBOARD_ABORT;

	} else if (k == MACRO_FILENAME)
		macro_filename();

	else if (k == MACRO_FILE_BASE)
		macro_file_base();

	else if (k == MACRO_FILE_EXT)
		macro_file_ext();

	else if (k == MACRO_NUMBER)
		macro_number();
	else if (k == MACRO_TIME1)
		macro_time1();
	else if (k == MACRO_TIME2)
		macro_time2();
	else if (k == MACRO_START)
		macro_start();
	else
		return k;

	/* If we get here it WAS a special key (macro), go we
	 * need to get another one. Since it may too be special
	 * some recursion is appropriate....
	 */

	return dialog_getkey();
}

/**************************************************
 * Create the message window. If it already exists
 * we just have to make it visible on the internal
 * window stack.
 */

static void
dialog_window_create(void)
{
	if (w == NULL) {
		xs = xsize * 2 / 3;
		if (xs < 40)
			xs = xsize;
		ys = 7;
		if (ys > ysize)
			ys = ysize;

		if (dialog_pos < 1 || dialog_pos > 9)
			dialog_pos = 5;

		switch (dialog_pos) {
			case 1:
				xp = 0;
				yp = 0;
				break;

			case 2:
				xp = (xsize - xs) / 2;
				yp = 0;
				break;

			case 3:
				xp = xsize - xs;
				yp = 0;
				break;

			case 4:
				xp = 0;
				yp = (ysize - ys) / 2;
				break;

			case 5:
				xp = (xsize - xs) / 2;
				yp = (ysize - ys) / 2;
				break;

			case 6:
				xp = xsize - xs;
				yp = (ysize - ys) / 2;
				break;

			case 7:
				xp = 0;
				yp = ysize - ys;
				break;

			case 8:
				xp = (xsize - xs) / 2;
				yp = ysize - ys;
				break;


			case 9:
				xp = xsize - xs;
				yp = ysize - ys;
				break;

		}

		if (xp + xs >= xsize)
			xp = xsize - xs;

		if (yp + ys >= ysize)
			yp = ysize - ys;

		wf = newwin(ys, xs, yp, xp);
		if (wf != NULL) {
			wbkgdset(wf, display_attrs[DIALOG_BORDER] | ' ');
			box(wf, 0, 0);
			xp++;
			yp++;
			xs -= 2;
			ys -= 2;
		}
		w = newwin(ys, xs, yp, xp);

		if (w == NULL) {
			xp = 0;
			yp = 0;
			ys = 2;
			xs = 10;
			w = newwin(ys, xs, yp, xp);
			if (w == NULL)
				ved_exit("Couldn't allocate a message window. Fatal error!");
		}
		wbkgdset(w, display_attrs[DIALOG_NORMAL] | ' ');
		werase(w);

		scrollok(w, TRUE);
		keypad(w, TRUE);
		meta(w, TRUE);
	}
	if (wf)
		wm_add(wf);
	wm_add(w);
	wm_refresh();

	if (dialog_need_clear == TRUE) {
		werase(w);
		dialog_need_clear = FALSE;
	}
	old_curs_set = curs_set(1);
}

void
dialog_clear(void)
{
	dialog_need_clear = TRUE;
}

/********************************
 * Print a character in a dialog.
 */

static void
dialog_addch(int key)
{
	if (dcsize(key) == 2) {
		wbkgdset(w, display_attrs[DIALOG_HIGHLIGHT] | ' ');
		wprintw(w, "%02X", key);
		wbkgdset(w, display_attrs[DIALOG_NORMAL] | ' ');
	} else
		waddch(w, key);
}

/*********************************************************************
 * This really doesn't delete the window. It stays for the duration of the
 * session. We delete it from the internal window stack; then the
 * refresh overwrites it.
 */

void
dialog_window_delete(void)
{
	if (wf)
		wm_delete(wf);
	if (w)
		wm_delete(w);

	wm_refresh();

	if (old_curs_set != ERR)
		old_curs_set = curs_set(1);
}

/******************************************************
 * Print a line into a dialog window with word wrap
 */


static void
dialog_wrap_print(u_char * s, u_char * end)
{
	while (s <= end)
		dialog_addch(*s++);
}


static void
dialog_wrap_message(u_char * s)
{
	u_char *lastb = NULL;
	u_char *p;
	int xx;

	for (p = s, xx = 0;;) {
		if (xx >= xs - 1) {
			if (lastb == NULL)
				lastb = s;
			dialog_wrap_print(p, lastb);
			xx = 0;
			p = lastb + 1;
			s = p;
			lastb = NULL;
			if (*s != '\0')
				waddch(w, '\n');
		}
		if (*s == ' ')
			lastb = s;

		if (*s == '\0') {
			dialog_wrap_print(p, s - 1);
			break;
		}
		xx += dcsize(*s++);
	}
	wrefresh(w);
}


/*********************************************************************
 * Add a line to the message window. No pause. This is needed
 * if you want multiple line messages. You can't just include
 * a LF in the text since this will get converted to whatever
 * the print displayer wants it to display as.
 */

void
dialog_addline(char *s,...)
{
	va_list ap;
	u_char buf[200];

	dialog_window_create();

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf) - 2, s, ap);
	va_end(ap);
	dialog_wrap_message(buf);
	waddch(w, '\n');
}

void
dialog_adddata(char *s,...)
{
	va_list ap;
	u_char buf[200];

	dialog_window_create();

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf) - 2, s, ap);
	va_end(ap);
	dialog_wrap_message(buf);
	wrefresh(w);
}

/*********************************************************************
 * Display a message in a centered window. This is used for errors
 * and various status messages.
 * The program is halted until a keypress is entered
 */

int
dialog_msg(char *s,...)
{
	va_list ap;
	u_char buf[200];
	KEY k;

	dialog_window_create();

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf) - 2, s, ap);
	va_end(ap);
	dialog_wrap_message(buf);
	waddch(w, ' ');
	wrefresh(w);
	doupdate();

	k = dialog_getkey();

	if (w)
		waddch(w, '\n');
	dialog_window_delete();

	return v_tolower(k);
}


/*********************************************************************
 * Print a prompt and get a string
 * The returned string is stored in a static
 * buffer...the caller should copy it to a safe
 * area PDQ.
 */
static inline KEY
window_input_mvyx(int offset)
{
	u_char *s = buffer;
	int xx = homex;
	int yy = homey;

	while (offset--) {
		xx += dcsize(*s++);
		if (xx >= xs) {
			xx = 0;
			yy++;
		}
	}
	wmove(w, yy, xx);
	doupdate();
	return dialog_getkey();
}

static void
window_input_erase(void)
{
	int count;

	wmove(w, homey, homex);
	for (count = MAXLEN; count--;)
		dialog_addch(' ');
}

static void
window_input_show(void)
{
	u_char *s = buffer;

	window_input_erase();

	wmove(w, homey, homex);
	while (*s)
		dialog_addch(*s++);
}

u_char *
dialog_entry(u_char * s,...)
{
	va_list ap;
	int offset, t, buflen, maxlen;
	KEY k;

	if (get_pending_key() == EOL) {
		ved_getkey(bf->win);
		buffer[0] = '\0';
		return buffer;
	}
	dialog_window_create();
	set_history_pointer();

	/* print the prompt */

	va_start(ap, s);
	vsnprintf(buffer, MAXLEN - 2, s, ap);
	va_end(ap);
	dialog_wrap_message(buffer);
	dialog_addch(' ');

	/* Clear the region for the input. The getyx()
	 * is needed to set the homey/x globals. These point
	 * to the 'home' position of the string (right after
	 * the prompt. The erase routine does it's work from
	 * homey/x.
	 */

	getyx(w, homey, homex);
	window_input_erase();

	/* We've displayed an empty input string. Now determine the
	 * start position. If we never scrolled this would not be
	 * necessary. But scrolling does have it's problems...mainly
	 * that the homey pos can change by one or more lines.
	 * All we do is to backup the number of chars. advanced by erase...
	 */

	getyx(w, homey, homex);

	for (t = MAXLEN - 1; t--;) {
		if (homex == 0) {
			homex = xs - 1;
			homey--;
		} 
		homex--;
	} 

	buffer[0] = 0;
	buflen = 0;
	offset = 0;
	maxlen = MAXLEN - 1;

	/* Now the cursor is set. We'll do all future cursor positioning
	 * using the xoffset and the homex/y.
	 */


	while (1) {
		k = window_input_mvyx(offset);

		if (k == QUIT || k == KEYBOARD_ABORT) {
			offset = -1;
			goto EXIT;
		}
		if (k == EOL)
			goto EXIT;

		if (k == CURSOR_LEFT) {

			if (offset)
				offset--;
			continue;
		}
		if (k == CURSOR_LINESTART) {
			offset = 0;
			continue;
		}
		if (k == CURSOR_RIGHT) {
			if (offset < buflen)
				offset++;
			continue;
		}
		if (k == CURSOR_LINEEND) {
			offset = buflen;
			continue;
		}
		if (k == INSERT) {
			if (buflen <= maxlen - 1 && offset < buflen) {
				memmove(&buffer[offset + 1], &buffer[offset],
					buflen - offset);
				buffer[offset] = ' ';
				buflen++;
				window_input_show();
			}
			continue;
		}
		if (k == BACKSPACE) {
			if (offset)
				offset--;
			k = DELETE;
		}
		if (k == DELETE) {
			if (buflen && offset < buflen) {
				maxlen -= dcsize(buffer[offset]) -
				    dcsize(' ');
				memmove(&buffer[offset], &buffer[offset + 1],
					buflen - offset);
				buffer[buflen] = 0;
				buflen--;
				window_input_show();
			}
			continue;
		}
		if (k == CURSOR_UP || k == CURSOR_DOWN) {
			u_char *p;
			if ((p = get_next_history(buffer, (k == CURSOR_UP) ?
						  -1 : 1)) != NULL) {
				save_history(buffer);
				window_input_erase();
				strncpy(buffer, p, MAXLEN - 1);
				buffer[MAXLEN - 1] = 0;
				buflen = strlen(buffer);
				maxlen = MAXLEN - 1;
				for (t = 0; t < buflen; t++)
					maxlen -= dcsize(buffer[t]) - dcsize(' ');
				if (maxlen < buflen)
					buflen = maxlen;
				offset = buflen;
				window_input_show();
			}
			continue;
		}
		if (k == HEXINPUT)
			k = get_hex_input(w);

		if (k == INSERT_TAB)
			k = TAB;

		if (k > 0xff)
			continue;


		if (offset >= maxlen)
			break;

		if (offset < buflen)
			maxlen -= dcsize(buffer[offset]) - dcsize(k);

		buffer[offset] = k;

		if (offset == buflen && buflen < maxlen)
			buflen++;

		buffer[buflen] = 0;

		if (offset < buflen)
			offset++;

		window_input_show();
	}

      EXIT:

	if (w) {
		window_input_show();
		waddch(w, '\n');
		dialog_window_delete();
		save_history(buffer);
	}
	return (offset >= 0) ? buffer : NULL;

}


/* EOF */
