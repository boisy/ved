/* dir.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAXNAMEBUF 200		/* this is use by format_entry */

enum {
	RC_EXTEND, RC_INVISIBLE, RC_SCALE, RC_REVERSE, RC_SIZE,
	RC_RM, RC_CP, RC_FIELD, RC_NAME, RC_EXTENSION,
	RC_TYPE, RC_DATE
};

static NAMETABLE dir_rc[] =
{
	{"Extend", RC_EXTEND},
	{"Invisible", RC_INVISIBLE},
	{"Scale", RC_SCALE},
	{"Reverse", RC_REVERSE},
	{"Size", RC_SIZE},
	{"rm", RC_RM},
	{"cp", RC_CP},
	{"Field", RC_FIELD},
	{"Name", RC_NAME},
	{"Extension", RC_EXTENSION},
	{"Type", RC_TYPE},
	{"Date", RC_DATE},
	{0}
};

enum {
	PRINT_HILIGHT,		/* highlight */
	PRINT_NORMAL
};				/* no-hilight */

	/* These are control values for the find_name() function */

enum {
	FIND_FORWARD,
	FIND_BACKWARD,
	FIND_AGAIN_FORWARD,
	FIND_AGAIN_BACKWARD
};

enum {
	SORT_NAME, SORT_EXT, SORT_TYPE, SORT_DATE, SORT_SIZE
};


/* This is a structure which holds the info we need for each directory entry. */

typedef struct {
	char *dir_name;		/* ptr to directory name */
	FLAG tag;		/* set if tagged file */
	mode_t attr;		/* attributes for this entry */
	off_t size;		/* size of this file */
	time_t mtime;		/* last modified date */
} ENTRY;

static ENTRY *ents = NULL;	/* ptr to directory buffer */

static WINDOW *fw;		/* full frame */
static WINDOW *mw;		/* sub window for menu */

static char *curr_dirname = NULL;	/* name of currently processed directory */
static char *filename = NULL;	/* filename to return */

static char *rmfunc = NULL;	/* rm function to use */
static char *cpfunc = NULL;	/* cp function to use */
static int colsize;		/* width of each column */
static int currpage;		/* current page being displayed */
static int dirx;		/* current entry, x (0..ncolumns) */
static int diry;		/* current entry, y (0..page_nrows) */
static int dn_count;		/* number of directories in this dir */
static int entperpage;		/* entries per display page */
static int fn_count;		/* number of files in this dir */
static int fz_total;		/* total file size in this dir */
static int maxlen;		/* length of longest filename */
static int num_marked;		/* number of marked entries */
static int numallocated;	/* total number of entries allocated */
static int numcols;		/* number of columns in display */
static int numentries;		/* total number of entries in this dir */
static int numpages;		/* total number of pages  */
static int page_nrows;		/* number of rows per page */
static int xwidth;		/* width of window 'w' */
static int yheight;		/* depth of window 'w' */
static int scale = 80;		/* size to scale window from main window */

static FLAG flag_extend = 0;	/* 1==show extended entry info */
static FLAG flag_invisible;	/* 1==show hidden files */
static FLAG flag_sfield = 0;	/* sort field (ext, name, date, etc) */
static FLAG flag_sorder = 0;	/* 1==reverse sort order */

/* Function prototypes for static functions */

static ENTRY *get_curr_name(int);

static char *format_entry(int);
static char *get_fullname(char *);

static int dirent_cmp(const void *, const void *);
static int get_array_space(void);
static int get_curr_ent(int, int);
static int read_directory(void);

static void add_curr_dirname(char *);
static void create_dir_window(void);
static void dir_find(void);
static void dir_find_back(void);
static void dir_find_next(void);
static void dir_find_prior(void);
static void dirhl(int, int, int);
static void file_original(void);
static void file_reread(void);
static void file_setdir(void);
static void find_name(int);
static void select_parent(void);
static void set_curr_dirname(char *);
static void set_curr_ent(int);
static void set_page_size(void);
static void showdirpage(void);
static void sort_dir(void);
static void sortopt_date(void);
static void sortopt_ext(void);
static void sortopt_name(void);
static void sortopt_rev(void);
static void sortopt_size(void);
static void sortopt_type(void);
static void toggle_tags(void);
static void mark_extensions(void);
static void deletefile(void);
static void copyfile(void);
static void sort_help(void);
static void cursor_help(void);
static void misc_help(void);

/* Special menu for the directory selector */

static MENUITEM sortitems[] =
{
	{"File name", NULL, 'F', sortopt_name},
	{"Extension", NULL, 'E', sortopt_ext},
	{"file Type", NULL, 'T', sortopt_type},
	{"Date", NULL, 'D', sortopt_date},
	{"file Size", NULL, 'S', sortopt_size},
	{"Reverse", NULL, 'R', sortopt_rev}
};

static MENUITEM fileitems[] =
{
	{"Re-read dir", NULL, 'R', file_reread},
	{"Original dir", NULL, 'O', file_original},
	{"Set dir", NULL, 'S', file_setdir}
};

static MENUITEM searchitems[] =
{
	{"Find", NULL, 'F', dir_find},
	{"find-Next", NULL, 'N', dir_find_next},
	{"find-Back", NULL, 'B', dir_find_back},
	{"find-Prior", NULL, 'P', dir_find_prior}
};

static MENUITEM helpitems[] =
{
	{"Sorting", NULL, 'S', sort_help},
	{"Cursor", NULL, 'C', cursor_help},
	{"Misc", NULL, 'M', misc_help},
	{"About", NULL, 'A', about_ved}
};


