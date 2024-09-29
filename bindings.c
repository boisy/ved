/* bindings.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

static NAMETABLE curses_keys[] =
{
	{"KEY_ESC", 0x1b},	/* ESC isn't defined by ncurses */
	{"KEY_BREAK", KEY_BREAK},
	{"KEY_DOWN", KEY_DOWN},
	{"KEY_UP", KEY_UP},
	{"KEY_LEFT", KEY_LEFT},
	{"KEY_RIGHT", KEY_RIGHT},
	{"KEY_HOME", KEY_HOME},
	{"KEY_BACKSPACE", KEY_BACKSPACE},
	{"KEY_F0", KEY_F0},
	{"KEY_F1", KEY_F(1)},
	{"KEY_F2", KEY_F(2)},
	{"KEY_F3", KEY_F(3)},
	{"KEY_F4", KEY_F(4)},
	{"KEY_F5", KEY_F(5)},
	{"KEY_F6", KEY_F(6)},
	{"KEY_F7", KEY_F(7)},
	{"KEY_F8", KEY_F(8)},
	{"KEY_F9", KEY_F(9)},
	{"KEY_F10", KEY_F(10)},
	{"KEY_F11", KEY_F(11)},
	{"KEY_F12", KEY_F(12)},
	{"KEY_F13", KEY_F(13)},
	{"KEY_F14", KEY_F(14)},
	{"KEY_F15", KEY_F(15)},
	{"KEY_F16", KEY_F(16)},
	{"KEY_F17", KEY_F(17)},
	{"KEY_F18", KEY_F(18)},
	{"KEY_F19", KEY_F(19)},
	{"KEY_F20", KEY_F(20)},
	{"KEY_DL", KEY_DL},
	{"KEY_IL", KEY_IL},
	{"KEY_DC", KEY_DC},
	{"KEY_IC", KEY_IC},
	{"KEY_EIC", KEY_EIC},
	{"KEY_CLEAR", KEY_CLEAR},
	{"KEY_EOS", KEY_EOS},
	{"KEY_EOL", KEY_EOL},
	{"KEY_SF", KEY_SF},
	{"KEY_SR", KEY_SR},
	{"KEY_NPAGE", KEY_NPAGE},
	{"KEY_PPAGE", KEY_PPAGE},
	{"KEY_STAB", KEY_STAB},
	{"KEY_CTAB", KEY_CTAB},
	{"KEY_CATAB", KEY_CATAB},
	{"KEY_ENTER", KEY_ENTER},
	{"KEY_SRESET", KEY_SRESET},
	{"KEY_RESET", KEY_RESET},
	{"KEY_PRINT", KEY_PRINT},
	{"KEY_LL", KEY_LL},
	{"KEY_A1", KEY_A1},
	{"KEY_A3", KEY_A3},
	{"KEY_B2", KEY_B2},
	{"KEY_C1", KEY_C1},
	{"KEY_C3", KEY_C3},
	{"KEY_BTAB", KEY_BTAB},
	{"KEY_BEG", KEY_BEG},
	{"KEY_CANCEL", KEY_CANCEL},
	{"KEY_CLOSE", KEY_CLOSE},
	{"KEY_COMMAND", KEY_COMMAND},
	{"KEY_COPY", KEY_COPY},
	{"KEY_CREATE", KEY_CREATE},
	{"KEY_END", KEY_END},
	{"KEY_EXIT", KEY_EXIT},
	{"KEY_FIND", KEY_FIND},
	{"KEY_HELP", KEY_HELP},
	{"KEY_MARK", KEY_MARK},
	{"KEY_MESSAGE", KEY_MESSAGE},
	{"KEY_MOVE", KEY_MOVE},
	{"KEY_NEXT", KEY_NEXT},
	{"KEY_OPEN", KEY_OPEN},
	{"KEY_OPTIONS", KEY_OPTIONS},
	{"KEY_PREVIOUS", KEY_PREVIOUS},
	{"KEY_REDO", KEY_REDO},
	{"KEY_REFERENCE", KEY_REFERENCE},
	{"KEY_REFRESH", KEY_REFRESH},
	{"KEY_REPLACE", KEY_REPLACE},
	{"KEY_RESTART", KEY_RESTART},
	{"KEY_RESUME", KEY_RESUME},
	{"KEY_SAVE", KEY_SAVE},
	{"KEY_SBEG", KEY_SBEG},
	{"KEY_SCANCEL", KEY_SCANCEL},
	{"KEY_SCOMMAND", KEY_SCOMMAND},
	{"KEY_SCOPY", KEY_SCOPY},
	{"KEY_SCREATE", KEY_SCREATE},
	{"KEY_SDC", KEY_SDC},
	{"KEY_SDL", KEY_SDL},
	{"KEY_SELECT", KEY_SELECT},
	{"KEY_SEND", KEY_SEND},
	{"KEY_SEOL", KEY_SEOL},
	{"KEY_SEXIT", KEY_SEXIT},
	{"KEY_SFIND", KEY_SFIND},
	{"KEY_SHELP", KEY_SHELP},
	{"KEY_SHOME", KEY_SHOME},
	{"KEY_SIC", KEY_SIC},
	{"KEY_SLEFT", KEY_SLEFT},
	{"KEY_SMESSAGE", KEY_SMESSAGE},
	{"KEY_SMOVE", KEY_SMOVE},
	{"KEY_SNEXT", KEY_SNEXT},
	{"KEY_SOPTIONS", KEY_SOPTIONS},
	{"KEY_SPREVIOUS", KEY_SPREVIOUS},
	{"KEY_SPRINT", KEY_SPRINT},
	{"KEY_SREDO", KEY_SREDO},
	{"KEY_SREPLACE", KEY_SREPLACE},
	{"KEY_SRIGHT", KEY_SRIGHT},
	{"KEY_SRSUME", KEY_SRSUME},
	{"KEY_SSAVE", KEY_SSAVE},
	{"KEY_SSUSPEND", KEY_SSUSPEND},
	{"KEY_SUNDO", KEY_SUNDO},
	{"KEY_SUSPEND", KEY_SUSPEND},
	{"KEY_UNDO", KEY_UNDO},
	{"KEY_MOUSE", KEY_MOUSE},
	{"KEY_RESIZE", KEY_RESIZE},
	{0, 0}
};


