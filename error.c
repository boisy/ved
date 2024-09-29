/* error.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

/* error report and handler */


#include "ved.h"
#include <setjmp.h>

static char *revision =
#include "version.h"
;


void
vederror(char *errmsg,...)
{
	va_list ap;
	u_char msg[200];


	if (errmsg) {
		va_start(ap, errmsg);
		vsprintf(msg, errmsg, ap);
		va_end(ap);
	} else
		*msg = '\0';

	if (ved_initialized == FALSE) {
		ved_reset();
		printf(msg);
		puts("");
		puts("VED terminated prior to initialization completion.");
		puts("This should never occur! Advise bvdp.");
		exit(0);
	}
	if (errmsg)
		dialog_msg(msg);

	keyboard_kill_pending();

	curs_set(1);
	dialog_window_delete();	/* may not be necessary */
	wm_refresh();

	longjmp(jumpbuff, 0);
}


void
ved_reset(void)
{
	int x, y;

	noraw();
	echo();
	keypad(stdscr, FALSE);
	curs_set(1);
	getmaxyx(stdscr, y, x);
	move(y - 1, x - 1);
	refresh();
	endwin();
}

void
ved_exit(char *errmsg,...)
{
	va_list ap;

	ved_reset();

	if (errmsg) {
		fprintf(stderr, "Ved terminated\n");
		va_start(ap, errmsg);
		vfprintf(stderr, errmsg, ap);
		va_end(ap);
		fputc('\n', stderr);
	}

	history_savefile();

	exit(0);
}


void
usage(void)
{
	ved_reset();

	printf("ved - text editor, version %s\n"
		"(c) Bob van der Poel Software, 1995-2000\n"
		"Usage: ved [options] [file(s)] [options]\n"
		"  -a   append files on command line\n"
		"  -v   view mode (no changes permitted\n"
		"  -u   convert CR and CR/LF sequences to LF\n"
		"  -l   log rcfile processing to vedrc.log\n"
		"  +nn[:cc]  set line number [column] for next file to nn\n\n",
		revision);

	exit(1);
}

/* EOF */
