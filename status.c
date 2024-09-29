/* status.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"


/*****************************************************
 * Read/write status options to RC files
 */

enum {
	RC_HISTORYFILE, RC_STATUSDIR, RC_HISTORYFORCE, RC_CURSOR
};

static NAMETABLE rc_status[] =
{
	{"History_File", RC_HISTORYFILE},
	{"History_Create", RC_HISTORYFORCE},
	{"Status_Dir", RC_STATUSDIR},
	{"Cursor", RC_CURSOR},
	{}
};



char *
status_set(char *s)
{
	char *s1, *p;

	s1 = strsep(&s, " \t");
	s = strsep(&s, " \t");

	if (s1 == NULL)
		return NULL;
		
	switch (lookup_value(s1, rc_status)) {

		case RC_HISTORYFILE:
			set_history_filename(translate_delim_string(s));
			return NULL;

		case RC_HISTORYFORCE:
			set_history_force(s);
			return NULL;

		case RC_STATUSDIR:
			free(status_dir);
			status_dir = NULL;
			p = translate_delim_string(s);
			if( p != NULL)
				status_dir = strdup(p);
			return NULL;

		case RC_CURSOR:
			if (loading_status == FALSE)
				return "CURSOR COMMAND IN RC FILE";
			if (bf != NULL) {
				int value, err;
				get_exp(&value, s, &err);
				if (value < bf->bsize && err == 0)
					cursor_moveto(value);
			}
			return NULL;

		default:
			return "UNKNOWN COMMAND";
	}
}


void
rc_status_options(FILE * f, char *name)
{

#define L(x) lookup_name(x, rc_status)

	if (saving_status == FALSE) {
		fputs("# Status options\n\n", f);

		fprintf(f, "%s\t%s\t%s\n", name, L(RC_HISTORYFILE),
			make_delim_string(get_history_filename()));

		fprintf(f, "%s\t%s\t%s\n", name, L(RC_HISTORYFORCE),
			truefalse(get_history_force()));

		fprintf(f, "%s\t%s\t%s\n", name, L(RC_STATUSDIR),
		make_delim_string(status_dir == NULL ? "" : status_dir));

		putc('\n', f);
		putc('\n', f);
	}
	if (saving_status == TRUE) {
		fprintf(f, "%s\t%s\t%d\n", name, L(RC_CURSOR), bf->curpos);
	}
#undef L
}


/* EOF */