static char *
binding_keyname(int k)
{
	static char buff[30];
	char *p;

	if ((p = lookup_name(k, curses_keys)) != NULL)
		sprintf(buff, "%s", p);
	else if (k <= ' ')
		sprintf(buff, "^%c", k + 0x40);
	else if (k == '\\')
		sprintf(buff, "'\\\\'");
	else if (k > ' ' && k < 0x80)
		sprintf(buff, "\'%c\'", k);
	else
		sprintf(buff, "$%x", k);

	return buff;
}

static char *
bindings_key_string(KEY * keys, u_char kcount)
{
	int t;
	static char s[100];

	s[0] = '\0';
	for (t = 0; t < kcount; t++) {
		sprintf(strend(s), "%s ", binding_keyname(keys[t]));
	}
	return s;

}

void
rc_print_bindings(FILE * fp, char *name)
{
	CMDTABLE *tbl;
	char *p;

	fputs("# Key bindings\n\n", fp);

	for (tbl = cmdtable; tbl->cmdvalue; tbl++) {
		fprintf(fp, "%s %s ", name, tbl->cmdname);

		if (tbl->user_comment != NULL)
			p = make_delim_string(tbl->user_comment);
		else if (tbl->comment != NULL)
			p = make_delim_string(tbl->comment);
		else
			p = make_delim_string("");
		fprintf(fp, "%s ", p);


		if (tbl->user_keyname != NULL)
			p = make_delim_string(tbl->user_keyname);
		else if (tbl->keyname != NULL)
			p = make_delim_string(tbl->keyname);
		else
			p = make_delim_string("");
		fprintf(fp, "%s ", p);
		fprintf(fp, " %s\n", bindings_key_string(tbl->keys, tbl->kcount));
	}
	fprintf(fp, "\n");
}