static MENUPULL pull_menus[] =
{
	{"Order", sortitems, arraysize(sortitems), 'o'},
	{"File", fileitems, arraysize(fileitems), 'f'},
	{"Search", searchitems, arraysize(searchitems), 's'},
	{"Help", helpitems, arraysize(helpitems), 'h'}
};

static MENU menu =
{
	pull_menus, arraysize(pull_menus), DIR_MENU, DIR_MENU_HIGHLIGHT
};

/*********************************************************
 * Initialize some dir variables. Called from main() at
 * startup.
 */
 
void
dir_init(void)
{
	rmfunc = strdup("rm 2>/dev/null");
	cpfunc = strdup("cp 2>/dev/null");
}

/*****************************************************************/

/**********************************************
 * Help routines for dir
 */

static void
sort_help(void)
{
	werase(mw);

	waddstr(mw, "\n\tThe following keys sort\n"
		"\tthe directory entries\n\n"
		"\t- - Reverse sort order\n"
		"\t0 - sort by name\n"
		"\t1 - sort by extension\n"
		"\t2 - sort by file type\n"
		"\t3 - sort by modification date\n"
		"\t4 - sort by file size\n");

	ved_getkey(mw);
}

static void
cursor_help(void)
{
	werase(mw);

	waddstr(mw, "\n\tThe following keys move\n"
		"\tthe selection bar\n\n"
		"\tUP, DOWN, LEFT, RIGHT - Moves the bar\n"
		"\tESC-UP, ESC-DOWN      - Moves to first/last entry\n"
		"\tPAGE-UP/DOWN          - Moves to pervious/next page\n"
	    "\tHOME/END              - Moves to top/bottom of column\n");

	ved_getkey(mw);
}

static void
misc_help(void)
{
	werase(mw);

	waddstr(mw, "\n\tMiscellaneous commands\n\n"
		"\t$ !    - Shell\n"
		"\tA      - Toggle Tags\n"
		"\tC      - Copy File(s)\n"
		"\tCTRL-C - Exit\n"
		"\tD      - Delete File(s)\n"
		"\tE      - Mark extensions\n"
		"\tEsc-F  - Find\n"
		"\tEsc-I  - Find-previous\n"
		"\tEsc-N  - Find-next\n"
		"\tEsc-S  - Find backwards\n"
		"\tF1-F4  - Enable menus\n"
		"\tG      - Re-read current dir\n"
		"\tI .    - Toggle Invisible display\n"
		"\tR      - Set to current directory \n"
		"\tS      - Enter new directory name\n"
		"\tSPACE  - Mark entry\n"
		"\tReturn - Select Entry\n"
		"\tX      - Toggle Extended display\n");


	ved_getkey(mw);

}

/*********************************************************************
 * Create the ncurses window for the directory
 */


static void
create_dir_window(void)
{
	int x, y;

	if (fw == NULL) {
		xwidth = xsize * scale / 100;
		if (xwidth < 40)
			xwidth = xsize;
		if (xwidth >= xsize - 2)
			xwidth = xsize - 2;

		yheight = ysize * scale / 100;
		if (yheight < 20)
			yheight = ysize;
		if (yheight >= ysize - 2)
			yheight = ysize - 2;

		y = (ysize - yheight) / 2;
		if (y <= 0)
			y = 1;
		x = (xsize - xwidth) / 2;
		if (x <= 0)
			x = 1;

		if ((mw = newwin(yheight + 2, xwidth + 2, y - 1, x - 1)) == NULL)
			ved_exit("Couldn't allocate directory window frame"
				 " Requested %d x %d at %d/%d. Term is %d x %d. Fatal error!",
				 yheight, xwidth, y, x, ysize, xsize);


		wbkgdset(mw, display_attrs[DIR_MENU] | ' ');
		keypad(mw, TRUE);
		meta(mw, TRUE);
		scrollok(mw, FALSE);
		werase(mw);
		menu_create(mw, &menu);		/* create menu bar */

		if ((fw = newwin(yheight, xwidth, y, x)) == NULL)
			ved_exit("Couldn't allocate directory window."
				 " Requested %d x %d at %d/%d. Term is %d x %d. Fatal error!",
				 yheight, xwidth, y, x, ysize, xsize);

		wbkgdset(fw, display_attrs[DIR_NORMAL] | ' ');
		keypad(fw, TRUE);
		meta(fw, TRUE);

		scrollok(fw, FALSE);
	}
	wm_add(mw);
	wm_add(fw);
	wm_refresh();
}

/*********************************************************************
 * Set the size of the columns, etc.
 * This is done by scanning the directory entries and finding the longest
 * one. This entry is then formatted for printing (which adds the directory
 * info, etc. if it is needed) and that size, plus some extras, is taken
 * as the longest line length. We then divide up the terminal screen to
 * accept the most possible.
 */

static void
set_page_size(void)
{
	int l, t, n, max;
	ENTRY *cur_ent;

	page_nrows = yheight;

	/* find longest name, save entry # in n */

	for (t = 0, max = 0, n = 0, cur_ent = ents; t < numentries; t++) {
		if ((l = strlen(cur_ent++->dir_name)) > max) {
			max = l;
			n = t;
		}
	}
	if (max > MAXNAMEBUF)
		vederror("Directory function exit--excessive length of entries");

	/* format longest entry */

	maxlen = strlen(format_entry(n));

	colsize = maxlen + 2;	/* size of each column */
	if (colsize > xwidth - 3) {
		maxlen = xwidth - 3;
		colsize = xwidth - 3;
	}
	numcols = xwidth / colsize;	/* number of columns on screen */
	entperpage = page_nrows * numcols;	/* entries per page */
	numpages = (numentries / entperpage) + 1;	/* total # of pages needed */

	currpage = 0;
	dirx = 0;
	diry = 0;
}

