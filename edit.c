/* edit.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

void edit(KEY k);

#define INCREASE_SIZE 200

static FLAG doing_changeword = FALSE;

void
change_word(void)
{
	sync_x();
	bf->block_start = bf->curpos;
	cursor_endword();
	bf->block_end = bf->curpos;
	cursor_moveto(bf->block_start);
	doing_changeword = TRUE;
	redisplay();
	edit(curkey());
	doing_changeword = FALSE;
	block_delete();
}

void
delete(void)
{
	edit(DELETE);
}

void
backspace(void)
{
	edit(BACKSPACE);
}

void
insert(void)
{
	edit(INSERT);
}


void
insert_tab(void)
{
	edit(INSERT_TAB);
}

static inline int
set_edx(u_char * b, int offset)
{
	int x = 0;
	int pos = 0;

	while (pos < offset) {
		x += csize(b[pos++], x);
	}
	return x;
}

/*********************************************************************
 * Edit a screen line
 */

void
edit(KEY k)
{
	static u_char *buf = NULL;
	int linelen, ebufsize = 0, edpos, origx, t, y, pos, orig_bstart,
	    orig_bend, bufferline_len, temp1, temp2;

	u_char *bufferline_pos, *p;
	FLAG blockflag = FALSE;

	sync_x();

	/* save the original position pointers */

	origx = bf->x;
	edpos = bf->curpos - bf->linestarts[bf->curline];

	retkey(k);

	/* copy the current screen line to a buffer */

	linelen = bf->linestarts[bf->curline + 1] - bf->linestarts[bf->curline];
	bufferline_len = linelen;

	bufferline_pos = &bf->buf[bf->linestarts[bf->curline]];

	if (ebufsize < linelen + INCREASE_SIZE || memory(ebufsize)) {
		buf = realloc(buf, sizeof(u_char) * (linelen + INCREASE_SIZE));
		if (buf == NULL)
			vederror("Unable to edit...no core memory for buffer");
		ebufsize = linelen + INCREASE_SIZE;
	}
	memcpy(buf, bufferline_pos, linelen * sizeof(u_char));

	orig_bstart = bf->block_start;
	orig_bend = bf->block_end;

	if ((orig_bstart != -1) || (orig_bend != -1))
		blockflag = TRUE;

	bf->block_start = -1;
	bf->block_end = -1;
	
	/* If we have a valid block defined we set the start/end of the block
	 * to be either within the line being edited, or to the start and/or end
	 * of the line. In other words, while the line is being edited the
	 * entire block is in the line. This makes it easy to hilight the block
	 * during editing and to adjust the markers during editing. The real
	 * positions are restored after the edit is complete.
	 */

	if (orig_bstart != -1 && orig_bstart < orig_bend && orig_bend > orig_bstart) {

		t = bf->linestarts[bf->curline];

		if ((orig_bstart < t + linelen) && (orig_bend >= t)) {
			if (orig_bstart < t)
				bf->block_start = 0;
			else
				bf->block_start = orig_bstart - t;

			t += linelen;
			if (orig_bend > t)
				bf->block_end = linelen;
			else
				bf->block_end = linelen - (t - orig_bend);
		}
	}
	
	/* For C-mode we handle two issues when the entry point is
	 * on a EOL or WHITESPACE-EOL:
	 *      1. if key is '#' strip out leading WHITESPACE
	 *      2. if key is '}' detab by one position
	 *      3. if prior line ends in '{' increase tab by one pos
	 */

	if (opt_cmode == TRUE) {

		/* Find the first non-space in the current line. This
		 * is safe since a line always ends in an EOL.
		 */

		for (t = 0, p = buf;; t++) {
			if (p[t] != ' ' && p[t] != '\t')
				break;
		}

		/* Now do c-stuff if the line is space[s]-EOL */

		if (p[t] == EOL) {

			/* (1) we're inserting a '#' at the start of
			 * a line. We just strip out the whitespace we
			 * parsed earlier...
			 */

			if (k == '#') {
				edpos = 0;
				linelen = 1;
				if (bf->block_start != -1) {
					bf->block_start = 0;
					bf->block_end = 0;
				}
				buf[0] = EOL;
			}
			/* (2) we're inserting a '}' at the start of a
			 * line. We strip out a level on indent. This is
			 * either a single space or tab. We don't worry
			 * about folks who think a 'tab' is several spaces.
			 */

			if (k == '}' && edpos) {
				edpos--;
				linelen--;
				if (bf->block_start > linelen)
					bf->block_start = linelen;
				if (bf->block_end > linelen)
					bf->block_end = linelen;
				buf[linelen - 1] = buf[linelen];
			}
		}
	}
	
	/* edit loop.... */

	while (1) {

		/* set the xpos, update the screen and get a key */

		bf->x = set_edx(buf, edpos);

		/* display the edit buffer line */

		set_xoffset();
		wmove(bf->win, bf->y, 0);
		wclrtoeol(bf->win);
		display_line_part(bf->y, 0, buf, 0, linelen);

		wmove(bf->win, bf->y, bf->x - bf->xoffset);
		k = ved_getkey(bf->win);

		/* Insert/overlay a valid key into the line buffer.
		 * This includes everything from Space to the max ascii
		 * char $ff.
		 */

		if ((k == EOL) && (doing_changeword == TRUE)) {
			k = 0;
			break;
		}
		if (k == INSERT || k == '\n' || (k >= ' ' && k <= 0xff) ||
		    (k == INSERT_TAB)) {

			/* We insert (make a hole in the buffer) if
			 * insert mode is on, or if we have a EOL, INSERT or INSERT_TAB,
			 * or if the current character is an EOL.
			 */

			if ((opt_insert == TRUE) ||
			    (buf[edpos] == EOL) ||
			    (k == INSERT) || (k == INSERT_TAB)) {

				/* Check to make sure buffer doesn't overflow */

				if (linelen + 1 >= ebufsize) {
					retkey(k);
					break;
				}
				/* Make a hole */

				memmove(buf + edpos + 1, buf + edpos,
				     sizeof(u_char) * (linelen - edpos));
				if (bf->block_start >= edpos)
					bf->block_start++;
				if (bf->block_end >= edpos)
					bf->block_end++;
				linelen++;
				if (linelen >= ebufsize - 2)
					break;
			}
			/* Now a hole is made, or we're doing an overwrite */

			if (k == INSERT_TAB)
				k = TAB;
			if (k == INSERT)
				buf[edpos] = ' ';
			else
				buf[edpos++] = k;

			if ((doing_changeword == TRUE) && (edpos >= bf->block_start))
				edpos = bf->block_start;

			if ((edpos >= linelen) ||
			    (k == EOL) ||
			    (bf->wrapwidth && bf->x >= bf->wrapwidth))
				break;

			continue;
		}
		/* Backspace is just Backspace + Delete.
		 * Note, we're pushing keystrokes on a stack so
		 * do it in reverse order.
		 */

		if (k == BACKSPACE) {
			retkey(DELETE);
			retkey(CURSOR_LEFT);
			continue;
		}
		/* Delete. Just remember to exit if we delete the last
		 * character in  the line or a EOL.
		 */

		if (k == DELETE) {

			if ((bf->block_start != -1) && (bf->block_start > edpos))
				bf->block_start--;
			if ((bf->block_end != -1) && (bf->block_end > edpos))
				bf->block_end--;
			if (bf->block_start >= bf->block_end) {
				bf->block_start = -1;
				bf->block_end = -1;
			}

			memmove(buf + edpos, buf + edpos + 1,
				sizeof(u_char) * (linelen - edpos - 1));
			wdelch(bf->win);
			linelen--;
			if (linelen <= 0 || edpos >= linelen)
				break;
			continue;
		}
		/* Move the cursor left/right, etc */

		if (k == CURSOR_LEFT) {
			edpos--;
			if (edpos < 0) {
				edpos = 0;
				retkey(CURSOR_LEFT);
				break;
			}
			continue;
		}
		if (k == CURSOR_LINESTART) {
			edpos = 0;
			continue;
		}
		if (k == CURSOR_RIGHT) {
			edpos++;
			if (edpos >= linelen)
				break;
			if ((doing_changeword == TRUE) && (edpos > bf->block_start))
				edpos = bf->block_start;
			continue;
		}
		if (k == CURSOR_LINEEND) {
			edpos = linelen - 1;
			continue;
		}
		/* Undelete line editing. We just need to restore the
		 * original X screen position. The horizontal scrolling is
		 * taken care of in moveto() ????
		 */

		if (k == UNDELETE) {

			if (blockflag) {
				bf->block_start = orig_bstart;
				bf->block_end = orig_bend;
			}
			bf->x = origx;
			bf->dirtyscreen = TRUE;
			cursor_moveto(bf->curpos);
			return;
		}
		/* Unrecognized key. Push the key back and exit
		 * the edit loop.
		 */

		retkey(k);
		break;

	}			/* end of main loop */

	/* Insert the changed data into the main buffer
	 * Note that we save the current block markers and
	 * then restore them. This is needed since close/openbuff()
	 * is smart enuf the update the block marker positions, but
	 * we need to do it here since they might have been in the buffer.
	 *					bvdp, 99/05/05
	 */

 	temp1=bf->block_start;
	temp2=bf->block_end;
 
	if (bufferline_len > linelen)
		closebuff(bufferline_len - linelen, bufferline_pos - bf->buf);
	else if (bufferline_len < linelen)
		openbuff(linelen - bufferline_len, bufferline_pos - bf->buf);

	memcpy(bufferline_pos, buf, linelen * sizeof(u_char));

	bf->block_start=temp1;
	bf->block_end=temp2;


	if (blockflag) {
		t = bufferline_pos - bf->buf;

		if (orig_bstart != -1) {
			if (orig_bstart < t)
				bf->block_start = orig_bstart;
			else if (orig_bend > t + bufferline_len)
				bf->block_start = orig_bstart + (linelen - bufferline_len);
			else if (bf->block_start != -1)
				bf->block_start += t;
		}

		if (orig_bend != -1) {
			if (orig_bend < t)
				bf->block_end = orig_bend;
			else if (orig_bend > t + bufferline_len)
				bf->block_end = orig_bend + (linelen - bufferline_len);
			else if (bf->block_end != -1)
				bf->block_end += t;
		}
	}
	/* Don't screw around here. This is simple, but important.
	 * We need the Y position of the of line the cursor was in
	 * when we started the edit (y) and the position of the cursor
	 * in the edit buffer at the end of edit. This is the calculated
	 * as the offset into the edit buffer plus the position of the current
	 * line. 
	 * 
	 * This has be be grabbed before we do the wrap. Wrap does line
	 * wrapping, but it also updates the linestart pointers.
	 * 
	 * After fixing the linestarts, we redisplay the page from the old
	 * top of screen. Then we move the cursor. If the new cursor pos is
	 * off screen, moveto() handles that.
	 */

	y = bf->curline - bf->y;
	pos = bf->linestarts[bf->curline] + edpos;
	bf->changed = TRUE;
	bf->lastchange = pos;

	/* Auto indent if edit terminated with EOL
	 * This gets tricky since the linepointer
	 * stack is not reliable here. So, use brute force...
	 * we scan the buffer from the start keeping track
	 * of EOLs. The position after the last EOL found
	 * before the current position is the previous
	 * indentation to use.
	 */

	if ((k == EOL) && (opt_indent == TRUE)) {

		for (t = 0, p = NULL; t < pos - 1; t++)
			if (bf->buf[t] == EOL)
				p = &bf->buf[t + 1];
		if (p != NULL) {
			t = 0;
			while (p[t] == ' ' || p[t] == '\t')
				t++;
			if (t && memory(t) == 0) {
				openbuff(t, pos);
				memcpy(&bf->buf[pos], p, t * sizeof(u_char));
				pos += t;
			}
		}
	}
	/* For Cmode we check the last 2 chars in the line. Note that
	 * this is -2 and -3, not -1 and -2 as expected since linelen
	 * is incremented after the EOL is hit. If the line ends in
	 * a {EOL then we insert a TAB into the input stream
	 */

	if ((opt_cmode == TRUE) && (linelen > 2) &&
	    (buf[linelen - 2] == EOL) && (buf[linelen - 3] == '{')) {
		if (memory(1) == 0) {
			openbuff(1, pos);
			bf->buf[pos] = '\t';
			pos++;
		}
	}
	/* Wrapping buffer also checks to make sure there is an EOL
	 * at the end of buffer, so don't need to check here to see
	 * if editing delete last one.
	 */

	wrap_buffer(bf->wrapwidth, bf->curline);
	display_page(y);
	cursor_moveto(pos);

	if ((k == EOL) && (opt_auto_number == TRUE)) {
		retkey(MACRO_NUMBER);
	}
}

/* EOF */
