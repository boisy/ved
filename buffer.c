/* buffer.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

int
make_buffer(void)
{
	BUFFER *b;

	if (num_buffers >= MAXBUFFS )
		vederror("Attempt to create too many buffers");

	b = &buffers[num_buffers];

	if ((b->mw = newwin(ysize, xsize, 0, 0)) == NULL)
		return 1;

	wbkgdset(b->mw, display_attrs[EDITOR_MENU] | ' ');
	keypad(b->mw, TRUE);
	meta(b->mw, TRUE);
	scrollok(b->mw, FALSE);
	werase(b->mw);
	create_edit_menu(b->mw);

	if ((b->win = newwin(ysize - 2, xsize - get_scroll_indicator_change(),
			1, get_scroll_indicator_offset())) == NULL)
		return 2;

	wbkgdset(b->win, display_attrs[EDITOR_NORMAL] | ' ');
	keypad(b->win, TRUE);
	scrollok(b->win, TRUE);
	meta(b->win, TRUE);
	werase(b->win);
	wrefresh(b->win);

	b->buf = NULL;		/* let file reader worry about buffer memory */
	b->bsize = 0;		/* empty buffer */
	b->balloc = 0;

	b->xsize = xsize - get_scroll_indicator_change();
	b->ysize = ysize - 2;
	b->curpos = 0;		/* gotta put the cursor somewhere */
	b->x = 0;
	b->y = 0;
	b->curline = 0;
	b->xoffset = 0;
	b->lastchange = -1;
	b->changed = FALSE;
	b->needbackup = FALSE;
	b->dirtyscreen = TRUE;
	b->xsync = FALSE;
	
	b->block_start = -1;
	b->block_end = -1;
	b->hilite_start = -1;
	b->hilite_end = -1;
	b->pos_mark = -1;

	b->scrollbar_pos = 1;
	b->wrapwidth = b->xsize - 1;
	
	b->filename = NULL;
	b->file_ext = NULL;
	
	b->linestarts = NULL;
	b->linestarts_size = 0;
	b->numlines = 0;
		jump_init(num_buffers);	/* init jump buffer pointers */

	wm_add(b->mw);
	wm_add(b->win);
	num_buffers++;

	return 0;
}


/*********************************************************************
 * Memory allocator. This function checks to see the the requested
 * amount of memory is available in the current buffer. If it is
 * then 0 is returned to the caller. Otherwise a call is done to
 * realloc() in an attempt to create a larger buffer. If this
 * call fails -1 is returned. 
 */


int
memory(int s)
{
	int memreq;
	u_char *newbuf;

	if (bf->bsize + s + 2 >= bf->balloc) {
		memreq = (bf->balloc) + 8000 + s;
		newbuf = realloc(bf->buf, memreq * sizeof(u_char));

		if (newbuf == NULL)	/* can't get more memory */
			return -1;

		bf->buf = newbuf;
		bf->balloc = memreq;
	}
	return 0;
}

/*********************************************************************
 * Open up the buffer by 'count' bytes at 'pos'. Most routines
 * will have checked to see if memory is available BEFORE
 * coming here.
 */

void
openbuff(int count, int pos)
{
	u_char *s, *end;

	if ((bf->bsize) + count >= bf->balloc) {
		if (memory(count))
			vederror("No more core memory to allocate for buffer");
	}
	if (pos > bf->bsize)
		pos = bf->bsize;

	s = &bf->buf[pos];
	end = &bf->buf[bf->bsize];

	memmove(s + count, s, (end - s) * sizeof(u_char));
	bf->bsize += count;

	if (bf->block_start > pos)
		bf->block_start += count;
	if (bf->block_end > pos)
		bf->block_end += count;
	if (bf->pos_mark > pos)
		bf->pos_mark += count;

	if (bf->curpos > pos)
		bf->curpos += count;

	bf->lastchange = pos;
	bf->changed = TRUE;
}

/*********************************************************************
 * Close the buffer by 'count' bytes at 'pos'
 */

void
closebuff(int count, int pos)
{
	u_char *s, *s1;
	int num;

	if (count + pos >= bf->bsize)
		count = bf->bsize - pos;

	if (bf->curpos > pos)
		bf->curpos -= count;

	s = &bf->buf[pos];	/* current pos */
	s1 = s + count;
	num = &bf->buf[bf->bsize] - s1;

	memmove(s, s1, num);

	bf->bsize -= count;

	if (bf->block_start >= pos) {
		if (bf->block_start <= (pos + count))
			bf->block_start = -1;
		else
			bf->block_start -= count;
	}
	if (bf->block_end >= pos) {
		if (bf->block_end <= (pos + count))
			bf->block_end = -1;
		else
			bf->block_end -= count;
	}
	if (bf->pos_mark >= pos) {
		if (bf->pos_mark <= (pos + count))
			bf->pos_mark = -1;
		else
			bf->pos_mark -= count;
	}
	bf->lastchange = pos;
	bf->changed = TRUE;
}


/* EOF */
