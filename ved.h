/* ved.h */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

/* Main include file for the curses based version of ved. */

#ifndef VED_H
#define VED_H 1

#define _GNU_SOURCE
#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#ifndef MAIN
#define GLOBAL extern
#else
#define GLOBAL
#endif

#define SPACE ' '
#define TAB 9
#define EOL 0xa

#define MAXBUFFS 50
#define MIN_X_SIZE 6		/* Min window X size */
#define MIN_Y_SIZE 8		/* Min window Y size */

#define KEY short
#define FLAG unsigned char

/* ask flag values for opening files */

enum {
	OVERWRITE,		/* okay to overwrite, no prompt */
	PROMPT_OVERWRITE,	/* prompt if exist, permit overwrite */
	PROMPT_APPEND		/* prompt if exist, permit overwrite or append */
};

/* types for file saves */

enum {
	NATIVE, UNIX, MAC, DOS
};


/* useful for determining the number of elements in an array */

#define arraysize(a) (sizeof((a))/sizeof((a[0])))

/* Userful container to store string/enum pairs
 * which we use for string to value conversions
 * when reading rc files, etc.
 */

typedef struct {
	char *name;
	chtype value;
} NAMETABLE;


/* Ved uses pulldown menus. These are defined as follows */


typedef struct {
	char *name;		/* text for this item */

	/* If the text is NULL then we use the next function to get the
	 * text. This is handy for having items change their names
	 * depending on context.
	 */

	char *(*printfunc) (void);

	char shortcut;		/* short cut key for this item */
	void (*func) (void);	/* function to call */
} MENUITEM;

typedef struct {
	char *title;		/* name for this pulldown */
	MENUITEM *items;	/* pointer to array of menuitems */
	int numitems;		/* number of items in the menu */
	char shortcut;		/* sortcut to this menu */
} MENUPULL;

typedef struct {
	MENUPULL *pls;		/* pointer to array of pulldowns */
	int numpulls;
	u_char attr_normal;
	u_char attr_highlight;
} MENU;



typedef struct {
	u_char *buf;		/* ptr to text buffer */
	int bsize;		/* actual buffer size in bytes */
	int balloc;		/* bytes allocated in buffer */
	int *linestarts;	/* malloced array of pointers to line starts */
	int linestarts_size;	/* size of array */
	int numlines;		/* line count (real or wrapped) */
	int x;			/* current screen x pos of cursor */
	int y;			/* current screen y pos of cursor */
	int curpos;		/* current position in buffer */
	int curline;		/* current line in buffer */
	int xsize;		/* width of window */
	int ysize;		/* height of window */
	int wrapwidth;		/* line wrap width, 0==no wrap */
	int block_start;
	int block_end;
	int pos_mark;
	int hilite_start;
	int hilite_end;
	int scrollbar_pos;
	FLAG xsync;		/* TRUE if cursor synced with real x position */
	FLAG needbackup;
	time_t mtime;
	int xoffset;		/* horizontal x offset  */
	int xoffset_last;	/* xoffset at time of last screen update */
	WINDOW *win;		/* subwindow for text */
	WINDOW *mw;
	int lastchange;		/* point in buffer where last edit occurred */
	FLAG changed;		/* set if buffer changed from last save */
	FLAG dirtyscreen;	/* set to TRUE after resize.. */
	char *filename;		/* filename associated with buffer */
	char *file_ext;
} BUFFER;

/* now that the various structures/typedefs are done we can read the
 * global prototype file.
 */

#include "cmdtable.h"
//#include "proto.h"
#include "proto.h"

GLOBAL BUFFER buffers[MAXBUFFS];	/* data for each buffer */
GLOBAL BUFFER *bf;		/* active buffer pointer */
GLOBAL int cur_buffer;		/* active buffer number (0..MAXBUFFS-1) */
GLOBAL int num_buffers;		/* number of allocated buffers (1..MAXBUFFS) */
GLOBAL int ved_initialized;	/* set if longjump initiaized */

GLOBAL int ysize;		/* screen size */
GLOBAL int xsize;

GLOBAL u_short opt_tabsize;	/* tab width */
GLOBAL int scroll_jumpsize;	/* number of cells to inc/dec hor scroll */
GLOBAL int opt_casematch;	/* set if case is significant in search/replace */
GLOBAL int mousex;
GLOBAL int mousey;

GLOBAL u_char wildcard;		/* wild card char for searches */
GLOBAL FLAG opt_insert;		/* 1==insert, 0==overstrike */
GLOBAL FLAG opt_indent;		/* 1==auto indent */
GLOBAL int last_menu_xpos;	/* last xpos for a pull down menu */
GLOBAL FLAG opt_noedit;		/* set if no editing permited */
GLOBAL FLAG opt_nocr_display;	/* set to hide display of CRs */
GLOBAL FLAG opt_fixcr_input;	/* TRUE/FALSE, if set CR->LF conversions done */
GLOBAL FLAG opt_dobackups;
GLOBAL FLAG opt_cmode;
GLOBAL char *opt_backup_prefix;
GLOBAL char *opt_backup_suffix;
GLOBAL char *clipboard_name;
GLOBAL char *time1buf;
GLOBAL char *time2buf;
GLOBAL int auto_number_value;	/* used for number macro and auto-number mode */
GLOBAL int auto_number_inc;
GLOBAL FLAG opt_auto_number;
GLOBAL int dialog_pos;
GLOBAL FLAG saving_status;
GLOBAL FLAG loading_status;
GLOBAL jmp_buf jumpbuff;	/* global buffer for longjump environment */
GLOBAL char *status_dir;
GLOBAL int alias_count;


/* Some libs don't have asprintf(), so supply a replacement */

#ifdef NEED_ASPRINTF
#define asprintf ved_asprintf
extern int ved_asprintf (char **, const char *, ...);
#endif

/* Some libs don't have strsep(), so supply a replacement */

#ifdef NEED_STRSEP
#define strsep ved_strsep
extern char *strsep(char **stringp, const char *delim);
#endif


/* Real curses doesn't supply a KEY_MOUSE, so we define a
 * a dummy value. This is just to keep the table in main.h
 * happy.
 */
 
#ifndef NCURSES_VERSION
#define KEY_MOUSE -5
#endif


#endif		/* ifdef VED_H */

void about_ved();
void block_start();
void block_end();
void block_wordwrap();
void block_unformat();
void block_double();
void jump();
void find();
void find_next();
void find_back();

/* EOF */