/*********************************************************************
 * Return a pointer to a directory entry structure
 */

static ENTRY *
get_curr_name(int t)
{
	if (t >= 0 && t < numentries)
		return &ents[t];
	else
		return NULL;
}

/*********************************************************************
 * Return the full path name. This just appends the passed string to the
 * current directory name.
 * Note the static buffer...copy this if you want to save it.
 */

static char *
get_fullname(char *name)
{
	static char *fn = NULL;

	free(fn);
	asprintf(&fn, "%s/%s", curr_dirname, name);
	return fn;
}

/*********************************************************************
 * 'curr_dirname' is a static pointer to the current directory being processed
 * These functions will set curr_dirname and append various paths to it. If
 * memory is exhausted a warning is printed, and we try to reassign the value
 * to ".". This should work????
 */

static void
set_curr_dirname(char *p)
{				/* set current directory name */
	free(curr_dirname);
	curr_dirname = strdup(p);

	if (curr_dirname == NULL) {
		dialog_msg("Warning: We have lost the directory path"
			   " due to memory shortage!");
		curr_dirname = strdup(".");
	}
}


/*********************************************************************
 * Return a entry number.
 */

static int
get_curr_ent(int x, int y)
{
	int t;

	t = (currpage * entperpage) + (page_nrows * x) + y;
	return (t >= 0 && t < numentries) ? t : -1;
}


/*********************************************************************
 * Set the current x,y, and page from the entry number
 */

static void
set_curr_ent(int n)
{
	if (n < 0 || n >= numentries)
		n = numentries - 1;
	currpage = n / entperpage;
	n %= entperpage;
	dirx = n / page_nrows;
	diry = n % page_nrows;
}



/*********************************************************************
 * Format a directory entry for printing. This is called by dirhl() and by
 * set_page_size().
 * 
 * n = entry number to format
 * RETURNS: pointer to static char string
 */

static char *
format_entry(int n)
{
	static char buff[MAXNAMEBUF + 50];
	ENTRY *s;
	char attrmark, tagmark, *fname, mdate[30], fsize[10], fattrs[15];

	/* set strings to defaults so we can still print if there is no entry... */

	attrmark = ' ';
	tagmark = ' ';
	fname = "";
	mdate[0] = '\0';
	fsize[0] = '\0';
	fattrs[0] = '\0';

	if ((s = get_curr_name(n))) {
		mode_t attr;

		attr = s->attr;
		if (S_ISDIR(attr))
			attrmark = '/';
		else if (S_ISLNK(attr))
			attrmark = '@';
		else if (S_ISFIFO(attr))
			attrmark = '|';
		else if (S_ISSOCK(attr))
			attrmark = '=';
		else if (S_ISREG(attr) && (attr & (S_IEXEC | S_IEXEC >> 3 | S_IEXEC >> 6)))
			attrmark = '*';

		if (s->tag)
			tagmark = '+';

		if (flag_extend) {
			if (s != ents) {
				char *p;

				mode_string(s->attr, fattrs);
				fattrs[10] = '\0';
				sprintf(fsize, "%d", (u_int) s->size);
				sprintf(mdate, "%s", ctime(&s->mtime));
				if ((p = index(mdate, '\n')) != NULL)
					*p = '\0';
			}
		}
		fname = s->dir_name;
	}
	if (flag_extend)
		sprintf(buff, "%10s %8s %24s ", fattrs, fsize, mdate);
	else
		buff[0] = '\0';
	sprintf(strend(buff), "%c%s%c", tagmark, fname, attrmark);

	return buff;
}

/*********************************************************************
 * Display an entry. This will use reverse video if available,
 * otherwise pointers will be used...
 * 
 * mode values  -  PRINT_HILIGHT  - hilight it
 * PRINT_NORMAL   - no hilight
 * 
 */

static void
dirhl(int mode, int x, int y)
{
	u_char *p;
	int t;
	chtype a, c;

	if (mode == PRINT_HILIGHT)
		a = display_attrs[DIR_HIGHLIGHT];
	else
		a = display_attrs[DIR_NORMAL];

	/* erase entry to all spaces */

	wmove(fw, y, (x * colsize) + 2);
	for (t = maxlen, c = ' ' | a; t--;)
		waddch(fw, c);

	/* Get the name and truncate it by moving the pointer right
	 * if the string is longer than the max permitted length.
	 */

	p = format_entry(get_curr_ent(x, y));
	t = strlen(p);
	if (t > maxlen)
		p += t - maxlen;

	wmove(fw, y, (x * colsize) + 2);
	while (*p) {
		waddch(fw, ((*p >= ' ' && *p <= 0x7f) ? *p : ' ') | a);
		p++;
	}
}

/*********************************************************************
 * Show all entries for a directory page
 */

