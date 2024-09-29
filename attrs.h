/* attrs.h */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#define MAX_DISPLAY_CHARS 5

typedef struct {
	u_char a_area;		/* attribute area for this character */
	u_char count;		/* number of char cells to display */
	u_char chars[MAX_DISPLAY_CHARS];	/* display cells */
	FLAG deft;		/* changed from default true/false */
} DISTAB;

/* Symbolic names for the various window areas used. */

enum {
	THE_BACKGROUND, DIR_NORMAL, DIR_HIGHLIGHT, DIR_MENU, DIR_MENU_HIGHLIGHT,
	EDITOR_NORMAL, EDITOR_HIGHLIGHT, EDITOR_MENU, EDITOR_MENU_HIGHLIGHT,
	DIALOG_BORDER, DIALOG_NORMAL, DIALOG_HIGHLIGHT,
	DISPLAY_AREAS
};


#ifndef ATTRS
extern
#endif
DISTAB display_tab[256];

#ifndef ATTRS
extern
#endif
chtype display_attrs[DISPLAY_AREAS];


static inline int
csize(int c, int x)
{
	if ((c == EOL) && (opt_nocr_display == FALSE))
		c = ' ';
	return c == TAB ? (tabplus(x)) : display_tab[(c)].count;
}