void
rc_print_alias(FILE * fp, char *name)
{
	int t;
	char *p;
	CMDTABLE *tbl;

	fputs("# Alias Keys\n\n", fp);

	for (t = 0; t < alias_count; t++) {
		for (tbl = cmdtable, p = NULL; tbl->cmdvalue; tbl++) {
			if (tbl->cmdvalue == alias_cmdtable[t].cmdvalue) {
				p = tbl->cmdname;
				break;
			}
		}
		if (p == NULL)
			continue;

		fprintf(fp, "%s %s %s\n", name, p,
			bindings_key_string(alias_cmdtable[t].keys,
					    alias_cmdtable[t].kcount));
	}
	fprintf(fp, "\n");
}

/**************************************
 * Bind a keyseq to a command. All fields must
 * be present!
 *  KEYNAME "comment" "Key label" Keys (up to 5)
 */
 
char *
bindings_set(char *s)
{
	CMDTABLE *tbl;
	char *s1;
	int t, k, err;

	/* Grab the binding name */

	s1 = strsep(&s, " \t");

	/* Lookup name in table, abort if not found */

	for (tbl = cmdtable; tbl->cmdvalue; tbl++) {
		if (strcasecmp(tbl->cmdname, s1) == 0)
			break;
	}

	if (tbl->cmdvalue == 0)
		return "UNKNOWN COMMAND";

	/* Set comment field (used for help messages) */

	s = skip_white(s);
	s1 = translate_delim_string(s);

	if(s1 == NULL)
		return "INCOMPLETE (comment) KEYBINDING";
		
	s = strend(s1) + 1;

	if (strcmp(s1, tbl->comment) != 0) {
		if ((tbl->user_comment == NULL) ||
		    (strcmp(s1, tbl->user_comment) != 0)) {
			free(tbl->user_comment);
			tbl->user_comment = strdup(s1);
		}
	}
	
	/* Set keyname field */

	s = skip_white(s);
	s1 = translate_delim_string(s);
	if(s1 == NULL)
		return "INCOMPLETE (keyname) KEYBINDING";
	
	s = strend(s1) + 1;

	if (strcmp(s1, tbl->keyname) != 0) {
		if ((tbl->user_keyname == NULL) ||
		    (strcmp(s1, tbl->user_keyname) != 0)) {
			free(tbl->user_keyname);
			tbl->user_keyname = strdup(s1);
		}
	}
	
	for (t = 0, tbl->kcount = 0; t < MAXCMDKEY; t++) {
		if (s == NULL || *s == '\0')
			break;
		s = skip_white(s);
		s1 = strsep(&s, " \t");
		if (strlen(s1) <= 0)
			break;
		k = lookup_value(s1, curses_keys);
		if (k > 0)
			tbl->keys[t] = k;
		else {
			get_exp(&k, s1, &err);
			if (err == 0)
				tbl->keys[t] = k;
			else
				break;
		}
		tbl->kcount++;
	}

	if(tbl->kcount == 0)
		return "INCOMPLETE (no keys) KEYBINDING";
	
	return NULL;
}


char *
alias_set(char *s)
{
	CMDTABLE *tbl;
	char *s1;
	int t, k, err, kcount;
	KEY keys[MAXCMDKEY];

	/* Grab the binding name */

	s1 = strsep(&s, " \t");

	/* Lookup name in table, abort if not found */

	for (tbl = cmdtable; tbl->cmdvalue; tbl++) {
		if (strcasecmp(tbl->cmdname, s1) == 0)
			break;
	}
	if (tbl->cmdvalue == 0)
		return "UNKNOWN COMMAND";

	if (alias_count >= MAXALIAS - 1)
		return "TOO MANY ALIASES";

	for (t = 0, kcount = 0; t < MAXCMDKEY; t++) {
		if (s == NULL || *s == '\0')
			break;
		s = skip_white(s);
		s1 = strsep(&s, " \t");
		if (strlen(s1) <= 0)
			break;
		k = lookup_value(s1, curses_keys);
		if (k > 0)
			keys[t] = k;
		else {
			get_exp(&k, s1, &err);
			if (err == 0)
				keys[t] = k;
			else
				break;
		}
		kcount++;
	}

	/* Check to make sure this alias isn't a duplicate */

	for (t = 0; t < alias_count; t++) {
		if ((alias_cmdtable[t].cmdvalue == tbl->cmdvalue) &&
		    (memcmp(alias_cmdtable[t].keys, keys,
			    kcount * sizeof(KEY)) == 0))
			return "DUPLICATE ALIAS";
	}

	/* Finally, add to the table. */

	alias_cmdtable[alias_count].cmdvalue = tbl->cmdvalue;
	alias_cmdtable[alias_count].kcount = kcount;
	for (t = 0; t < kcount; t++)
		alias_cmdtable[alias_count].keys[t] = keys[t];
	alias_count++;

	return NULL;
}