static void
showdirpage(void)
{
	int x, y, t;
	static char *buf = NULL;
	static int buf_len = 0;
	char buff[100];

	werase(fw);


	t = currpage * entperpage;	/* set t to the first entry to display */

	/* This loop actually prints the entire page of entires */

	for (x = 0; x < numcols; x++) {
		for (y = 0; y < page_nrows && t++ < numentries;
		     dirhl(PRINT_NORMAL, x, y++)) ;
	}

	/* All the rest does the border and the status info...
	 * First allocate a buffer to print the bottom line info
	 */

	if (buf_len < xwidth + 4) {
		buf = realloc(buf, xwidth + 4);
		if (buf == NULL) {
			buf_len = 0;
			return;
		}
	}
	buf_len = xwidth + 4;
	memset(buf, ' ', buf_len - 1);	/* blank fill string. NOTE: not terminated */

	/* print the footer info. We have a buffer the width of the page
	 * so we fill it with the left and right info
	 */

	snprintf(buff, sizeof(buff) - 1, "Page %d/%d", currpage + 1, numpages);
	t = strlen(buff);
	if (t > xwidth / 2) {
		t = xwidth / 2;
		buff[t] = 0;
	}
	strcpy(buf + xwidth - t, buff);		/* page is on right */

	t = xwidth - strlen(buff) - 2;	/* room left for dirname */
	snprintf(buf, t, "%s", curr_dirname);
	buf[strlen(buf)] = ' ';

	mvwaddstr(mw, yheight + 1, 0, buf);
	wnoutrefresh(mw);
}

/**************************************
 * Toggle the state of the file selection
 * tags.
 *
 */

static inline void
toggle_tags(void)
{
	int t;

	for (t = 1; t < numentries; t++)
		num_marked += (ents[t].tag ^= 1) ? 1 : -1;
}

/***************************************************
 * Mark files with a specified extension.
 *
 */


static inline void
mark_extensions(void)
{
	char *p, *e;
	int t;

	dialog_clear();
	p = dialog_entry("Enter file extension to (un)mark:");
	if (p == NULL || *p == 0)
		return;

	for (t = 1; t < numentries; t++) {
		if ((e = rindex(ents[t].dir_name, '.')) &&
		    (strcasecmp(e + 1, p) == 0))
			num_marked += (ents[t].tag ^= 1) ? 1 : -1;
	}
}

/*****************************************************************
 * Copy.
 *
 * Since most of the directory variables are global to this file we can jump
 * here and do the copy in full knowledge of the current pointers.
 *
 * We prompt for the destination and call on 'cp' to do the work for us.
 * When this function returns the current directory will be re-read.
 *
 */

static inline void
copyfile(void)
{
	char *p;
	int t, err;

	dialog_clear();

	/* if we have marked entries, then copy each one... */

	if (num_marked) {

		p = dialog_entry("Copy %d marked files to", num_marked);
		if (p == NULL || *p == 0)
			return;

		for (t = 1; t < numentries; t++) {
			if (ents[t].tag) {
				char *s;
				asprintf(&s, "cp %s %s",
				      get_fullname(ents[t].dir_name), p);
				err = system(s);
				free(s);
				if (err) {
					dialog_addline("Can't copy %s, %s",
						       ents[t].dir_name, strerror(errno));
					if (dialog_msg("Continue (yes/No):") != 'y')
						return;
				}
			}
		}
	} else {		/* just copy the highlighted file */
		char *f;
		t = get_curr_ent(dirx, diry);
		if (t <= 0 || t >= numentries)
			return;
		f = ents[t].dir_name;
		dirhl(PRINT_HILIGHT, dirx, diry);
		p = dialog_entry("Copy to:");
		if (p == NULL || *p == 0)
			return;
		{
			char *s;
			asprintf(&s, "%s %s %s", cpfunc, get_fullname(f), p);
			err = system(s);
			free(s);
			if (err)
				dialog_msg("Couldn't copy %s, %s!", f, strerror(errno));
		}
	}
}


/*****************************************************************
 * Delete
 *
 * Essentially the same as copy. We use rm instead of cp; NOT unlink().
 * Using rm DOES NOT means that the usr could be using a shell wrapper for rm
 * will use the wrapper function UNLESS he has used shopt to force this...
 * (Bash doesn't use alias when doing an non-interactive shell).
 *
 */

static inline void
deletefile(void)
{
	int t, err;
	KEY k;
	FLAG flag;
	u_char buff[100], *f;
	char *p;

	dialog_clear();

	if (opt_noedit) {
		dialog_msg("Deletions are not permitted in 'view only' mode!");
		return;
	}
	if (num_marked == 0) {
		t = get_curr_ent(dirx, diry);
		if (t <= 0 || t > numentries)
			return;
		ents[t].tag = 1;
		dirhl(PRINT_HILIGHT, dirx, diry);
		num_marked++;
		flag = 0;
	} else
		flag = 1;


	for (k = 0, t = 1; t < numentries; t++) {
		if (ents[t].tag) {
			if (k != 'a') {
				if (flag)
					sprintf(buff, "all[%d]marked/", num_marked);
				else
					*buff = '\0';
				k = dialog_msg("Deleting '%s' (yes/No/%squit):",
					       ents[t].dir_name, buff);
				num_marked--;
				ents[t].tag = 0;
				if (flag == 0 && k == 'a')
					k = 0;
				if (k == KEYBOARD_ABORT || k == 'q')
					return;
				if (k != 'y' && k != 'a')
					continue;
			}
			f = get_fullname(ents[t].dir_name);
			asprintf(&p, "%s %s", rmfunc, f);
			err = system(p);
			free(p);
			if (err) {
				dialog_addline("Can't delete %s, %s",
					       f, strerror(errno));
				if (num_marked > 1) {
					if (dialog_msg("Continue (yes/No):") != 'y')
						return;
				} else
					dialog_msg("Press any key to continue:");
			}
		}
	}
}


