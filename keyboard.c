/* keyboard.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

KEY ved_getkey(WINDOW * w);

#define MAXRETKEYS 10

static KEY retkbuf[MAXRETKEYS];	/* buffer for returned keys, there should never */
static int retkcnt = 0;		/* be more than 3 or 4 keys in buffer */

/* This is an array of pointers for the macros. If the macro
 * is NULL, no macro defined. Slots avail for A-Z, a-z. 0-9 are
 * system defined.
 */

#define MAXMACS	20
#define MAXLEARN 200

static KEY *macros[39];		/* a..z (26) + 0..9 (10) */
static FLAG macro_keydefined[39];	/* set TRUE if defined from keyboard */
static KEY *macstack[MAXMACS];
static int macrolevel;
static KEY repeatky;
static KEY repeatky2;
static int repeatcount = 0;
static int repeatcntdef = 0;
static int macro_learning = 0;
static int macro_learn_num;
static KEY macro_learn_buffer[MAXLEARN];
static KEY macro_learning_name;

static void macro_add_learned(void);

static void display_macro_learning(void);

/*********************************************************************
 * Multiple key unget() 
 */

void
retkey(KEY k)
{
	if (retkcnt < MAXRETKEYS - 1)
		retkbuf[retkcnt++] = k;
}

KEY
get_pending_key(void)
{
	return (retkcnt) ? retkbuf[retkcnt - 1] : 0;
}


void
keyboard_kill_pending(void)
{
	retkcnt = 0;
	macrolevel = 0;
	repeatcount = 0;
	macro_learning = 0;
}

/*********************************************************************
 * Get keypress with cursor positioning. This can only be
 * used from edit screens! In addition to returning a keypress
 * this also updates the scroll bar and the line-number status
 */

KEY
curkey(void)
{
	int lnum;

	lnum = bf->curline;

	/* Use the line number for the top line if we're in no-edit mode */

	if (opt_noedit == TRUE)
		lnum -= bf->y;

	wmove(bf->mw, bf->ysize + 1, bf->xsize - 15);
	wclrtoeol(bf->mw);
	wprintw(bf->mw, "%c %c %c %c %d",
		(opt_insert == TRUE ? 'I' : 'O'),
		(opt_indent == TRUE ? '>' : ' '),
		(opt_cmode == TRUE ? 'C' : ' '),
		(opt_casematch == TRUE ? 'M' : 'm'),
		(lnum + 1));


	update_scroll_pointer(lnum);		/* go update the marker */	
	

	wnoutrefresh(bf->mw);

	/* Update the horizontal scroll stuff */

	set_xoffset();

	/* Finally, position the cursor */

	wmove(bf->win, bf->y, bf->x - bf->xoffset);
	
	wnoutrefresh(bf->win);
	doupdate();


	/* Go get a key */

	return ved_getkey(bf->win);
}

/*********************************************************************
 * Get a keysequence
 * 
 * Scan command table, look for partial/total match
 * RETURNS: 0 - partial match
 * 1..MAXVEDNAME keypress/command value
 * -1  no match at all
 */

static inline KEY
cmp_key_cmdtable(int count, KEY * kbff)
{
	int n, t;
	KEY *kyseq, *buff;
	CMDTABLE *tbl;
	ALIAS_CMDTABLE *a_tbl;
	u_char partial;

	for (tbl = cmdtable, n = 0, partial = 0; n < NUMCMDS; n++, tbl++) {
		if (tbl->kcount == 0)
			continue;	/* skip unbound commands */

		for (t = 0, buff = kbff, kyseq = tbl->keys;;) {
			if (t >= count)
				break;
			if (v_tolower(*buff++) != v_tolower(*kyseq++))
				break;
			t++;
			if (t == count)
				partial = 1;
		}

		/* command key match */

		if (t == count && t == tbl->kcount)
			return tbl->cmdvalue;
	}

	if (partial)
		return 0;

	/* Check the alias table. This table is just like the above,
	 * but it only contains bindings/cmdnames.
	 */

	for (a_tbl = alias_cmdtable, n = 0, partial = 0; n < alias_count; n++, a_tbl++) {
		if (a_tbl->kcount == 0)
			continue;	/* skip unbound commands??? */

		for (t = 0, buff = kbff, kyseq = a_tbl->keys;;) {
			if (t >= count)
				break;
			if (v_tolower(*buff++) != v_tolower(*kyseq++))
				break;
			t++;
			if (t == count)
				partial = 1;
		}

		/* alias  key match */

		if (t == count && t == a_tbl->kcount)
			return a_tbl->cmdvalue;
	}


	/* We've checked the main and alias tables and found no match at all .
	 * Could it be that a single ascii key is intended ?
	 */

	if (count == 1 && (kbff[0] <= 0xff) && (kbff[0] >= SPACE || kbff[0] == '\n'))
		return kbff[0];

	/* nope, must be a bad sequence then */

	return -1;
}


