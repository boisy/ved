/* clipboard.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

static FLAG first_paste = TRUE;
static char *clipname = NULL;

/*************************************************************
 * Paste buffer routines. These permit a block to be copied to a clipboard
 * in a regular file in /tmp or whatever the user sets clipboard_name to.
 * This clipboard can be accessed from any ved process
 * by the same user, so it is easy to cut and paste between various
 * incarnations of ved.
 *
 * NOTE: No file locking is done. I don't think it is necessary as
 * only the user can write/read from his clipboard file and I don't
 * see how he could do so from 2 processes at that the same time.
 */

static FILE *
openclip(char *mode)
{
	FILE *fp;

	int old_umask;
	dialog_clear();

	if (!clipname) {
		if (clipboard_name == NULL)
			vederror("Clipboard name isn't defined, why'd you do that?");

		asprintf(&clipname, "%s.%X", clipboard_name, getuid());

		if (clipname == NULL)
			vederror("No memory for clipboard functions");
	}
	/* Setting umask results in clipboard file
	 * with -rw-------  permissions.
	 */

	old_umask = umask(077);
	fp = fopen(tilde_fname(clipname), mode);
	umask(old_umask);

	if (fp == NULL)
		vederror("Can't open clipboard file, %s", strerror(errno));

	return fp;
}


void
block_paste(void)
{
	FILE *fp;
	int len, t;

	fp = openclip("a+");

	len = get_filesize(fp);
	if (first_paste == TRUE && len > 0) {
		KEY c;
		c = dialog_msg("Clipbard contains old data (Append/overwrite/quit):");
		if (c == 'q') {
			fclose(fp);
			vederror(NULL);
		}
		if (c == 'o') {
			fclose(fp);
			fp = openclip("w+");
		}
	}
	rewind(fp);

	t = fwrite(&bf->buf[bf->block_start], (bf->block_end - bf->block_start) + 1,
		   sizeof(u_char), fp);
	fclose(fp);
	first_paste = FALSE;

	if (t != 1)
		dialog_msg("WARNING: Error writing to clipboard, %s!", strerror(errno));
}

void
block_cut(void)
{
	block_paste();
	block_delete();
}

/**********************************************************
 * Get the block -- esc-b-g will leave delete the existing data from
 * the buffer, esc-b-y will leave it there for a 2nd pull.
 * mode -  0==delete data in clipboard after grab, 1==save for 2nd grab
 */

static void
blk_do_get(void)
{
	int fsize, t;
	FILE *fp;

	sync_x();

	/* using mode 'a+' will create an empty file if it
	 * doesn't already exist. This means a slightly slower
	 * read, but makes it simpler to ignore 'file doesn't exit
	 * errors. 
	 */
	 
	fp = openclip("a+");

	fsize = get_filesize(fp);

	if (fsize <= 0) {
		fclose(fp);
		vederror("The clipboard is empty");
	}
	if (memory(fsize)) {
		fclose(fp);
		vederror("Not enuf core to read clipboard");
	}
	openbuff(fsize, bf->curpos);

	rewind(fp);

	t = fread(&bf->buf[bf->curpos], fsize, sizeof(u_char), fp);
	fclose(fp);

	if (t != 1)
		dialog_msg("CAUTION: Possible read error getting clipboard data!");

	cursor_change_refresh();
}

/* get block and leave the paste buffer */

void
block_yget(void)
{
	blk_do_get();
}

/* get the block and delete the paste buffer */

void
block_get(void)
{
	blk_do_get();
	truncate(clipname, 0);
}


/* EOF */
