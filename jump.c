/* jump.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

#define JBUFMAX 10

int jstacktop[MAXBUFFS];	/* top of stacks */
int jstack[MAXBUFFS][JBUFMAX];	/* jump stacks */

/*********************************************************************
 * Moves to the new position determined by jump()
 * Maintain the jump stack.
 */


static void
jumpto(int pos)
{
	int t;

	if (jstacktop[cur_buffer] >= JBUFMAX - 1) {
		for (t = 0; t < JBUFMAX - 1; t++)
			jstack[cur_buffer][t] = jstack[cur_buffer][t + 1];
		jstacktop[cur_buffer] = JBUFMAX - 2;
	}
	/* store current pos in stack */

	jstack[cur_buffer][++jstacktop[cur_buffer]] = bf->curpos;

	/* now move to the desired position */

	cursor_moveto(pos);
}

static int
current_line(void)
{
	int t, count;

	for (t = 1, count = 0; count < bf->curline;) {
		if ((bf->buf[bf->linestarts[count + 1] - 1]) == EOL)
			t++;
		count++;
	}
	return t;
}

/***********************************************************
 */

void
jump_init(int buff)
{
	jstacktop[buff] = -1;
}

void
jump(void)
{
	u_char *buf, *p, *new;
	int value, err, t, count, len, offset;
	char offset_flag = 0;

	dialog_clear();
	dialog_addline("Jump to [+/-]line[:offset], [+/-]label,[+/-]%%pos, or '.'");
	buf = dialog_entry(":");

	if (buf == NULL || *buf == '\0')
		return;

	p = buf;		/* we bugger 'p', so remember the buffer start */

	/* to DOT ==last change made to buffer */

	if (strcmp(p, ".") == 0) {
		if (bf->lastchange == -1)
			vederror("No last edit point for jump");
		return jumpto(bf->lastchange);
	}
	if (strcmp(p, "-") == 0) {	/* jump to last jumpoff point */
		if (jstacktop[cur_buffer] == -1) {
			beep();
		} else {
			value = jstack[cur_buffer][jstacktop[cur_buffer]--];
			if (value <= bf->bsize)
				cursor_moveto(value);
		}
		return;
	}
	/* Is there an offset flag? Offsets are relative to the current line
	 * if there is a leading -/+. For line numbers we add/sub the offset
	 * to the current line number. See below for labels
	 */

	if (*p == '+') {
		p++;
		offset_flag = 1;
	} else if (*p == '-') {
		p++;
		offset_flag = -1;
	}
	/* Jump to line number if input was a value
	 * Note that we have to step though the buffer
	 * to find the line since we SKIP cont lines...
	 */

	if (v_isdigit(*p) || *p == '$' || *p == '\'') {
		p = get_exp(&value, p, &err);
		if (err)
			vederror("Expression error in line number");
		if (value <= 0)
			vederror("Line numbers must be greater than 0");

		if (offset_flag == 1) {
			value += current_line();
		} else if (offset_flag == -1) {
			value = current_line() - value;
			if (value < 1)
				value = 1;
		}
		offset = 0;
		if (*p == ':' || *p == ' ') {
			p++;
			get_exp(&offset, p, &err);
			if (err)
				vederror("Expression error in offset");
			if (offset < 0)
				vederror("Offset must be greater than 0");
		}
		for (t = 1, count = 0;;) {
			if (t == value) {
				if (bf->linestarts[count] + offset >= bf->linestarts[count + 1])
					offset = bf->linestarts[count + 1] -
					    bf->linestarts[count] - 1;
				return jumpto(bf->linestarts[count] + offset);
			}
			if (bf->buf[bf->linestarts[count + 1] - 1] == EOL)
				t++;
			count++;

			if (count > bf->numlines - 1)
				vederror("Line number out of range,"
					 " we only have %d lines", t);
		}
	}
	/* jump to a percentage position in the file */

	if (*p == '%') {
		get_exp(&value, ++p, &err);
		if (err)
			vederror("Expression error");
		if (value < 0 || value > 100)
			vederror("Percentages must be entered in the range 0..100");

		len = bf->bsize;

		if (offset_flag == -1) {
			value = bf->curpos - ((len / 100) * value);
			if (value < 0)
				value = 0;
		} else if (offset_flag == 1) {
			value = bf->curpos + ((len / 100) * value);
			if (value > len)
				value = len;
		} else {
			value = (len / 100) * value;
		}
		return jumpto(value);
	}
	/* Jump to a label. A label must start a line, be 
	 * terminated by a non - alpha / numeric.Any leading spaces 
	 * in the input are skipped-- that way numbers, leading %, +,
	 * and - can be entered as labels.
	 *
	 * If an offset was specified the search will start at the current 
	 * location, not the start.A - offset jumps back, +forward. Handy 
	 * if you want to jump to the "next" label(ie a #include, etc.)
	 */

	while (v_isblank(*p))
		p++;		/* strip leading blanks from input */
	len = strlen(p);
	new = &bf->buf[(offset_flag == 0) ? 0 : bf->curpos];
	new += offset_flag;
	if (offset_flag == 0)
		offset_flag = 1;

	for (;; new += offset_flag) {
		if (new < bf->buf || new > &bf->buf[bf->bsize])
			break;

		new = find_pattern(new, p, offset_flag == 1 ?
		 &bf->buf[bf->bsize] - new : new - bf->buf, offset_flag);

		if (new == NULL)
			break;

		if (((new - 1) > bf->buf) && (*(new - 1) != EOL))
			continue;
		if (((new + len) < bf->buf + bf->bsize) && (v_isalnum(*(new + len))))
			continue;

		jumpto(new - bf->buf);
		hilight(bf->curpos, len);
		return;
	}

	vederror("Can't find label '%s'", p);
}


/**************************************************
 * Mark the current position as the 'position'
 */

void
mark_position(void)
{
	sync_x();
	bf->pos_mark = bf->curpos;
}

void
mark_jump(void)
{
	if (bf->pos_mark >= 0 && bf->pos_mark < bf->bsize)
		cursor_moveto(bf->pos_mark);
}


/* EOF */
