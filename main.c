/* ved.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

/* initialization and main loop */

#define MAIN

#include "ved.h"
#include "main.h"
#include "attrs.h"
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	KEY c;
	int t, n, lineno, colno, offset;
	FILE *fp;
	char *initdir, *p;
	FLAG append_files = FALSE;
	int err;
	int log_rc = 0;

	/* set curses screen mode */

	ved_initialized = FALSE;

	initscr();

	start_color();
	for (t = 1; t < COLOR_PAIRS; t++)
		init_pair(t, t % COLORS, t / COLORS);

	init_colors();
	set_char_set("Latin1");

	raw();
	keypad(stdscr, TRUE);
	scrollok(stdscr, TRUE);
	noecho();
	meta(stdscr, TRUE);

	ysize = LINES;
	xsize = COLS;

	if (ysize < MIN_Y_SIZE || xsize < MIN_X_SIZE)
		ved_exit("Terminal size too small, min %dx%d", MIN_Y_SIZE, MIN_X_SIZE);

	/* Set the default settings. Note that that string options
	 * must be set with strdup() so that later changes can
	 * be done using free(), strdup().
	 */

	opt_insert = TRUE;
	opt_nocr_display = TRUE;
	opt_tabsize = 8;
	opt_casematch = FALSE;
	set_findmatch_table();
	opt_indent = TRUE;
	opt_cmode = TRUE;
	opt_fixcr_input = FALSE;
	opt_backup_prefix = strdup("");
	opt_backup_suffix = strdup("~");
	opt_dobackups = TRUE;
	time1buf = strdup("%A %B %d, %Y");
	time2buf = strdup("%X");
	clipboard_name = strdup("/tmp/vedclip");
	wildcard = '?';
	opt_noedit = FALSE;
	scroll_jumpsize = 4;
	auto_number_value = 1;
	auto_number_inc = 1;
	set_scroll_indicator("Both");	/* enable scroll indicator left/right */	
	opt_auto_number = FALSE;
	dialog_pos = 5;
	set_history_filename(".ved_history");
	saving_status = FALSE;
	loading_status = FALSE;
	status_dir = NULL;
	alias_count = 1;
	dir_init();		/* set initial dir values */
	
	rcfile(log_rc);

	bkgdset(display_attrs[THE_BACKGROUND]);
	clear();
	refresh();

#ifdef NCURSES_VERSION
	//mousemask(BUTTON1_CLICKED, NULL);
	mousemask(BUTTON1_PRESSED, NULL);
#endif

	death_init();

#ifdef SIGWINCH
	signal(SIGWINCH, sigwinch_handler);
#endif

	/* parse command line options */

	while ((t = getopt(argc, argv, "avul?")) != -1) {
		switch (v_tolower(t)) {

			case 'a':
				append_files = TRUE;
				break;

			case 'v':
				opt_noedit = TRUE;
				break;

			case 'u':
				opt_fixcr_input = TRUE;
				break;

			case 'l':
				log_rc = 1;
				break;

			default:
				usage();
		}
	}



	/* Read in the files specified on the command line. If
	 * no filename at all is given we enter the fileselector.
	 * Returning from the fileselector with no file just terminates
	 * ved. If a filename is returned from the fileselector we
	 * mung the argv/c array. This eliminates the value of argv[0]
	 * which is normally set as the program name. However, this is
	 * only arg we are assured of a slot. Could be a problem???
	 *
	 * Each filename is loaded in turn. If only one filename is given
	 * and it doesn't exist the user is prompted. With multiples,
	 * all filenames must exist.
	 */

	/* Count # of filenames. Gotta do it the hard way since
	 * +nnn counts as an arg, but not file...
	 */

	for (t = optind, n = 0; t < argc; t++)
		if (argv[t][0] != '+')
			n++;

	if (n > MAXBUFFS)
		ved_exit("Too many filenames, max %d permitted", MAXBUFFS);

	if ((n == 1) && isdir(tilde_fname(argv[optind])) == TRUE) {
		initdir = strdup(argv[optind++]);
		n = 0;
	} else
		initdir = ".";

	if (n == 0) {
		optind = 0;
		argc = 1;
		if ((argv[optind] = get_dirname(initdir, 0)) == NULL)
			ved_exit("No filename specified");
	}
	for (t = optind, lineno = 1; t < argc; t++) {

		if (argv[t][0] == '+') {
			colno=0;
			p=get_exp(&lineno, &argv[t][1], &err);
			if (err)
				lineno = 1;
			if(*p==':') {
				p++;
				get_exp(&colno, p, &err);
				if(err)
					colno=0;
			}
			continue;
		}
		if (isdir(tilde_fname(argv[t])) == TRUE)
			ved_exit("You cannot specify both directory names "
				 "and filenames on the command line");

		fp = fopen(argv[t], "r");
		if (fp == NULL) {
			if (optind == argc - 1) {	/* one file on cmdline */
				c = dialog_msg("File %s does not exist. "
				    "Okay to create (yes/No):", argv[t]);
				if (c == 'n')
					ved_exit(NULL);
			} else {
				ved_exit("No file '%s', All filenames must exist when "
				      "loading multiple files", argv[t]);
			}
		}
		if (append_files == FALSE || t == optind) {
			if (make_buffer())
				ved_exit("Can't create window");

			bf = &buffers[num_buffers - 1];

			/* read into new buffer */

			if (fp != NULL) {
				bf->mtime = get_file_mod_date(fp);
				readfile(fp, 0);
				bf->needbackup = opt_dobackups;
			}
			/* new empty file */

			else {
				bf->mtime = (time_t) 0;
				bf->needbackup = FALSE;
				openbuff(1, 0);
				bf->buf[0] = EOL;
				wrap_buffer(bf->wrapwidth, 0);
			}

			set_filename(argv[t]);
			bf->lastchange = -1;
			bf->changed = FALSE;
			display_filename();
			display_page(0);
			if (lineno > 1){
				if (lineno > bf->numlines - 1)
					lineno = 1;
				offset = bf->linestarts[lineno-1] + colno;
				if(offset >= bf->linestarts[lineno])
					offset = bf->linestarts[lineno]-1;
				cursor_moveto(offset);
			}
			lineno = 1;
		}
		/* append */

		else
			readfile(fp, bf->bsize);
	}

	/* All files are loaded
	 * Use buffer 0
	 */

	cur_buffer = 0;
	bf = &buffers[cur_buffer];

	rcfile(log_rc);
	load_status();		/* read status file for 1st file only */

	wm_add(bf->mw);
	wm_add(bf->win);

	if (append_files == TRUE)
		redisplay();

	wm_refresh();

	history_loadfile();	/* read old command line history */


	/* Finally we can set up our jump. This is used by
	 * the vederror(). If we've not set ved_initialized
	 * any error will abort ved...this would indicated
	 * a serious error!
	 */

	setjmp(jumpbuff);

	ved_initialized = TRUE;

	/* This is the main loop. We just get keys and
	 * process them.
	 */

	while (1) {
		c = curkey();

		if (c < FIRSTVEDCOMMAND) {
			if (opt_noedit == FALSE) {
				edit(c);
			}
		} else
			dojump(c);
	}

	/* UNREACHABLE */
}

/* EOF */
