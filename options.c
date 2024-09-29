/* options.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

enum {
	RC_CR, RC_INDENT, RC_CMODE, RC_OVERWRITE, RC_DOBACKUPS,
	RC_BACKUPSUFFIX, RC_BACKUPPREFIX, RC_CLIPBOARD, RC_CASE,
	RC_TIME1, RC_TIME2, RC_AUTONUM, RC_DIALOGPOS,
	RC_TABWIDTH, RC_WILDCARD, RC_CHARSET, RC_SCROLL
};


static NAMETABLE option_table[] =
{
	{"Cr_display", RC_CR},
	{"Auto_indent", RC_INDENT},
	{"Cmode", RC_CMODE},
	{"Overwrite_mode", RC_OVERWRITE},
	{"Exact_case", RC_CASE},
	{"Dobackups", RC_DOBACKUPS},
	{"Backup_suffix", RC_BACKUPSUFFIX},
	{"Backup_prefix", RC_BACKUPPREFIX},
	{"Clipboard", RC_CLIPBOARD},
	{"Time_Macro_1", RC_TIME1},
	{"Time_Macro_2", RC_TIME2},
	{"Auto_Number", RC_AUTONUM},
	{"Dialog_pos", RC_DIALOGPOS},
	{"Wildcard", RC_WILDCARD},
	{"Tabwidth", RC_TABWIDTH},
	{"CharSet", RC_CHARSET},
	{"Scroll_indicator", RC_SCROLL},
	{0}
};

/***************************************************
 * Toggle the display of EOLs.
 *
 * Note that if the size of an EOL and SPACE differ
 * we have to redo the linewrap stuff.
 */

void
toggle_crdisplay(void)
{
	int t;

	if (opt_nocr_display == TRUE)
		opt_nocr_display = FALSE;
	else
		opt_nocr_display = TRUE;

	for (t = 0; t < num_buffers; t++) {
		bf = &buffers[t];

		if (display_tab[' '].count != display_tab[EOL].count)
			cursor_change_refresh();
		else
			redisplay();
	}
	bf = &buffers[cur_buffer];
}

char *
menutext_crdisplay(void)
{
	return (opt_nocr_display == FALSE) ? "hide eOls" : "display eOls";
}
/**********************************************
 * Toggle case match
 */

void
toggle_casematch(void)
{
	opt_casematch = (opt_casematch == TRUE) ? FALSE : TRUE;
	set_findmatch_table();
}

char *
menutext_casematch(void)
{
	return (opt_casematch == TRUE) ? "cAsematch off" : "cAsematch exact";
}


/**********************************************
 * Toggle overwrite
 */

void
toggle_overwrite(void)
{
	opt_insert = (opt_insert == TRUE) ? FALSE : TRUE;
}

char *
menutext_overwrite(void)
{
	return (opt_insert == TRUE) ? "Mode-overwrite" : "Mode-insert";
}


/**********************************************
 * Toggle auto indent mode
 */

void
toggle_autoindent(void)
{
	opt_indent = (opt_indent == TRUE) ? FALSE : TRUE;
}

char *
menutext_autoindent(void)
{
	return (opt_indent == TRUE) ? "Indent off" : "Indent on";
}

/**********************************************
 * Toggle C editting mode
 */

void
toggle_cmode(void)
{
	opt_cmode = (opt_cmode == TRUE) ? FALSE : TRUE;
}

char *
menutext_cmode(void)
{
	return (opt_cmode == TRUE) ? "Cmode off" : "Cmode on";
}

/*****************************************
 * Set the tab width
 */

static void
set_tabwidth_do(int value)
{
	int t;

	if (value < 0 || value > 12 || value == opt_tabsize)
		return;

	opt_tabsize = value;
	if (num_buffers == 0)
		return;


	for (t = 0; t < num_buffers; t++) {
		bf = &buffers[t];
		cursor_change_refresh();
	}
	bf = &buffers[cur_buffer];
}


void
set_tabwidth(void)
{
	char *s;
	int value, err;

	s = dialog_entry("Tab Width (currently %d):", opt_tabsize);
	if (s == NULL || *s == '\0')
		return;

	get_exp(&value, s, &err);
	if (err == 0)
		set_tabwidth_do(value);
	else
		vederror("Illegal value");
}


void
set_wildcard(void)
{
	KEY c;

	c = dialog_msg("Set wildcard (currently '%c'):", wildcard);

	if (c <= ' ' || c >= 0x7f)
		vederror("Wildcard must be $21...$7f");
	wildcard = c;
}

/*****************************************
 * Auto number set and enable
 */

void
toggle_autonum(void)
{
	opt_auto_number = (opt_auto_number == TRUE) ? FALSE : TRUE;
}

char *
menutext_autonum(void)
{
	return (opt_auto_number == TRUE) ? "autoNumber off" : "autoNumber on";
}