KEY
ved_getkey(WINDOW * w)
{

	int count;
	KEY kbff[MAXCMDKEY + 1], n;

	if (retkcnt) {
		n = retkbuf[--retkcnt];
		return n;
	}
	if (repeatcount) {
		repeatcount--;
		n = repeatky;
		if (n == MACRO_START)
			retkey(repeatky2);
		return n;
	}
	if (macrolevel > 0) {
		KEY *mptr;

		mptr = macstack[macrolevel - 1];	/* point to macro */
		n = *mptr++;	/* get char, upate ptr */
		macstack[macrolevel - 1] = mptr;	/* remember ptr for next */
		if (n == 0 || *mptr == 0) {
			macrolevel--;	/* end current macro */
			if (n == 0)
				n = ved_getkey(w);
		}
		return n;
	}
	for (count = 0;;) {

		if (count >= MAXCMDKEY) {
			beep();
			doupdate();
			count = 0;
		}
		kbff[count++] = wgetch(w);

		/* A few inputs are given special handling.... */

		/* a KEY_ENTER (number pad enter??) is converted to 
		 * a regular < enter >
		 */

		if (kbff[count - 1] == KEY_ENTER)
			kbff[count = 1] = '\n';

		/* A mouse click sets the mouse x/y coods 
		 * and then returns the MOUSE key to the caller to 
		 * process.
		 */

#ifdef NCURSES_VERSION
 		if (kbff[count - 1] == KEY_MOUSE) {
			MEVENT mp;
			if (getmouse(&mp) == ERR)
				continue;
			mousex = mp.x;
			mousey = mp.y;
			n = MOUSE;
			break;
		}
#endif
		/* convert the key (sequence) to a command value,
		 * just a key, or loop back to complete a sequence
		 */

		n = cmp_key_cmdtable(count, kbff);

		if (n == 0)	/* partial match, get more seq. */
			continue;

		if (n == -1) {	/* bad sequence, start over */
			count = MAXCMDKEY + 2;
			continue;
		}
		break;		/* complete match, return value */
	}


	if (macro_learning > 0) {
		if (n == KEYBOARD_ABORT)
			macro_add_learned();
		else {
			macro_learn_buffer[macro_learning - 1] = n;
			if (macro_learning >= MAXLEARN - 2) {
				macro_add_learned();
				dialog_msg("Macro definition '%c' prematurely terminated. "
					   "Too many learned keys!", macro_learning_name);
			} else {
				macro_learning++;
				display_macro_learning();
			}
		}
	}
	return n;
}


/*********************************************************************
 * Get a hex value from the keyboard. This is called if the user enters
 * HEXINPUT from the keyboard. We wait for 2 keys to be pressed...if either
 * is not a valid hex digit the input is ignored.
 * The resulting value is returned to the caller.
 * This way various functions can handle hexinput differently.
 */

int
get_hex_input(WINDOW * w)
{
	int t;
	int k;
	int value[2];

	for (t = 0; t < 2; t++) {
		k = v_toupper(ved_getkey(w));
		if (k >= '0' && k <= '9')
			value[t] = k - '0';
		else if (k >= 'A' && k <= 'F')
			value[t] = (k - 'A') + 10;
		else
			return -1;
	}
	return value[0] * 16 + value[1];
}

/***********************************************************
 * Set up a repeat key. Get the count and the key. This key
 * will be repeated N times. It can be a command or a text char.
 */

void
repeat_key(void)
{
	u_char *p;
	int err;

	repeatcount = 0;	/* end existing repeat if set... */

	dialog_clear();
	p = dialog_entry("Repeat count (%d):", repeatcntdef);

	if (p == NULL)
		vederror(NULL);

	if (*p) {
		get_exp(&repeatcntdef, p, &err);
		if (err)
			vederror("Expression error");
	}
	if (repeatcntdef <= 0)
		vederror(NULL);

	dialog_adddata("Which command or key:");

	repeatky=ved_getkey(bf->win);
	if (repeatky == MACRO_START) {
		dialog_addline("");
		repeatky2 = dialog_msg("Which macro:");
	}
	repeatcount = repeatcntdef;
}

