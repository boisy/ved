/* rc.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include <ctype.h>

enum {
	RC_AREA, RC_CHARACTER, RC_DIRECTORY, RC_DEAD, RC_OPTION, RC_MACRO,
	RC_SPELL, RC_KEYS, RC_ALIAS, RC_STATUS, RC_INCLUDE
};

static NAMETABLE rcmajors[] =
{
	{"Area", RC_AREA},
	{"Character", RC_CHARACTER},
	{"Directory", RC_DIRECTORY},
	{"Dead", RC_DEAD},
	{"Option", RC_OPTION},
	{"Macro", RC_MACRO},
	{"Speller", RC_SPELL},
	{"Bind", RC_KEYS},
	{"Alias", RC_ALIAS},
	{"Status", RC_STATUS},
	{"Include", RC_INCLUDE},
};


static FILE *rc_log = NULL;

static char *
process_rc_line(char *s)
{
	char *s1;
	int t;

	/* delete tailing NL from line, leading blanks
	 * then ignore  blank lines and comments */

	s = skip_white(s);

	if (*s == '#' || *s == '\0')
		return NULL;

	s1 = strsep(&s, " \t");

	switch (lookup_value(s1, rcmajors)) {

		case RC_AREA:
			return display_change_attr(s);

		case RC_CHARACTER:
			return display_change_character(s);

		case RC_DIRECTORY:
			return directory_set_rc_options(s);

		case RC_DEAD:
			return death_set_rc(s);

		case RC_OPTION:
			return option_set(s);

		case RC_MACRO:
			return macro_define(s);

		case RC_SPELL:
			return spell_set(s);

		case RC_KEYS:
			return bindings_set(s);

		case RC_ALIAS:
			return alias_set(s);

		case RC_STATUS:
			return status_set(s);

		case RC_INCLUDE:
			s = skip_white(s);
			s1 = strsep(&s, " \t");
			t = processrc(s1);
			if (t == 0 && rc_log)
				fprintf(rc_log, "INCLUDE COMPLETE\n");
			return (t ? strerror(errno) : NULL);

		default:
			return "UNKNOWN COMMAND";
	}
}

/*************************************************
 * If/then processor.
 * In a rc file you can have IF condition value...value
 * Conditons:
 *	$xxx  - the value of the shell variable xxx
 *	
 */

static int
check_condition(char *s)
{
	char *s1, *value = NULL;

	s = skip_white(s);

	s1 = strsep(&s, " \t");

	if (*s1 == '$')
		value = getenv(s1 + 1);

	else if (strcasecmp(s1, "ext") == 0) {
		if (bf != NULL)
			value = bf->file_ext;
	}
	if (rc_log)
		fprintf(rc_log, "CONDITIONAL TEST: %s VALUE=%s\n", s1, s);

	if (value == NULL)
		return 1;

	while (1) {
		s1 = strsep(&s, ":");
		if (s1 == NULL || *s1 == '\0')
			break;
		if (strcasecmp(s1, value) == 0)
			return 0;
	}
	return 1;
}


/********************************************
 * Open the named rc file and process
 * 
 * returns 1  - named file does not exist or can not be opened
 * 0  - success
 */

int
processrc(char *filename)
{
	FILE *f;
	char buff[500], *s;
	int t;
	int lcount = 0;
	int inif = 0;

	f = fopen(tilde_fname(filename), "r");

	if (rc_log) {
		if (f == NULL)
			fprintf(rc_log, "DIDN'T OPEN %s\n", filename);
		else
			fprintf(rc_log, "PROCESSING: %s\n", filename);
	}
	if (f == NULL)
		return 1;

	while (fgets(buff, sizeof(buff) - 2, f)) {

		s = index(buff, 0x0a);
		if (s)
			*s = '\0';

		if (rc_log != NULL) {
			lcount++;
			fprintf(rc_log, "%d: %s\n", lcount, buff);
		}
		s = skip_white(buff);

		if ((strncasecmp(s, "if", 2) == 0) &&
		    ((s[2] == ' ') || (s[2] == '\t') || (s[2] == '\0'))) {

			if (inif) {
				if (rc_log)
					fprintf(rc_log, "NESTED IF, PROCESSING ABORTED\n");
				break;
			}
			t = check_condition(s + 3);

			if (rc_log)
				fprintf(rc_log, "CONDITONAL TEST: %s\n", t ?
					"FAIL" : "SUCCESS");

			if (t == 0) {
				inif++;
				continue;
			}
			while (1) {
				if ((fgets(buff, sizeof(buff) - 2, f)) == 0)
					break;

				if (strncasecmp(s, "endif", 5) == 0) {
					if (rc_log)
						fprintf(rc_log, "ENDIF %s", s);
					break;
				}
				if (rc_log)
					fprintf(rc_log, "SKIPPING: %s", buff);
			}
			continue;
		}
		if (strncasecmp(s, "endif", 5) == 0) {

			if (inif) {
				if (rc_log)
					fprintf(rc_log, "END OF CONDITONAL\n");
				inif = 0;
			} else {
				if (rc_log)
					fprintf(rc_log, "ENDIF WITHOUT IF\n");
			}
			continue;
		}
		s = process_rc_line(buff);

		if (s != NULL && rc_log)
			fprintf(rc_log, "RC ERROR: %s\n", s);
	}
	fclose(f);
	return 0;
}