/*********************************************************************
 * Sort the directory currently in memory
 */



/* Sort function called by qsort when sorting dirs. The only trickery here is
 * that the two args are reversed if flag_sorder flag is set (which reverses
 * the sort order), and that if the sort fields are equal we always drop to
 * a simple name comparison (which makes name the 2nd sort key in computerieze).
 */

static int
dirent_cmp(const void *p1, const void *p2)
{
	const ENTRY *e1, *e2;
	char *x1, *x2;
	int t;

	if (flag_sorder)
		e1 = p2, e2 = p1;
	else
		e1 = p1, e2 = p2;

	switch (flag_sfield) {
		case SORT_EXT:	/* extension */

			if ((x1 = rindex(e1->dir_name, '.')) == NULL)
				x1 = "";
			else
				x1++;
			if ((x2 = rindex(e2->dir_name, '.')) == NULL)
				x2 = "";
			else
				x2++;
			if ((t = strcmp(x1, x2)) == 0)
				break;
			else
				return t;

		case SORT_TYPE:	/* file type */

			if (e1->attr == e2->attr)
				break;
			return e1->attr <= e2->attr;

		case SORT_DATE:	/* mod. time */

			if (e1->mtime == e2->mtime)
				break;
			return e1->mtime <= e2->mtime;

		case SORT_SIZE:	/* file size */

			if (e1->size == e2->size)
				break;
			return e1->size <= e2->size;

		default:
			break;
	}
	return strcmp(e1->dir_name, e2->dir_name);
}

static void
sort_dir(void)
{
	if (numentries > 2)
		qsort(ents + 1, numentries - 1, sizeof(ENTRY), dirent_cmp);
}

/* Sort options, called from menu. */

static void
sortopt_name(void)
{
	flag_sfield = SORT_NAME;
	sort_dir();
}

static void
sortopt_ext(void)
{
	flag_sfield = SORT_EXT;
	sort_dir();
}

static void
sortopt_type(void)
{
	flag_sfield = SORT_TYPE;
	sort_dir();
}

static void
sortopt_date(void)
{
	flag_sfield = SORT_DATE;
	sort_dir();
}

static void
sortopt_size(void)
{
	flag_sfield = SORT_SIZE;
	sort_dir();
}

static void
sortopt_rev(void)
{
	flag_sorder = !flag_sorder;
	sort_dir();
}


/*********************************************************************
 * Read a directory into memory
 */


/* Allocate a larger directory array. This is done 100 entries at a go.
 * returns 0 - success
 * -1 - failure
 */

#define INCREASE 100

static int
get_array_space(void)
{
	ENTRY *a_ent;

	a_ent = (ENTRY *) realloc(ents, (numallocated + INCREASE) * sizeof(ENTRY));
	if (a_ent == NULL) {
		if (numentries == 0)
			vederror("Weird, no core for only %d filenames"
				 "--better buy more bits", INCREASE);
		else {
			beep();
			dialog_msg("Warning: Not enough core to read entire directory!");
			return -1;
		}
	} else
		ents = a_ent;
	numallocated += INCREASE;
	return 0;
}

#undef INCREASE


static int
read_directory(void)
{
	DIR *dirpath;
	struct dirent *d_buff;
	int t;
	struct stat statbuff;
	ENTRY *cur_ent;

	if ((dirpath = opendir(curr_dirname)) == NULL) {
		free(curr_dirname);
		curr_dirname = NULL;
		return -1;
	}
	d_buff = readdir(dirpath);

	for (t = numentries, cur_ent = ents; t--; cur_ent++) {	/* dump old name storage */
		free(cur_ent->dir_name);
		cur_ent->dir_name = NULL;
	}

	if (numallocated == 0)
		get_array_space();	/* calls terminate if no room at all */

	/* Read directory into memory */

	ents[0].dir_name = strdup("<PARENT>");
	ents[0].tag = 0;

	numentries = 1;
	fz_total = 0;
	fn_count = 0;
	dn_count = 0;

	while (1) {
		d_buff = readdir(dirpath);
		if (d_buff == NULL)
			break;
		if (numentries >= numallocated && get_array_space())
			break;

		if (d_buff->d_name[0] == '.') {
			if (flag_invisible == 0)
				continue;
			if (d_buff->d_name[1] == '\0')
				continue;	/* skip . and .. */
			if (d_buff->d_name[1] == '.' && d_buff->d_name[2] == '\0')
				continue;
		}
		cur_ent = ents + numentries;
		cur_ent->dir_name = strdup(d_buff->d_name);
		stat(get_fullname(d_buff->d_name), &statbuff);
		cur_ent->attr = statbuff.st_mode;
		cur_ent->size = statbuff.st_size;
		cur_ent->mtime = statbuff.st_mtime;
		cur_ent->tag = 0;
		if (S_ISDIR(cur_ent->attr))
			dn_count++;
		else {
			fn_count++;
			fz_total += statbuff.st_size;
		}
		numentries++;
	}
	closedir(dirpath);

	sort_dir();
	set_page_size();
	num_marked = 0;
	return 0;
}





/*********************************************************************
 * select the parent directory
 * 
 * We have a number of options here.
 * 
 * foo/bar/..   --> foo/bar/../..   this should never happen!
 * foo/bar      --> foo
 * foo          --> .
 * .            --> ..
 * ..           --> ../..
 * ../../foo    --> ../..
 *
 */

