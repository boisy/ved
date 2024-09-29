/* quit.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

FLAG quitting_all = FALSE;

/**************************************************
 * The quit command from the edito/menur comes here. We
 * handle stuff like saving and deleting buffers
 */

void
quit(void)
{
	KEY k;

	dialog_clear();

	while (bf->changed == TRUE) {
		k = dialog_msg("Closing %s, do you want to save (Yes/no):",
		 bf->filename == NULL ? "Unnamed buffer" : bf->filename);

		switch (k) {

			case '.':
				if (bf->lastchange) {
					cursor_moveto(bf->lastchange);
					curkey();
				}
				break;


			case 'n':
				k = dialog_msg("Sure you want to abandon file (yes/No):");
				if (k != 'y')
					return;
				bf->changed = FALSE;
				break;

			case 'd':
				save_buffer(DOS);;
				break;

			case 'm':
				save_buffer(MAC);
				break;

			case '?':
			case 'h':
				dialog_addline("'N'o save, 'D'os format, M'ac format,"
				      " '.'-to last change 'Q'uicksave");
				break;

			case 'q':
				retkey(EOL);
				save_buffer(NATIVE);
				break;

			default:
				save_buffer(NATIVE);
				break;
		}

	}

	/* Now delete the buffer memory */

	free(bf->buf);
	free(bf->linestarts);
	free(bf->filename);
	free(bf->file_ext);

	delwin(bf->win);
	delwin(bf->mw);
	wm_delete(bf->win);
	wm_delete(bf->mw);

	/* Move all the buffer stuff past the one deleted down
	 * by one notch.
	 */

	memmove(&buffers[cur_buffer], &buffers[cur_buffer + 1],
		sizeof(buffers[0]) * ((MAXBUFFS) - cur_buffer));



	/* Prob not necessary, but avoid potential free()
	 * errors later by setting buffers in the last
	 * buffer to NULL. 
	 */

	buffers[num_buffers].buf = NULL;
	buffers[num_buffers].linestarts = NULL;
	buffers[num_buffers].filename = NULL;
	buffers[num_buffers].file_ext = NULL;

	num_buffers--;

	if (num_buffers <= 0)
		ved_exit(NULL);

	if (cur_buffer > num_buffers - 1)
		cur_buffer = num_buffers - 1;

	bf = &buffers[cur_buffer];
	wm_add(bf->mw);
	wm_add(bf->win);

	if (quitting_all == FALSE)
		refresh_all();
}

void
quit_all(void)
{
	u_char *p;

	quitting_all = TRUE;

	cur_buffer = 0;

	while (1) {
		bf = &buffers[cur_buffer];
		p = bf->buf;
		quit();

		if (p == bf->buf) {
			if (cur_buffer == num_buffers - 1)
				break;

			cur_buffer++;
			wm_refresh();
		}
	}
	quitting_all = FALSE;

	wm_refresh();
}

/* EOF */
