/* colors.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#define ATTRS
#include "attrs.h"
#undef ATTRS



/************************************************************************
 * Character sets supported. Doen't effect what 'chars' are really
 * displayed, but controls which are printable as single or 2digit-hex
 */

enum {
	CHARSET_PC, CHARSET_LATIN1, CHARSET_ASCII
};


static NAMETABLE charsets[] =
{
	{"PC", CHARSET_PC},
	{"Latin1", CHARSET_LATIN1},
	{"Ascii", CHARSET_ASCII},
	{0}
};

static int attr_charset;

/***************************************************
 * This stores the colorpairs/attributes for
 * various areas of the display. They are
 * set/modified with the display_change_attr()
 * routine. Functions which print to the
 * screen call display_attr() before printing.
 */


static NAMETABLE colors[] =
{
	{"Black", COLOR_BLACK},
	{"Red", COLOR_RED},
	{"Green", COLOR_GREEN},
	{"Yellow", COLOR_YELLOW},
	{"Blue", COLOR_BLUE},
	{"Magenta", COLOR_MAGENTA},
	{"Cyan", COLOR_CYAN},
	{"White", COLOR_WHITE},
	{0}
};

static NAMETABLE attrs[] =
{
	{"Normal", A_NORMAL},
	{"Standout", A_STANDOUT},
	{"Underline", A_UNDERLINE},
	{"Reverse", A_REVERSE},
	{"Blink", A_BLINK},
	{"Dim", A_DIM},
	{"Bold", A_BOLD},
	{0}
};

static NAMETABLE areas[] =
{
	{"The_background", THE_BACKGROUND},
	{"Dir_Normal", DIR_NORMAL},
	{"Dir_Highlight", DIR_HIGHLIGHT},
	{"Dir_Menu", DIR_MENU},
	{"Dir_Menu_HighLight", DIR_MENU_HIGHLIGHT},
	{"Editor_Normal", EDITOR_NORMAL},
	{"Editor_Highlight", EDITOR_HIGHLIGHT},
	{"Editor_Menu", EDITOR_MENU},
	{"Editor_Menu_Highlight", EDITOR_MENU_HIGHLIGHT},
	{"Dialog_Border", DIALOG_BORDER},
	{"Dialog_Normal", DIALOG_NORMAL},
	{"Dialog_Highlight", DIALOG_HIGHLIGHT},
	{0}
};

void
init_colors(void)
{
	int t;
	int color;
	int defs[][4] =
	{
		{DIR_NORMAL, COLOR_BLUE, COLOR_WHITE, A_NORMAL},
		{DIR_HIGHLIGHT, COLOR_BLUE, COLOR_WHITE, A_REVERSE},
		{DIR_MENU, COLOR_BLACK, COLOR_WHITE, A_NORMAL},
		{DIR_MENU_HIGHLIGHT, COLOR_BLACK, COLOR_BLUE, A_REVERSE},
		{EDITOR_NORMAL, COLOR_WHITE, COLOR_WHITE, A_NORMAL},
		{EDITOR_HIGHLIGHT, COLOR_WHITE, COLOR_WHITE, A_REVERSE},
		{EDITOR_MENU, COLOR_BLACK, COLOR_WHITE, A_NORMAL},
	     {EDITOR_MENU_HIGHLIGHT, COLOR_WHITE, COLOR_BLUE, A_REVERSE},
		{DIALOG_BORDER, COLOR_MAGENTA, COLOR_CYAN, A_NORMAL},
		{DIALOG_NORMAL, COLOR_BLACK, COLOR_YELLOW, A_NORMAL},
		{DIALOG_HIGHLIGHT, COLOR_BLACK, COLOR_YELLOW, A_REVERSE}
	};


	for (t = 0; t < arraysize(defs); t++) {
		if (defs[t][1] == defs[t][2])
			color = 0;
		else
			color = COLOR_PAIR((defs[t][1] * COLORS) + defs[t][2]);

		display_attrs[defs[t][0]] = color | defs[t][3];
	}
}

