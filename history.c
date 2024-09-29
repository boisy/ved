/* history.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

/* This file contains the routines and variables needed to maintain
 * the command line history.
 */

#include "ved.h"

void save_history(u_char * p);

#define MAXHISTORY 20

static char *history_lines[MAXHISTORY] =
{NULL};
static int history_count = -1;
static int currhist = -1;
static FLAG history_force = FALSE;

static char *history_filename = NULL;


void
set_history_pointer(void)
{
	currhist = history_count;
}

/*********************************************************************
 * Set the history filename (called from env)
 */

void
set_history_filename(char *p)
{
	free(history_filename);
	history_filename = NULL;
	if(p != NULL && *p != '\0' )
		history_filename = strdup(p);
}

char *
get_history_filename(void)
{
	return history_filename;
}

void
set_history_force(char *p)
{
	history_force = istrue(p);
}

int
get_history_force(void)
{
	return history_force;
}

/*********************************************************************
 * Save the history stack to a file
 */

void
history_savefile(void)
{
	FILE *fp;
	int t;

	/* nothing to save?? */

	if (history_count < 0)
		return;
		
	/* nowhere to save ?? */
	
	if( history_filename == NULL || *history_filename == '\0' )
		return;

	/* history doesn't exist */

	if ((history_force == FALSE) && (access(tilde_fname(history_filename), F_OK) != 0))
		return;

	fp = fopen(tilde_fname(history_filename), "w");

	if (fp == NULL)
		return;		/* silent error...should never happen */

	for (t = 0; t <= history_count; t++) {
		if (history_lines[t])
			fprintf(fp, "%s\n", history_lines[t]);

	}
	fclose(fp);
}

/*********************************************************************
 * Load the history buffer from disk. All this does is open a file
 * and then read lines and pass them to save_history(). 
 */


void
history_loadfile(void)
{
	FILE *fp;
	u_char buf[200], *p;
	
	if(history_filename == NULL)
		return;
		
	fp = fopen(tilde_fname(history_filename), "r");

	if (fp == NULL)
		return;

	history_count = -1;
	currhist = -1;

	while (1) {
		if (fgets(buf, sizeof(buf) - 2, fp) == NULL)
			break;

		if ((p = index(buf, '\n')) != NULL)
			*p = 0;
		save_history(buf);
	}
	fclose(fp);
}

/*********************************************************************
 * Save a line to the history stack
 */

void
save_history(u_char * p)
{
	int t;
	char *stored;

	if (!p || *p == '\0')
		goto EXIT;	/* ignore empty strings */

	/* check for duplicates */

	for (t = 0; t <= history_count; t++) {
		if (strcmp(history_lines[t], p) == 0)
			goto EXIT;
	}

	/* save the new history string */

	if ((stored = strdup(p)) == NULL)
		goto EXIT;	/* if no storage avail, ignore */

	/* Allocate a storage pointer. If all have been used, compress the
	 * array and store in last position
	 */

	if (history_count == MAXHISTORY - 1) {
		free(history_lines[0]);
		for (t = 1; t < MAXHISTORY; t++)
			history_lines[t - 1] = history_lines[t];
		if (currhist == history_count)
			currhist--;
		history_count--;
	}
	history_lines[++history_count] = stored;

      EXIT:;
}

/*********************************************************************
 * Find a previous/prior line from the history stack. A NULL pointer
 * is returned if no acceptable new line is found.
 */

char *
get_next_history(u_char * curr_line, int direction)
{
	char *p;

	while (1) {
		p = NULL;
		if ((direction < 0 && currhist < 0)
		    || (direction > 0 && currhist >= history_count))
			break;

		if (direction > 0)
			currhist++;
		p = history_lines[currhist];
		if (direction < 0)
			currhist--;
		if (strcmp(p, curr_line) != 0)
			break;
	}
	return p;
}


/* EOF */