static void
select_parent(void)
{
	char *p;
	ino_t dot_ino;
	dev_t dot_dev;
	struct stat sbuff;
	int t;

	/* Start off by doing a stat on . and .. to see if they match. If they do */
	/* then we are at root and can't go up any further. */

	t = stat(get_fullname("."), &sbuff) == -1;
	if (t == -1)
		return;		/* can't stat '.', don't cd */
	dot_ino = sbuff.st_ino;
	dot_dev = sbuff.st_dev;
	t = stat(get_fullname(".."), &sbuff);
	if (t == -1)
		return;
	if (dot_ino == sbuff.st_ino && dot_dev == sbuff.st_dev)
		return;		/* at root! */

	p = rindex(curr_dirname, '/');

	/* p != NULL if name is "xxx/foo", "/xxx/xxx/foo", "/foo" or "foo/.."
	 * Names with a leading '/' (/foo) are a special case which get
	 * converted to "/"
	 */

	if (p != NULL) {
		if (p == curr_dirname)
			set_curr_dirname("/");

		/* names ending in "/.." get another "/.." appended */

		else if (strcmp(p, "/..") == 0)
			add_curr_dirname("/..");

		/* this leaves normal names...which get truncated */
		/* so "/xxx/foo" becomes "/xxx" */

		else
			*p = '\0';
	} else {		/* no "/" in name */
		/* the current name is ".", convert to ".." */

		if (strcmp(curr_dirname, ".") == 0)
			set_curr_dirname("..");

		/* the current name is "..", convert to "../.." */

		else if (strcmp(curr_dirname, "..") == 0)
			add_curr_dirname("/..");

		/* the current name is "foo". This only occurs when you invoke 
		 * "ved foo" and foo is a dir. In this case we just backup to the 
		 * original parent ".". Changing to "foo/.." would be the same.
		 */

		else
			set_curr_dirname(".");
	}
}

static void
add_curr_dirname(char *p)
{				/* add a path to the current dir name */
	char *temp;
	int newlen = 0;

	if (p)
		newlen += strlen(p);
	if (curr_dirname)
		newlen += strlen(curr_dirname);
	temp = malloc(newlen);
	if (temp) {
		if (curr_dirname)
			strcpy(temp, curr_dirname);
		else
			temp[0] = '\0';
		if (p)
			strcat(temp, p);
		set_curr_dirname(temp);
	}
	free(temp);
}


static void
file_reread(void)
{
	read_directory();
}

static void
file_original(void)
{
	set_curr_dirname(".");
	read_directory();
}

static void
file_setdir(void)
{
	char *p;

	dialog_clear();

	p = dialog_entry("New directory:");
	if (p == NULL || *p == '\0')
		return;

	p = tilde_fname(p);
	if (isdir(p) == 0)
		dialog_msg("Can't access the directory '%s'!", p);
	else {
		set_curr_dirname(p);
		read_directory();
	}
}

/*********************************************************************
 * Find a directory name.
 * 
 * flag FIND_FORWARD            enter a new target
 * FIND_BACKWARD                enter target, search backwards
 * FIND_AGAIN_FORWARD   search forward for existing target
 * FIND_AGAIN_BACKWARD  search back for existing target
 *
 */


static void
find_name(int flag)
{
	int t, direction, oldpage;
	static char *pattern = NULL;
	char *p;
	
	dialog_clear();
	
	
	if(flag==FIND_FORWARD || flag==FIND_BACKWARD) {
		
		if (flag == FIND_FORWARD) 
			p = dialog_entry("FIND:");
		else 
			p = dialog_entry("^FIND:");

		
		if(p==NULL || *p=='\0')
			return;
		free(pattern);
		pattern = strdup(p);
	}
	
	if(pattern == NULL || *pattern=='\0')
		return;
	
	direction = (flag == FIND_BACKWARD || flag == FIND_AGAIN_BACKWARD) ? -1 : 1;

	for (t = get_curr_ent(dirx, diry) + direction; t >= 0 &&
	     t < numentries; t += direction) {
		if (find_pattern(ents[t].dir_name, pattern, strlen(ents[t].dir_name), 1)) {
			oldpage = currpage;
			set_curr_ent(t);
			if (oldpage != currpage)
				showdirpage();
			return;
		}
	}
	dialog_msg("Target '%s' not found!", pattern);
}

static void
dir_find(void)
{
	find_name(FIND_FORWARD);
}

static void
dir_find_next(void)
{
	find_name(FIND_AGAIN_FORWARD);
}

static void
dir_find_back(void)
{
	find_name(FIND_BACKWARD);
}

static void
dir_find_prior(void)
{
	find_name(FIND_AGAIN_BACKWARD);
}

/*********************************************************************
 * Set the directory options. Called from rc.c
 * 
 * valid options:
 * 
 * invisible [=] true/1/yes/false/0/no
 * extend [=] true/
 * reverse [=] true/
 * sort [=] name/extension/type/date/size
 */

