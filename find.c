/* find.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

static u_char *findstring = NULL;


static void
finder(int direction)
{
	int new, current;
	u_char *s;
	KEY k;

	sync_x();

	current = bf->curpos;

	new = -1;

	if (direction > 0) {
		s = find_pattern(&bf->buf[current + 1], findstring, bf->bsize - current, 1);
		if (s != NULL)
			new = s - bf->buf;
	}
	if (new == -1) {
		if (current)
			current--;
		s = find_pattern(&bf->buf[current], findstring, current, -1);
		if (s != NULL)
			new = s - bf->buf;
	}
	if (new == -1) {
		vederror("Can't find target '%s' in buffer", findstring);

		/* UNREACHABLE */
	}
	if (direction > 0 && new < current) {
		k = dialog_msg("Found backwards...move to (yes/No):");
		if (k != 'y')
			return;
	}
	cursor_moveto(new);
	hilight(new, strlen(findstring));
}

static void
find_input(char *s, int direction)
{
	char *target;

	dialog_clear();

	target = dialog_entry(s);
	if (target == NULL || *target == '\0')
		return;

	free(findstring);
	findstring = strdup(target);
	finder(direction);

}


void
find(void)
{
	find_input("Find:", 1);
}

void
find_next(void)
{
	if (findstring)
		finder(1);
}

void
find_back(void)
{
	find_input("^Find:", -1);
}

void
find_prior(void)
{
	if (findstring)
		finder(-1);
}



/* EOF */
