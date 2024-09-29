/* delete.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

void delete_endword(void);

/*********************************************************************
 * These are the variables used for delete/undelete. A circular stack of
 * MAXDELS is used, each entry contains a size and storage pointer. If the
 * top and bottom ptrs are equal there is nothing to restore, otherwise
 * there are mod(top)-mod(bot) buffers to restore.
 */

#define MAXDELS 10

static struct {
	int delsize;		/* size of this deletion */
	u_char *delbuff;	/* address of deletion buffer */
} delstack[MAXDELS];

static int botdstack = 0;	/* points to bottom of del stack */
static int topdstack = 0;	/* points to top of del stack           */

void
un_delete(void)
{
	u_char *buff;
	int size;

	sync_x();

	buff = delstack[topdstack].delbuff;

	if (buff == NULL)
		return;		/* nothing stored */

	size = delstack[topdstack].delsize;


	/* do  restore */

	/* if the memory increase fails the buffer data is still available. */

	if (memory(size))
		vederror("There is not enough core memory to restore the %d byte deletion",
			 size);

	/* zap it back */

	openbuff(size, bf->curpos);
	memcpy(&bf->buf[bf->curpos], buff, size * sizeof(u_char));

	wrap_buffer(bf->wrapwidth, bf->curline);
	redisplay();

	/* clean up buffer */

	free(delstack[topdstack].delbuff);	/* return buffer memory to system */
	delstack[topdstack].delbuff = NULL;	/* mark buffer as empty */
	if (topdstack != botdstack) {	/* adjust the top pointer */
		topdstack--;
		if (topdstack < 0)
			topdstack = MAXDELS - 1;
	}
}


/*********************************************************************
 * Actually do a deletion. Used by yankword(), deleol(),
 * as well as block delete.
 * 
 * start and end are both offsets into the current buffer.
 */

void
del_save(int start, int end)
{
	u_char *buff;
	int size;
	KEY k;

	dialog_clear();

	/* never delete the final char in the buffer */

	if (end == bf->bsize - 1)
		end--;

	if (end < start || start < 0 || end > bf->bsize - 1)
		vederror("Internal error in %s, attempt to delete "
			 "outside of buffer (start=%d, end=%d",
			 __FUNCTION__, start, end);

	size = (end - start) + 1;
	cursor_moveto(start);

	/* make an attempt to save the stuff we are deleting */

	topdstack = ++topdstack % MAXDELS;	/* adjust the top pointer */

	/* If the top pointer has caught up to the bottom then we have to
	 * delete the bottom buffer and adjust the bottom pointer up one
	 */

	if (botdstack == topdstack) {
		free(delstack[botdstack].delbuff);
		botdstack = ++botdstack % MAXDELS;
	}
	buff = malloc(size * sizeof(u_char));

	/* Do the delete. If no buffer for undelete, then we
	 * advise the user that he won't be able to restore
	 */

	if (buff == NULL) {
		k = dialog_msg("Not saving deletion, proceed (yes/No):");
		if (k != 'y')
			return;

	}
	if (buff != NULL) {
		memcpy(buff, bf->buf + start, size * sizeof(u_char));
		delstack[topdstack].delsize = size;
		delstack[topdstack].delbuff = buff;
	}
	closebuff(size, start);	/* do the deletion, adjust markers */
	cursor_change_refresh();
}

void
delete_line(void)
{
	int start, end;

	cursor_linestart();
	start = bf->linestarts[bf->curline];
	end = bf->linestarts[bf->curline + 1] - 1;
	if (end >= bf->bsize - 1)
		end = bf->bsize - 2;
	if (end > start)
		del_save(start, end);
}

/* delete from cursor to end of screen line */

void
delete_toeos(void)
{
	int start, end;

	sync_x();
	start = bf->curpos;
	end = bf->linestarts[bf->curline + 1] - 2;
	if (end > start)
		del_save(start, end);
	else
		cursor_moveto(start);
}

/* delete from cursor to next EOL */

void
delete_toeol(void)
{
	int start, end;

	sync_x();

	start = bf->curpos;

	cursor_endpp();
	cursor_left();
	end = bf->curpos;

	if (end > start)
		del_save(start, end);
	else
		cursor_moveto(start);
}

/* delete the complete current word */

void
delete_word(void)
{
	cursor_startword();
	delete_endword();
}

void
delete_endword(void)
{
	int start;

	start = bf->curpos;
	cursor_endword();
	del_save(start, bf->curpos);
}

/* EOF */