char *
directory_set_rc_options(char *s)
{
	char *s1;
	int val, err;

	s1 = strsep(&s, " \t");
	s = strsep(&s, " \t");

	if (s1 == NULL)
		return "NO DIRECTORY COMMAND";;

	switch (lookup_value(s1, dir_rc)) {

		case RC_EXTEND:
			flag_extend = istrue(s);
			return NULL;

		case RC_INVISIBLE:
			flag_invisible = istrue(s);
			return NULL;

		case RC_REVERSE:
			flag_sorder = istrue(s);
			return NULL;

		case RC_SCALE:
		case RC_SIZE:
			get_exp(&val, s, &err);
			if (err == 0 && scale > 20 && scale <= 100) {
				scale = val;
				return NULL;
			} else
				return "ILLEGAL SIZE";

		case RC_RM:
			free(rmfunc);
			s = translate_delim_string(s);
			if (s == NULL)
				s = "";		/* need this for strdup() to dup */
			rmfunc = strdup(s);
			return NULL;

		case RC_CP:
			free(cpfunc);
			s = translate_delim_string(s);
			if (s == NULL)
				s = "";		/* need this for strdup() to dup */
			cpfunc = strdup(s);
			return NULL;

		case RC_FIELD:
			switch (lookup_value(s, dir_rc)) {

				case RC_NAME:
					flag_sfield = SORT_NAME;
					return NULL;

				case RC_EXTENSION:
					flag_sfield = SORT_EXT;
					return NULL;

				case RC_TYPE:
					flag_sfield = SORT_TYPE;
					return NULL;

				case RC_DATE:
					flag_sfield = SORT_DATE;
					return NULL;

				case RC_SIZE:
					flag_sfield = SORT_SIZE;
					return NULL;

				default:
					return "ILLEGAL SORT FIELD";
			}
		default:
			return "UNKNOWN COMMAND";
	}
}

void
rc_print_directory(FILE * f, char *name)
{

#define L(x) lookup_name(x, dir_rc)


	fputs("# Directory settings\n"
	      "#\trm rm-function\n"
	      "#\tcp cp-function\n"
	      "#\textend true/false full information\n"
	      "#\tinvisible true/false displays . entries\n"
	      "#\treverse true/false inverts sort order\n"
	      "#\tsize %of_main_window\n"
	      "#\tsort by extension, type, date, size or name\n\n", f);


	fprintf(f, "%s\t%s\t%s\n", name, L(RC_RM), make_delim_string(rmfunc));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_CP), make_delim_string(cpfunc));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_EXTEND), truefalse(flag_extend));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_INVISIBLE), truefalse(flag_invisible));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_REVERSE), truefalse(flag_sorder));
	fprintf(f, "%s\t%s\t%d\n", name, L(RC_SCALE), scale);
	fprintf(f, "%s\t%s\t", name, L(RC_FIELD));

	switch (flag_sfield) {

		case SORT_EXT:
			fputs(L(RC_EXTENSION), f);
			break;

		case SORT_TYPE:
			fputs(L(RC_TYPE), f);
			break;

		case SORT_DATE:
			fputs(L(RC_DATE), f);
			break;

		case SORT_SIZE:
			fputs(L(RC_SIZE), f);
			break;

		default:
			fputs(L(RC_NAME), f);
			break;
	}
	fputs("\n\n", f);

#undef L

}

/*********************************************************************
 * 
 * On screen file selector
 * 
 * 
 * mode  0=new file, 1=append
 * initial_dir - where to start reading
 */

