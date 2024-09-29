/* openfile.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

/* **********************************************************
 * Open a file for output. This is called by savebuf(),
 * saveblk() and save_env().
 * 
 * A valid FILE ptr is returned if the file is opened,
 * a return of NULL indicates that the file was NOT opened.
 */

FILE *
open_outfile(char *f, int ask)
{
	FILE *fp = NULL;
	u_char c;
	char *flags = "w";


	/* no filename, returns NULL */

	if (f == NULL || f[0] == '\0')
		goto EXIT;

	if (f[0] == '|')
		vederror("This function does not support writing to a pipe");

	f = tilde_fname(f);
	if (access(f, F_OK) == 0) {
		if (ask == PROMPT_OVERWRITE) {
			c = dialog_msg("File already exists, okay to overwrite (yes/No):");

			if (c != 'y')
				vederror(NULL);

			flags = "w";

		} else if (ask == PROMPT_APPEND) {
			c = dialog_msg("File already exists; Overwrite, Append or Quit:");

			if (c == 'o')
				flags = "w";

			else if (c == 'a')
				flags = "a";
			else
				vederror(NULL);
		}
	}
	fp = fopen(f, flags);

	if (fp == NULL)
		vederror("Can't open %s for writing, %s", f, strerror(errno));
      EXIT:
	return fp;
}

time_t
get_file_mod_date_name(char *f)
{
	struct stat stats;

	if (stat(f, &stats))
		return 0;
	return stats.st_mtime;
}

time_t
get_file_mod_date(FILE * fp)
{
	struct stat stats;

	if (fstat(fileno(fp), &stats))
		return 0;

	return stats.st_mtime;
}

int
get_filemode(char *f)
{
	struct stat stats;

	if (stat(f, &stats))
		return 0;
	return stats.st_mode;
}

/****************************************
 * Set the filename for the buffer
 * and the extension
 */

void
set_filename(char *f)
{
	char *p;

	free(bf->filename);
	free(bf->file_ext);
	bf->filename = NULL;
	bf->file_ext = NULL;

	if (f && *f) {
		bf->filename = strdup(f);

		p = rindex(f, '.');
		if (p != NULL)
			bf->file_ext = strdup(p + 1);
	}
}

unsigned int
get_filesize(FILE * fp)
{
	unsigned sz;

	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	rewind(fp);

	return sz;
}

/* EOF */
