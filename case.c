/* case.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

int v_toupper();
int v_tolower();

void
caser(void)
{
	int count, start, end, t;
	int (*cfunc) (int);
	u_char a;

	cursor_endword();
	end = bf->curpos;
	cursor_startword();
	start = bf->curpos;

	/* find first alpha in word */

	while (!(v_isalpha(bf->buf[start])) && start < end)
		start++;

	if (!(v_isalpha(bf->buf[start])))
		return;

	if (v_islower(bf->buf[start])) {
		/* just change this character */

		bf->buf[start] = v_toupper(bf->buf[start]);
		count = 1;
	} else {
		/* toggle to case 2nd alpha char in word */

		for (t = start + 1, cfunc = NULL; t <= end; t++) {
			a = bf->buf[t];
			if (v_isalpha(a)) {
				cfunc = (v_islower(a)) ? v_toupper : v_tolower;
				break;
			}
		}

		/* if we didn't set (no alpha after 1st), then use 1st */

		if (cfunc == NULL)
			cfunc = (v_islower(bf->buf[start])) ? v_toupper : v_tolower;

		for (t = start; t <= end; t++) {
			bf->buf[t] = (u_char) (*cfunc) (bf->buf[t]);
		}
		count = (end - start) + 1;
	}
	cursor_moveto(start);
	hilight(start, count);
	bf->lastchange = start;
	bf->changed = TRUE;
}

/*********************************************************************
 * Transpose the characters or word  at the current position
 * 
 * 3 functions in 1! Depending on the current character...
 * if cursor at character, transpose with next char,
 * if cursor at SPACE, transpose words before/after cursor,
 * if cursor at LF, transpose lines
 */


static inline void
transpose_chars(void)
{

	u_char a, *s;

	s = &bf->buf[bf->curpos];

	a = *s;
	*s = *(s + 1);
	*(s + 1) = a;

	bf->lastchange = bf->curpos;
	bf->changed = TRUE;

	hilight(bf->curpos, 2);
}

static inline void
transpose_words(void)
{
	u_char *w, *s;
	int l1, l2;
	int orig, start, endpos;

	orig = bf->curpos;	/* current cursor position */

	/* move to prior word and copy it to a buffer */

	cursor_left();
	cursor_startword();
	start = bf->curpos;
	s = bf->buf;		/* buffer pointer (avoid using -> later) */
	l1 = orig - start;	/* len of word 1 */
	cursor_moveto(orig);
	cursor_right();

	cursor_endword();
	endpos = bf->curpos;
	l2 = endpos - orig;	/* len of word 2 */
	w = malloc(l1 * sizeof(u_char));

	if (w == NULL) {
		cursor_moveto(orig);
		vederror("Memory allocation failure in word transpose");
	}
	memcpy(w, s + start, l1 * sizeof(u_char));	/* save word 1 */

	memmove(s + start, s + start + l1 + 1, l2 * sizeof(u_char));	/* move word 2 */
	memcpy(s + start + l2 + 1, w, l1 * sizeof(u_char));
	*(s + start + l2) = ' ';
	free(w);
	cursor_moveto(start + l2);
	bf->lastchange = start;
	bf->changed = TRUE;

	hilight(start, l1 + l2 + 1);
}

static inline void
transpose_lines(void)
{
	u_char *w, *s;
	int line, ls1, ls2;
	int orig;

	if (bf->curline >= bf->numlines - 1)
		return;		/* nada if at eof */

	s = bf->buf;		/* avoid -> refs later */
	orig = bf->curpos;

	cursor_linestart();

	line = bf->curline;
	ls1 = bf->linestarts[line + 1] - bf->linestarts[line];
	ls2 = bf->linestarts[line + 2] - bf->linestarts[line + 1];

	if ((*(s + bf->linestarts[line + 2] - 1)) != EOL) {
		cursor_moveto(orig);
		vederror("Both lines in a transpose must terminate with a EOL."
			 " Try turning off linewrap");
	}
	w = malloc(ls1 * sizeof(u_char));
	if (w == NULL) {
		cursor_moveto(orig);
		vederror("Memory allocation failure in line transpose");
	}
	memcpy(w, s + bf->linestarts[line], ls1 * sizeof(u_char));	/* save line 1 */
	memmove(s + bf->linestarts[line], s + bf->linestarts[line + 1],
		ls2 * sizeof(u_char));
	memcpy(s + bf->linestarts[line] + ls2, w, (ls1 - 1) * sizeof(u_char));
	wrap_buffer(bf->wrapwidth, bf->curline);

	free(w);

	cursor_lineend();
	redisplay();
	bf->lastchange = bf->curpos;
	bf->changed = TRUE;

	hilight(bf->linestarts[line], ls1 + ls2);
}

void
transpose(void)
{
	u_char *s;
	u_char a;
	int pos;

	sync_x();

	pos = bf->curpos;
	s = &bf->buf[pos];

	a = *s;

	if (a > ' ' && pos < bf->bsize - 1 && *(s + 1) > ' ')
		transpose_chars();

	else if (a == ' ' && pos > 0 && *(s - 1) > ' ' &&
		 pos < bf->bsize - 1 && *(s + 1) > ' ')
		transpose_words();

	else if (a == EOL && pos < bf->bsize - 1)
		transpose_lines();
}

/* EOF */