static void
display_macro_learning(void)
{
	wmove(bf->mw, bf->ysize + 1, 1);
	wprintw(bf->mw, "LEARNING MACRO %c, %d", macro_learning_name, macro_learning);
	wnoutrefresh(bf->mw);
}

/*********************************************
 * Convert a ascii key to a macro slot.
 * Valid inputs are a..z, A..Z, 0..9.
 * Returns macro number 0..36. -1 if error.
 */

static int
get_macro_value(char c)
{
	int m = -1;

	if ((c >= '0') && (c <= '9'))
		m = (c - '0') + 26;

	else {
		c = v_tolower(c);
		if ((c >= 'a') && (c <= 'z'))
			m = c - 'a';
	}
	return m;
}

/*****************************************
 * Define a macro from an rc line(s)
 * Syntax: Macro X foo blah
 *     'X' is the macro to define
 *     'foo blah' is the macro text. You can
 *        include a command name with `NAME'
 *        or a non-printable with `nnn'.
 *        A single ` or ' must be defined with
 *           its numeric value.
 */

static inline KEY
macro_value_lookup(char **s)
{
	u_char buff[100];
	int t;
	char *p;
	int value, err;

	for (t = 0, p = *s;; t++) {
		if (*p == '\'') {
			p++;
			break;
		}
		if (*p == '\0')
			break;

		else
			buff[t] = *p++;

		if (t > sizeof(buff) - 2)
			break;
	}
	buff[t] = '\0';
	*s = p;

	get_exp(&value, buff, &err);
	return err ? 0 : value;
}

static inline KEY
macro_name_lookup(char **s)
{
	u_char buff[100];
	unsigned  t;
	char *p;
	CMDTABLE *tbl;

	for (t = 0, p = *s;; t++) {
		if (*p == '\'') {
			p++;
			break;
		}
		if (*p == '\0')
			break;
		buff[t] = *p++;

		if (t >= sizeof(buff) - 2)
			break;
	}
	buff[t] = '\0';

	*s = p;

	for (tbl = cmdtable; tbl->cmdname; tbl++)
		if (strcasecmp(buff, tbl->cmdname) == 0)
			return tbl->cmdvalue;

	return 0;
}


char *
macro_define(char *s)
{
	KEY buf[500], *p;
	int m;
	unsigned  t;


	if ((m = get_macro_value(*s++)) == -1)
		return "ILLEGAL MACRO KEY NAME";

	s = skip_white(s);

	for (t = 0;;) {
		if (*s == '`') {
			s++;
			if (v_isdigit(*s) == TRUE) {
				if ((buf[t++] = macro_value_lookup(&s)) == 0)
					return "ILLEGAL NUMERIC VALUE IN DEFINITION";
			} else {
				if ((buf[t++] = macro_name_lookup(&s)) == 0)
					return "ILLEGAL/UNKNOWN COMMAND NAME IN DEFINITION";
			}
		} else
			buf[t++] = (KEY) * s++;

		if (t >= sizeof(buf) - 2)
			break;
		if (*(s - 1) == 0)
			break;
	}

	free(macros[m]);
	p = malloc(t * sizeof(KEY));
	if (p)
		memcpy(p, buf, t * sizeof(KEY));

	macros[m] = p;

	macro_keydefined[m] = loading_status ? TRUE : FALSE;

	return NULL;

}

static inline void
macro_save_command(FILE * fp, KEY k)
{
	CMDTABLE *tbl;

	for (tbl = cmdtable; tbl->cmdname != NULL; tbl++) {
		if (k == tbl->cmdvalue) {
			fprintf(fp, "`%s'", tbl->cmdname);
			break;
		}
	}
}

void
rc_print_macros(FILE * fp, char *name)
{
	unsigned  t;
	KEY *s;
	char c;

	if (saving_status == FALSE)
		fprintf(fp, "# Macro definitions\n\n");

	for (t = 0; t < arraysize(macros); t++) {
		s = macros[t];
		if (s == NULL)
			continue;

		/* Don't save RC file defined macros in status file */

		if ((saving_status == TRUE) && (macro_keydefined[t] == FALSE))
			continue;

		if (t <= 25)
			c = t + 'A';
		else
			c = t - 26 + '0';

		fprintf(fp, "%s %c ", name, c);
		while (*s) {
			if (*s > 0xff)
				macro_save_command(fp, *s++);

			else if ((*s == '`') || (*s == '\''))
				fprintf(fp, "`%d'", *s++);

			else if ((*s < ' ') || (*s >= 0x7f))
				fprintf(fp, "`%d'", *s++);
			else
				fprintf(fp, "%c", *s++);
		}
		putc('\n', fp);
	}
	if (saving_status == FALSE) {
		putc('\n', fp);
		putc('\n', fp);
	}
}