/**********************************************
 * Initialize the display conversion table
 */

char *
set_char_set(char *s)
{
	int t, cs;

	cs = lookup_value(s, charsets);

	if (cs < 0)
		return "UNKNOWN CHARSET";

	attr_charset = cs;

	/* Set all to 2-digit hex, reverse video */

	for (t = 0; t <= 0xff; t++) {
		display_tab[t].count = 2;
		display_tab[t].a_area = EDITOR_HIGHLIGHT;
		sprintf(display_tab[t].chars, "%02X", (unsigned)t);
		display_tab[t].deft = TRUE;
	}


	switch (cs) {

		case CHARSET_PC:

			/* Set $80..$a0 for PC set.  This is only option
			 * which sets  $7f (DEL) to be printable
			 */

			for (t = 0x20; t <= 0xff; t++) {
				display_tab[t].count = 1;
				display_tab[t].a_area = EDITOR_NORMAL;
				display_tab[t].chars[0] = t;
			}
			break;


		case CHARSET_LATIN1:

			/* $20..$7e, $a1..$ff for PC and LATIN1.         */

			for (t = 0x20; t <= 0x7e; t++) {
				display_tab[t].count = 1;
				display_tab[t].a_area = EDITOR_NORMAL;
				display_tab[t].chars[0] = t;
			}
			for (t = 0xa1; t <= 0xff; t++) {
				display_tab[t].count = 1;
				display_tab[t].a_area = EDITOR_NORMAL;
				display_tab[t].chars[0] = t;
			}
			break;

		default:	/* CHARSET_ASCII and anything else */

			/* Space to $7e  */

			for (t = 0x20; t < 0x7f; t++) {
				display_tab[t].count = 1;
				display_tab[t].a_area = EDITOR_NORMAL;
				display_tab[t].chars[0] = t;
			}

	}

	/* Special handling for TAB, we don't set EOL since this
	 * is handled specially by the display routine.
	 */

	display_tab[TAB].count = 0;
	display_tab[TAB].chars[0] = ' ';
	display_tab[TAB].a_area = EDITOR_NORMAL;

	display_tab['\n'].deft = FALSE;		/* for eg. in rc file */

	/* This refreshes all windows, if we've changed something
	 * in the display this will force it to be reflected.
	 */

	if (num_buffers > 0) {
		for (t = 0; t < num_buffers; t++) {
			bf = &buffers[t];
			cursor_change_refresh();
		}
		bf = &buffers[cur_buffer];
	}
	return NULL;
}



char *
get_rc_charset(void)
{
	return lookup_name(attr_charset, charsets);

}

/***************************************************
 * Advance a string pointer past a word and following whitespace.
 */

static char *
display_advance(char *p)
{
	while (*p > ' ')
		p++;
	return skip_white(p);
}


/***************************************************
 * To set the current display attributes for a window we call
 * display_change_attr(str). str is a char pointer to the
 * name of the area and the foreground, backgound and attributes.
 * ie. display_change_attr("DIALOG_FRAME red white normal")
 * RETURN: -1 if error, 0 success
 */

char *
display_change_attr(char *s)
{
	int attr, fg, bg, area;

	s = skip_white(s);

	area = lookup_value(s, areas);
	if (area == -1)
		return "UNKNOWN/ILLEGAL AREA";

	s = display_advance(s);
	if ((fg = lookup_value(s, colors)) == -1)
		fg = COLOR_BLACK;

	s = display_advance(s);
	if ((bg = lookup_value(s, colors)) == -1)
		bg = COLOR_WHITE;

	s = display_advance(s);
	if ((attr = lookup_value(s, attrs)) == -1)
		attr = A_NORMAL;

	if (bg != fg)
		attr |= COLOR_PAIR((fg * COLORS) + bg);

	display_attrs[area] = attr;

	return NULL;
}