void
autonumber_set(void)
{
	char *p;
	int value, inc, err;

	dialog_clear();
	p = dialog_entry("Auto Number Value:");
	if ((p == NULL) || (*p == '\0'))
		return;
	get_exp(&value, p, &err);
	if (err)
		vederror("Expression error");

	p = dialog_entry("  Auto Number Inc:");
	if ((p == NULL) || (*p == '\0'))
		return;
	get_exp(&inc, p, &err);
	if (err)
		vederror("Expression error");
	auto_number_value = value;
	auto_number_inc = inc;
}


/****************************************************
 * RC File set/print
 */


char *
option_set(char *s)
{
	char *s1;
	int err, value;

	s1 = strsep(&s, " \t");

	/* NOTE: The arg (s) is everything after
	 * the option name (minus Spaces). This
	 * permits args like "%X %a"
	 */


	if (s1 == NULL)
		return NULL;

	switch (lookup_value(s1, option_table)) {
		case RC_CR:
			opt_nocr_display = (istrue(s) == TRUE ? FALSE : TRUE);
			return NULL;

		case RC_INDENT:
			opt_indent = istrue(s);
			return NULL;

		case RC_CMODE:
			opt_cmode = istrue(s);
			return NULL;

		case RC_CASE:
			opt_casematch = istrue(s);
			set_findmatch_table();
			return NULL;

		case RC_OVERWRITE:
			opt_insert = (istrue(s) == TRUE ? FALSE : TRUE);
			return NULL;

		case RC_DOBACKUPS:
			opt_dobackups = istrue(s);
			return NULL;

		case RC_BACKUPSUFFIX:
			free(opt_backup_suffix);
			s = translate_delim_string(s);
			if (s == NULL)
				s = "";
			opt_backup_suffix = strdup(s);
			return NULL;

		case RC_BACKUPPREFIX:
			free(opt_backup_prefix);
			s = translate_delim_string(s);
			if (s == NULL)
				s = "";
			opt_backup_prefix = strdup(s);
			return NULL;

		case RC_CLIPBOARD:
			free(clipboard_name);
			s = translate_delim_string(s);
			if (s == NULL)
				s = "";
			clipboard_name = strdup(s);
			return NULL;

		case RC_TIME1:
			free(time1buf);
			time1buf = NULL;
			s = translate_delim_string(s);
			if (s != NULL && *s != '\0')
				time1buf = strdup(s);
			return NULL;

		case RC_TIME2:
			free(time2buf);
			time2buf = NULL;
			s = translate_delim_string(s);
			if (s != NULL && *s != '\0')
				time2buf = strdup(s);
			return NULL;

		case RC_AUTONUM:
			opt_auto_number = istrue(s);
			return NULL;

		case RC_DIALOGPOS:
			get_exp(&value, s, &err);
			if (err == 0) {
				dialog_pos = value;
				return NULL;
			}
			break;


		case RC_TABWIDTH:
			get_exp(&value, s, &err);
			if (err == 0) {
				set_tabwidth_do(value);
				return NULL;
			}
			break;

		case RC_WILDCARD:
			get_exp(&value, s, &err);
			if (err == 0 && value > ' ' && value < 0x7f) {
				wildcard = value;
				return NULL;
			}
			break;

		case RC_CHARSET:
			return set_char_set(s);

		case RC_SCROLL:
			if( set_scroll_indicator(s) == -1)
				break;
			return NULL;
			

		default:
			return "UNKNOWN OPTION";
	}
	return "Illegal argument";
}

void
rc_print_options(FILE * f, char *name)
{
	
#define L(x) lookup_name(x, option_table)

	if (saving_status == FALSE) {
		fprintf(f, "# Option settings\n\n");
	}
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_CR), truefalse(!opt_nocr_display));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_CASE), truefalse(opt_casematch));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_INDENT), truefalse(opt_indent));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_CMODE), truefalse(opt_cmode));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_OVERWRITE), truefalse(!opt_insert));
	fprintf(f, "%s\t%s\t'%c'\n", name, L(RC_WILDCARD), wildcard);
	fprintf(f, "%s\t%s\t%d\n", name, L(RC_TABWIDTH), opt_tabsize);

	if (saving_status == TRUE)
		return;

	fprintf(f, "%s\t%s\t%s\n", name, L(RC_DOBACKUPS), truefalse(opt_dobackups));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_BACKUPSUFFIX),
		make_delim_string(opt_backup_suffix));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_BACKUPPREFIX),
		make_delim_string(opt_backup_prefix));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_CLIPBOARD),
		make_delim_string(clipboard_name));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_TIME1), make_delim_string(time1buf));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_TIME2), make_delim_string(time2buf));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_AUTONUM), truefalse(opt_auto_number));
	fprintf(f, "%s\t%s\t%d\n", name, L(RC_DIALOGPOS), dialog_pos);

	fprintf(f, "%s\t%s\t%s\n", name, L(RC_CHARSET), get_rc_charset());
	
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_SCROLL),  get_scroll_side_name() );
	

	fputs("\n", f);

#undef L

}

/* EOF */