/* Learn a macro from the keyboard */

void
macro_learn(void)
{
	KEY k;
	int t;

	dialog_clear();
	k = dialog_msg("To define a macro press the name of the macro (a-z 0..9). "
	      "All following keys up to a CTRL-C will be saved for that "
		       "macro definition:");

	k = v_toupper(k);
	if ((t = get_macro_value(k)) == -1)
		return;

	macro_learn_num = t;
	macro_learning = 1;
	macro_learning_name = k;
	wmove(bf->mw, bf->ysize + 1, 1);
	wclrtoeol(bf->mw);
	display_macro_learning();
}

static void
macro_add_learned(void)
{
	KEY *p;

	free(macros[macro_learn_num]);

	p = calloc((macro_learning + 1), sizeof(KEY));
	if (p != NULL) {
		memcpy(p, macro_learn_buffer, (macro_learning-1) * sizeof(KEY));
	}
	
	macros[macro_learn_num] = p;
	macro_keydefined[macro_learn_num] = TRUE;
	macro_learning = 0;
	
	/* Clear status line and redisplay filename */
	
	wmove(bf->mw, bf->ysize + 1, 1);
	wclrtoeol(bf->mw);
	display_filename();
}

/* start a macro */

static KEY *
keycopy(KEY * keybuf, char *cbuff)
{
	KEY *s = keybuf;

	if (cbuff == NULL)
		return NULL;

	while (1)
		if ((*s++ = (KEY) * cbuff++) == 0)
			break;

	return keybuf;
}

static void
do_macro_start(KEY * buff)
{
	if (buff != NULL) {
		if (macrolevel == MAXMACS)
			vederror("Macro stack overflow");
		macstack[macrolevel++] = buff;
	}
}

void
macro_start(void)
{
	KEY k;
	int t;

	k = ved_getkey(bf->win);

	if ((t = get_macro_value(k)) == -1)
		return;

	do_macro_start(macros[t]);
}



void
macro_filename(void)
{
	static KEY *buff = NULL;
	int len;

	free(buff);

	if (bf->filename == NULL)
		return;
	len = strlen(bf->filename);
	if (len <= 0)
		return;

	if ((buff = malloc((len + 2) * sizeof(KEY))) != NULL) {
		keycopy(buff, bf->filename);
		do_macro_start(buff);
	}
}

void
macro_file_ext(void)
{
	static KEY *buff = NULL;
	int len;

	free(buff);

	if (bf->file_ext == NULL)
		return;
	len = strlen(bf->filename);
	if (len <= 0)
		return;

	if ((buff = malloc((len + 2) * sizeof(KEY))) != NULL) {
		keycopy(buff, bf->file_ext);
		do_macro_start(buff);
	}
}

void
macro_file_base(void)
{
	static KEY *buff = NULL;
	int len, t;

	free(buff);

	if (bf->filename == NULL)
		return;
	len = strlen(bf->filename);
	if (len <= 0)
		return;

	if ((buff = malloc((len + 2) * sizeof(KEY))) != NULL) {
		keycopy(buff, bf->filename);
		for (t = len - 1; t; t--) {
			if (buff[t] == (KEY) '.') {
				buff[t] = 0;
				break;
			}
		}
		do_macro_start(buff);
	}
}
void
macro_number(void)
{
	char buff[20];
	static KEY kbuff[20];

	sprintf(buff, "%d", auto_number_value);
	auto_number_value += auto_number_inc;
	keycopy(kbuff, buff);
	do_macro_start(kbuff);
}

void
macro_time1(void)
{
	char buff[100];
	static KEY kbuff[100];
	struct tm *timeptr;
	clock_t clock_time;

	if (time1buf == NULL || *time1buf == '\0')
		return;

	time(&clock_time);
	timeptr = localtime(&clock_time);
	strftime(buff, sizeof(buff) - 2, time1buf, timeptr);
	keycopy(kbuff, buff);
	do_macro_start(kbuff);
}

void
macro_time2(void)
{
	char buff[100];
	static KEY kbuff[100];
	struct tm *timeptr;
	clock_t clock_time;

	if (time2buf == NULL || *time2buf == '\0')
		return;

	time(&clock_time);
	timeptr = localtime(&clock_time);
	strftime(buff, sizeof(buff) - 2, time2buf, timeptr);
	keycopy(kbuff, buff);
	do_macro_start(kbuff);
}

/* EOF */