char *
display_change_character(char *s)
{
	int t, ch, val, err, area, c[MAX_DISPLAY_CHARS];

	s = skip_white(s);

	s = get_exp(&ch, s, &err);

	if (ch < 0 || ch > 0xff || err != 0)
		return "ILLEGAL/UNKNOWN CHARACTER";
	;
	s = skip_white(s);
	if ((area = lookup_value(s, areas)) == -1)
		return "ILLEGAL/UNKNOWN AREA";

	s = display_advance(s);

	for (t = 0; t < MAX_DISPLAY_CHARS; t++) {
		s = skip_white(s);
		if (*s == 0)
			break;
		s = get_exp(&val, s, &err);
		if (err)
			return "ILLEGAL DISPLAY VALUE";;
		c[t] = val;
	}

	display_tab[ch].a_area = area;
	display_tab[ch].count = t;
	display_tab[ch].deft = FALSE;
	for (t = 0; t < MAX_DISPLAY_CHARS; t++)
		display_tab[ch].chars[t] = c[t];

	return NULL;
}

void
rc_print_areas(FILE * f, char *name)
{
	int t, x;
	short c, cf, cb;

	fputs("# Color settings for regions\n"
	      "# Each region can be set with the background color,\n"
	      "# foreground color and attribute. NOTE that if the\n"
	      "# fore/background colors are the same the default\n"
	      "# terminal color will (probably) be used\n"
	      "# Areas: \t", f);

	for (t = 0, x = 0; t < arraysize(areas) - 1; t++) {
		fprintf(f, "%s, ", areas[t].name);
		x += strlen(areas[t].name) + 2;
		if (x > 50) {
			fprintf(f, "\n#\t");
			x = 0;
		}
	}

	fputs("\n# Colors:\t", f);
	for (t = 0, x = 0; t < arraysize(colors) - 1; t++) {
		fprintf(f, "%s, ", colors[t].name);
		x += strlen(colors[t].name) + 2;
		if (x > 50) {
			fprintf(f, "\n#\t");
			x = 0;
		}
	}


	fputs("\n# Attributes:\t", f);
	for (t = 0, x = 0; t < arraysize(attrs) - 1; t++) {
		fprintf(f, "%s, ", attrs[t].name);
		x += strlen(attrs[t].name) + 2;
		if (x > 50) {
			fprintf(f, "\n#\t");
			x = 0;
		}
	}
	fputs("\n\n", f);

	for (t = 0; t < DISPLAY_AREAS; t++) {
		fprintf(f, "%s %s\t", name, areas[t].name);

		c = PAIR_NUMBER(display_attrs[t]);
		if (c == 0)
			fprintf(f, "Black\tBlack");
		else {
			pair_content(c, &cb, &cf);
			fprintf(f, "%s\t%s", lookup_name(cf, colors),
				lookup_name(cb, colors));
		}
		fprintf(f, "\t%s\n", lookup_name(display_attrs[t] &
				      (A_ATTRIBUTES & ~A_COLOR), attrs));
	}
	putc('\n', f);

}

void
rc_print_colors(FILE * f, char *name)
{
	int t, n;

	fputs("# Color/attribute setting for characters are\n"
	      "# set with the character value, the attribute of\n"
	      "# the display area and the display characters. By\n"
	      "# default the chars $00..$1f and $80..$ff are set\n"
	      "# to a 2 char. hex string in a hilighted video,\n"
	      "# $20..$7f are set to their single character ascii\n"
	      "# values. Only changes from this are displayed below\n"
	      "# (all values in decimal). As an example the entry\n"
	      "# for a NEWLINE character is displayed.\n\n", f);


	for (t = 0; t < 0xff; t++) {
		if (display_tab[t].deft == FALSE) {
			fprintf(f, "%s %d\t", name, t);

			fprintf(f, "%s\t", lookup_name(display_tab[t].a_area,
						       areas));

			for (n = 0; n < display_tab[t].count; n++)
				fprintf(f, "%d ", display_tab[t].chars[n]);

			putc('\n', f);
		}
	}
	putc('\n', f);
}

/* EOF */
