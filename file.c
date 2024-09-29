/* file.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

/*  File read/write routines  */

#include "ved.h"
#include <limits.h>
#include "attrs.h"


/*****************************************************************
 * Strip out any CR/LF and CRs from the buffer.
 * Used to convert Mac/OS9 and DOS
 * format files to unix.
 */


static void
strip_crs(void)
{
	u_char *p, *f, *e;

	for (f = p = bf->buf, e = f + bf->bsize; f < e;) {
		if (*f == 0x0d) {
			*p++ = 0x0a;
			f++;
			if (*f == 0x0a) {
				f++;
				bf->bsize--;
			}
		} else
			*p++ = *f++;
	}
}


/*********************************************************************
 * Read a file into memory.
 * A NULL filepointer is permitted. This happens from main() when a
 * non-existant file is created.
 */

void
readfile(FILE * fp, int pos)
{
	u_int len, t;

	if (fp != NULL)
		len = get_filesize(fp);
	else
		len = 0;


	/* file sizes may be greater that signed int??? */
	/* this is probably not necessary.... */

	if (len >= INT_MAX - 1) {
		fclose(fp);
		vederror("File size of %u is too large for little old ved", len);
	}
	/* create room in buffer for file */

	if (memory(len + 5)) {
		fclose(fp);
		vederror("Cannot increase buffer size to %d btyes",
			 len + bf->bsize);
	}
	if (len) {
		openbuff(len, pos);

		/* read file into buffer */

		rewind(fp);
		t = fread(&bf->buf[pos], len, sizeof(u_char), fp);
		if (t != 1)
			dialog_msg("WARNING: Read error loading file."
				   " Data may be truncated!");
	} else {

		/* If we've read an empty file into an empty buffer
		 * add a EOL. Ved really needs at least one char in
		 * its buffer to work.
		 */

		if (bf->bsize == 0) {
			openbuff(1, 0);
			bf->buf[0] = EOL;
		}
	}

	if (fp != NULL)
		fclose(fp);

	if (opt_fixcr_input == TRUE)
		strip_crs();

	wrap_buffer(bf->wrapwidth, 0);
}

/*********************************************************************
 * Read new file. This will delete the existing file in memory.
 * If changes have been made to the existing file there is an
 * opportunity to save it first.
 */

void
file_new(void)
{
	char *f;
	int t;
	FILE *fp;
	KEY k;

	dialog_clear();

	if (bf->changed == TRUE) {
		k = dialog_msg("Load new file. Do you want to save %s (Yes/no):",
			bf->filename ? bf->filename : "un-named buffer");

		if (k == 'n') {
			k = dialog_msg("Sure you want to abandon old file (yes/No):");
			if (k != 'y')
				return;
		} else
			save_buffer(NATIVE);
	}
	f = dialog_entry("Load file:");
	if (f == NULL)
		return;

	if (*f == '\0')		/* nothing entered, do visual */
		f = get_dirname(NULL, 0);

	else {			/* entered name or directory */

		f = tilde_fname(f);
		t = isdir(f);
		if (t == -1)
			vederror("Can't access directory %s, error %s",
				 f, strerror(errno));
		if (t == 1)
			f = get_dirname(f, 0);
	}

	/* This 2nd test is needed since dir() might return a null */

	if (f == NULL || *f == '\0')
		return;


	if ((fp = fopen(f, "r")) == NULL) {
		dialog_msg("Can't open %s for reading, error %s!", f, strerror(errno));
		return;
	}
	bf->bsize = 0;
	bf->mtime = get_file_mod_date(fp);
	readfile(fp, 0);
	set_filename(f);
	bf->needbackup = opt_dobackups;
	bf->curpos = 0;
	bf->curline = 0;
	bf->x = 0;
	bf->y = 0;
	bf->block_start = -1;
	bf->block_end = -1;
	display_page(bf->curline);
	bf->changed = FALSE;

	load_status();

	display_filename();
}

void
display_filename(void)
{
	wmove(bf->mw, bf->ysize + 1, 1);
	wclrtoeol(bf->mw);

	if (bf->filename != NULL)
		mvwprintw(bf->mw, bf->ysize + 1, 1, "%s", bf->filename);
}

/*********************************************************************
 * Append a new file at the current cursor position.
 * The old filename is retained.
 */