/*********************************************************
 * Set/mangle/change a keybinding
 */


void
set_keybindings(void)
{
	int t, yp, xp, flag;
	MENUITEM names[NUMCMDS];
	MENUPULL pull;
	CMDTABLE *cmd;
	KEY keys[MAXCMDKEY + 1], k;
	int keycount;

	/* create a pulldown menu with the names of commands */

	for (t = 0; t < NUMCMDS; t++) {
		names[t].name = cmdtable[t].cmdname;
		names[t].shortcut = 0;
		names[t].printfunc = NULL;
		names[t].func = NULL;
	}

	pull.title = NULL;
	pull.items = names;
	pull.numitems = NUMCMDS;
	pull.shortcut = 0;

	getbegyx(bf->win, yp, xp);
	t = pulldown_menu(yp, last_menu_xpos, &pull,
			  EDITOR_MENU, EDITOR_MENU_HIGHLIGHT);

	if (t < 0)
		return;

	cmd = &cmdtable[t];

	dialog_clear();

	dialog_addline("%s: %s", cmd->cmdname, cmd->user_comment ?
		       cmd->user_comment : cmd->comment);

	dialog_adddata("BINDING: ");
	for (t = 0; t < cmd->kcount; t++)
		dialog_adddata("%s ", binding_keyname(cmd->keys[t]));
	dialog_addline("");

	dialog_addline("Press new keys, end with a <ENTER>");

	dialog_adddata("NEW: ");
	for (keycount = 0;;) {
		if (keycount >= MAXCMDKEY)
			break;
		k = v_toupper(wgetch(bf->win));
		if (k == '\n' || k == KEY_ENTER)
			break;
		keys[keycount] = k;
		dialog_adddata("%s ", binding_keyname(k));
		keycount++;
	}

	dialog_addline("");

	/* no changes...just exit */

	if (keycount == cmd->kcount && memcmp(&cmd->keys, keys,
					keycount * sizeof(keys[0])) == 0)
		return;

	/* Check to make sure this is valid */

	for (t = 0, flag = 0; t < NUMCMDS; t++) {
		if (keycount == cmdtable[t].kcount &&
		    memcmp(keys, cmdtable[t].keys, keycount *
			   sizeof(cmdtable[0].keys[0])) == 0) {
			flag = 1;
			break;
		} else {
			k = keycount < cmdtable[t].kcount ? keycount : cmdtable[t].kcount;
			if (memcmp(keys, cmdtable[t].keys, k *
				   sizeof(cmdtable[0].keys[0])) == 0) {
				flag = 2;
				break;
			}
		}
	}

	switch (flag) {
		case 0:
			k = dialog_msg("Do you wish to change the binding (y/N):");
			break;

		case 1:
			k = dialog_msg("Duplicate key sequences found, "
			       "do you want to save this anyway (y/N):");
			break;

		case 2:
			k = dialog_msg("Duplicate leading key sequence, "
				"do you want to save this anyway (y/N:");
			break;
	}

	if (k == 'y') {
		cmd->kcount = keycount;
		for (t = 0; t < keycount; t++)
			cmd->keys[t] = keys[t];
	}
}

/* EOF */