char *
get_dirname(char *initial_dir, int mode)
{
	int t, entry_num;
	KEY k;
	char *dname;
	int old_curs_set;

	old_curs_set = curs_set(0);

	create_dir_window();

	free(filename);		/* always start off with no filename to return */
	filename = NULL;

	if (initial_dir) {
		set_curr_dirname(initial_dir);
		numentries = 0;
	}
	if (curr_dirname == NULL)
		set_curr_dirname(".");	/* make sure there is something! */

	if (numentries) {
		t = get_curr_ent(dirx, diry);
		set_page_size();
		set_curr_ent(t);
		goto LOOP;

	}
      NEWDIR:
	if (read_directory() == -1)
		goto EXIT;

	/* Main loop which shows dir names one screen at a time */

      LOOP:
	showdirpage();

	while (1) {



		dirhl(PRINT_HILIGHT, dirx, diry);	/* hilight entry */
		wrefresh(fw);
		k = ved_getkey(fw);	/* get a keypress */
		dirhl(PRINT_NORMAL, dirx, diry);	/* un-hilight the current entry */
		wrefresh(fw);

		entry_num = get_curr_ent(dirx, diry);
		if (entry_num >= 0)
			dname = get_curr_name(entry_num)->dir_name;
		else
			dname = NULL;


		/*      Main loop....get a keypress and process it. */

		switch (v_tolower(k)) {

				/*  Mouse/menu ************************************* */

			case MOVE_WINDOW:
				move_window_do(fw, mw);
				showdirpage();
				break;
#ifdef NCURSES_VERSION
			case RESIZE_WINDOW:
				resize_window_do(fw, mw);

				getmaxyx(fw, yheight, xwidth);
				menu_create(fw, &menu);
				set_page_size();
				showdirpage();
				break;

			case MOUSE:

				/* If on main screen and on entry, select */

				if (wenclose(fw, mousey, mousex) == TRUE) {
					int x, y;

					getbegyx(fw, y, x);

					y = mousey - y;
					x = mousex - x;

					/* if mouse is in selector and on an entry
					 * we select that entry
					 */

					if (mvwinch(fw, y, x) != ' ') {
						dirx = x / colsize;
						diry = y;
						retkey('\n');

					}
				}
				/* Test to see if we're on the menu bar */

				else if (wenclose(mw, mousey, mousex) == TRUE) {
					menu_line_do(mw, &menu, MOUSE);
					goto LOOP;
				}
				break;	/* all other mouse events are ignored */
#endif
			case TERMINAL_REFRESH:
				delwin(fw);
				delwin(mw);
				wm_delete(fw);
				wm_delete(mw);

				fw = NULL;
				mw = NULL;

				terminal_refresh();	/* resize the edit windows */

				create_dir_window();
				set_page_size();
				goto LOOP;

			case MENU0_ENABLE:
				menu_line_do(mw, &menu, pull_menus[0].shortcut);
				goto LOOP;

			case MENU1_ENABLE:
				menu_line_do(mw, &menu, pull_menus[1].shortcut);
				goto LOOP;

			case MENU2_ENABLE:
				menu_line_do(mw, &menu, pull_menus[2].shortcut);
				goto LOOP;

			case MENU3_ENABLE:
			case MENU6_ENABLE:
				menu_line_do(mw, &menu, pull_menus[3].shortcut);
				goto LOOP;

				/*  DIR, duplicate in menus ********************* */

			case 'g':
				file_reread();
				goto LOOP;

			case 'r':
				file_original();
				goto LOOP;

			case 's':
				file_setdir();
				goto LOOP;

				/*  ** SORT, duplicated in menus */

			case 'x':
				flag_extend = !flag_extend;
				set_page_size();
				goto LOOP;

			case 'i':
			case '.':
				flag_invisible = !flag_invisible;
				goto NEWDIR;

			case '0':
				sortopt_name();
				goto LOOP;

			case '1':
				sortopt_ext();
				goto LOOP;

			case '2':
				sortopt_type();
				goto LOOP;

			case '3':
				sortopt_date();
				goto LOOP;

			case '4':
				sortopt_size();
				goto LOOP;

			case '-':
				sortopt_rev();
				goto LOOP;

				/*   **** Help/Shell/Misc *** */

			case '$':	/* fork shell */
			case '!':
				doshell();
				goto LOOP;

			case QUIT:
			case KEYBOARD_ABORT:	/* abort, return null */

				goto EXIT;

				/*  FILE MAINTENANCE ******************************** */

			case ' ':
				if (entry_num > 0) {
					num_marked += (ents[entry_num].tag ^= 1) ? 1 : -1;
					retkey(CURSOR_DOWN);
				}
				break;

			case 'a':
				toggle_tags();
				goto LOOP;

			case 'e':
				mark_extensions();
				goto LOOP;

			case 'c':

				copyfile();
				goto NEWDIR;

			case 'd':
				deletefile();
				goto NEWDIR;

				/*  FIND, duplicated in menus  ************************ */

			case FIND:	/* find a pattern in a filename */
				dir_find();
				continue;

			case FIND_NEXT:	/* find a pattern, reverse */
				dir_find_next();
				continue;

			case FIND_BACK:
				dir_find_back();
				continue;

			case FIND_PRIOR:
				dir_find_prior();
				continue;

				/*  CURSOR ******************************************** */

			case CURSOR_UP:	/* up, we decrement y. If y<0 then */
				/* fall through and do a left too */

				if ((--diry) < 0)
					diry += page_nrows;
				else
					continue;

			case CURSOR_LEFT:	/* left.... */

				if ((--dirx) < 0)
					dirx += numcols;
				continue;

			case CURSOR_DOWN:	/* down... if Y > nrows we set to 0 */
				/* and drop though and inc xpos */
				if (++diry >= page_nrows)
					diry = 0;
				else
					continue;

			case CURSOR_RIGHT:	/* right... */

				if (++dirx >= numcols)
					dirx = 0;
				continue;

			case CURSOR_PAGEUP:	/* page up */

				if (numpages > 1) {
					if ((--currpage) < 0)
						currpage = numpages - 1;
					dirx = 0;
					diry = 0;
					goto LOOP;	/* display new screen */
				}
				continue;

			case CURSOR_PAGEDOWN:	/* page down */

				if (numpages > 1) {
					if (++currpage >= numpages)
						currpage = 0;
					dirx = 0;
					diry = 0;
					goto LOOP;
				}
				continue;

			case CURSOR_PAGETOP:	/* top of screen */
				diry = 0;
				continue;

			case CURSOR_PAGEBOTTOM:	/* bottom of screen */
				diry = page_nrows - 1;
				continue;


			case CURSOR_FILESTART:		/* entry #1 */
				t = currpage;
				set_curr_ent(0);
				if (currpage == t)
					continue;
				else
					goto LOOP;

			case CURSOR_FILEEND:	/* last entry */

				t = currpage;
				if (numentries)
					set_curr_ent(numentries - 1);
				if (currpage == t)
					continue;
				else
					goto LOOP;

			case '\n':	/* <CR>, return selection */

				if (entry_num < 0)
					break;
				dirhl(PRINT_HILIGHT, dirx, diry);	/* hilight entry */
				wrefresh(fw);

				if (entry_num == 0) {	/* parent selected */
					select_parent();
					goto NEWDIR;
				}
				filename = strdup(get_fullname(dname));
				if (isdir(filename)) {
					set_curr_dirname(filename);
					free(filename);
					filename = NULL;
					goto NEWDIR;
				}
				goto EXIT;	/* return the filename */




		}
	}

      EXIT:

	wm_delete(fw);
	wm_delete(mw);
	wm_refresh();

	if (old_curs_set != ERR)
		curs_set(old_curs_set);

	if (filename && strncmp(filename, "./", 2) == 0)
		memmove(filename, filename + 2, strlen(filename + 2) + 1);

	return filename;
}



/* EOF */