void
file_append(void)
{
	char *f;
	int t;
	FILE *fp;

	sync_x();
	dialog_clear();

	f = dialog_entry("Append file:");
	if (f == NULL)
		return;

	if (*f == '\0')		/* nothing entered, do visual */
		f = get_dirname(NULL, 0);

	else {			/* entered name or directory */

		f = tilde_fname(f);
		t = isdir(f);
		if (t == -1)
			vederror("Can't access directory %s, error %s",
				 f, strerror(errno));
		if (t == 1)
			f = get_dirname(f, 0);
	}

	/* This 2nd test is needed since dir() might return a null */

	if (f == NULL || *f == '\0')
		return;


	if ((fp = fopen(f, "r")) == NULL) {
		dialog_msg("Can't open %s for appending, error %s!", f, strerror(errno));
		return;
	}
	readfile(fp, bf->curpos);
	cursor_change_refresh();
}

/*********************************************************
 * Save file, no quit. esc-x
 */

void
xsave_buffer(void)
{
	if (opt_noedit)
		return;

	save_buffer(NATIVE);
}

/***********************************************************
 * Save the current file, use the same filename or new one entered
 * 
 */

void
save_buffer(int flag)
{
	char *p, c;
	FILE *fp;
	int t, openmode;
	time_t mtime;
	int copymode = 0;

	if (bf->filename != NULL)
		dialog_addline("Saving file, default = %s", bf->filename);
	else {
		beep();
		dialog_addline("No default filename!");
	}
	p = dialog_entry("New filename:");

	if (p == NULL)
		vederror(NULL);

	if (*p == 0)
		p = bf->filename;

	if (p == NULL)
		vederror(NULL);


	if (p == bf->filename) {
		mtime = get_file_mod_date_name(p);
		if (mtime != bf->mtime) {
			dialog_addline("File has been changed since initial read");
			c = dialog_msg("Do you still want to write (yes/No):");
			if (c != 'y')
				vederror(NULL);
		}
		openmode = OVERWRITE;
	} else
		openmode = PROMPT_OVERWRITE;

	/* Do a backup if the flag is set and the filenames are the same */

	if ((bf->needbackup == TRUE) && (p == bf->filename)) {
		char *nf, *s;

		/* if the prefix is a directory and the file is not a basename
		 * we strip the dirname from the filename. So, if the prefix is
		 * ~/.ved_backups/  and the file is ../woof/foo, the backup
		 * name will be ~./ved_backups/foo (plus suffix).
		 */


		if (index(opt_backup_prefix, '/') != NULL) {
			s = rindex(p, '/');
			s = (s == NULL ? p : s + 1);
		} else
			s = p;

		asprintf(&nf, "%s%s%s", tilde_fname(opt_backup_prefix), s,
			 opt_backup_suffix);
		if (rename(p, nf)) {
			dialog_addline("WARNING: unable to backup file to %s, %s",
				       nf, strerror(errno));
			c = dialog_msg("Do you want to continue save (yes/No):");
			if (c != 'y') {
				free(nf);
				vederror(NULL);
			}
		} else {
			bf->needbackup = FALSE;
			copymode = get_filemode(nf);
			free(nf);
		}
	}
	fp = open_outfile(p, openmode);

	if (fp == NULL)
		vederror("Can't open %s, error %s", p, strerror(errno));

	if (flag == NATIVE) {
		t = fwrite(bf->buf, bf->bsize, sizeof(u_char), fp);
		if (t != 1)
			dialog_msg("WARNING: Write error, %s!", strerror(errno));
	} else {
		for (t = bf->bsize, p = bf->buf; t--;) {
			if ((c = *p++) == '\n') {
				if (flag == MAC) {
					fputc('\r', fp);
				} else if (flag == DOS) {
					fputc('\r', fp);
					fputc('\n', fp);
				} else
					fputc('\n', fp);
			} else
				fputc(c, fp);
		}
	}

	if (bf->filename == p) {
		bf->mtime = get_file_mod_date(fp);
	}
	/* We signal that the buffer if unchanged even if we
	 * did the save to a different filename. I think this
	 * makes sense...if you save to a different file then
	 * you really are on your own.
	 */

	bf->changed = FALSE;

	/* Set the permissions of the new file . This is done
	 * only when the old file was renamed due to creating a
	 * backup copy.
	 */

	if (copymode != 0)
		fchmod(fileno(fp), copymode);

	fclose(fp);

	save_status();


	return;
}

/* EOF */