/**********************************************
 * Open a rc file from the default options
 */

void
rcfile(int logging)
{
	if (logging)
		rc_log = fopen("vedrc.log", "a");
	else
		rc_log = NULL;

	/* 1. Find global file in /usr/local/etc or /etc */

	if (processrc("/usr/local/etc/vedrc"))
		processrc("/etc/vedrc");

	/* do user rc file in home dir */

	processrc("~/.vedrc");

	/* do local rc, either .vedrc or vedrc */

	if (processrc(".vedrc"))
		processrc("vedrc");

	if (rc_log != NULL) {
		fclose(rc_log);
		rc_log = NULL;
	}
}

/**********************************************
 * Read a new RC file. User enters filename.
 */

void
load_rc(void)
{
	char *s;

	dialog_clear();
	s = dialog_entry("Name of RC file to load:");
	if (s == NULL || *s == '\0')
		return;

	if (processrc(s))
		vederror("Can't open file %s, error %s", s, strerror(errno));
}


/**********************************************
 * Write current rc values to specified file
 */

void
create_rc(void)
{
	FILE *f;
	char *s;

	dialog_clear();
	s = dialog_entry("Save RC options to:");
	if (s == NULL || *s == '\0')
		return;

	f = open_outfile(s, PROMPT_APPEND);

	if (f == NULL)
		return;

	if (dialog_msg("Save AREA colors (Y/n):") == 'y')
		rc_print_areas(f, lookup_name(RC_AREA, rcmajors));

	if (dialog_msg("Save the  options (Y/n):") == 'y') {
		rc_print_directory(f, lookup_name(RC_DIRECTORY, rcmajors));
		rc_print_death(f, lookup_name(RC_DEAD, rcmajors));
		rc_print_options(f, lookup_name(RC_OPTION, rcmajors));
		rc_spell_options(f, lookup_name(RC_SPELL, rcmajors));
		rc_status_options(f, lookup_name(RC_STATUS, rcmajors));
	}
	if (dialog_msg("Save CHARACTER values (Y/n):") == 'y')
		rc_print_colors(f, lookup_name(RC_CHARACTER, rcmajors));

	if (dialog_msg("Save MACRO definitions (Y/n):") == 'y')
		rc_print_macros(f, lookup_name(RC_MACRO, rcmajors));

	if (dialog_msg("Save KEY BINDINGS (Y/n):") == 'y') {
		rc_print_bindings(f, lookup_name(RC_KEYS, rcmajors));
		rc_print_alias(f, lookup_name(RC_ALIAS, rcmajors));
	}
	fclose(f);
}

/***************************************************************
 * Create the status filename
 */

static char *
get_statusfile_name(void)
{
	char *base, *s, *name;
	static char *path = NULL;

	free(path);
	path = NULL;

	name = bf->filename;

	if ((name == NULL) || (*name == '\0'))
		return NULL;

	/* just use the basename for the filename */

	if ((s = rindex(name, '/')) != NULL)
		name = s + 1;

	if ((s = index(bf->filename, '/')) != NULL) {
		base = bf->filename;
		*s = '0';
	} else
		base = ".";

	asprintf(&path, "%s/%s/VED-%s", status_dir, base, name);
	if (s != NULL)
		*s = '/';	/* restore in filename */

	return path;
}

/*********************************************************
 * Write the options for a status file. This is called when
 * a file is saved.
 */


void
save_status(void)
{
	FILE *fp;
	char *name;

	if (status_dir == NULL)
		return;

	name = get_statusfile_name();

	if (name == NULL)
		return;

	fp = fopen(name, "w");
	if (fp == NULL)
		return;

	saving_status = TRUE;

	rc_print_options(fp, lookup_name(RC_OPTION, rcmajors));
	rc_status_options(fp, lookup_name(RC_STATUS, rcmajors));
	rc_print_macros(fp, lookup_name(RC_MACRO, rcmajors));

	saving_status = FALSE;
}



/********************************************************
 * Load status file after a new file is loaded.
 */

void
load_status(void)
{
	char *name;

	name = get_statusfile_name();
	if (name == NULL)
		return;


	loading_status = TRUE;
	processrc(get_statusfile_name());
	loading_status = FALSE;
}


/**********************************************
 * Enter a line from the keyboard and process
 * the line as an RC line.
 */

void
set_rc_line(void)
{
	char *s;

	dialog_clear();
	while (1) {
		s = dialog_entry("RC command:");
		if (s == NULL || *s == '\0')
			break;

		s = process_rc_line(s);
		if (s != NULL) {
			dialog_addline("Error, %s, in processing line", s);
			beep();
		}
	}

	/* Since we might have done something to effect the display
	 * like changing an attribute or display value we might
	 * as well update the screens.
	 */

	if (num_buffers > 0) {
		int t;
		for (t = 0; t < num_buffers; t++) {
			bf = &buffers[t];
			redisplay();
		}
		bf = &buffers[cur_buffer];
		wm_refresh();
	}
}

/* EOF */
