/* replace.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

static KEY replace_mode;
static u_char *replace_with = NULL;
static u_char *replace_target = NULL;


/*********************************************************************
 * Replace a word in the buffer with a new one. This is called
 * from the speller and from rplagain().
 */


void
replace_string(u_char * newword, int pos, int newsize, int oldsize)
{
	int t;
	u_char a, c;
	int notab = 0;
	u_char *s;

	if (index(newword, TAB))
		notab++;

	t = oldsize - newsize;
	if (t > 0)
		closebuff(t, pos + newsize);

	if (t < 0)
		openbuff(-t, pos + oldsize);

	bf->lastchange = pos;
	bf->changed = TRUE;

	for (t = 0, s = &bf->buf[pos]; t < newsize;) {
		a = *s;
		c = *newword++;
		if (t++ < oldsize) {

			/* do wildcard/case stuff only 
			 * * while in 1st part of strings */

			if (a == TAB && c == SPACE && notab == 0)
				c = TAB;	/*  keep tabs  */
			else if (c == wildcard)
				c = a;
			else if (!opt_casematch) {
				if (v_islower(a))
					c = v_tolower(c);
				else if (v_isupper(a))
					c = v_toupper(c);
			}
		}
		*s++ = c;
	}
}


/***************************************************
 * Global find-replace functions
 *
 */


/* We buggered up and need to restore. Called after
 * a memory allocation error when reading back
 * the saved buffer. This should always work...the
 * buffer size never decreases.
 */

static void
global_restore_buffer(FILE * fp, int bufsize)
{
	int t;

	bf->bsize = bf->curpos + bufsize;
	rewind(fp);
	t = fread(&bf->buf[bf->curpos], bufsize, 1, fp);
	fclose(fp);
	if (t != 1)
		vederror("SERIOUS ERROR READING SCRATCH FILE, SUGGEST"
			 " BUFFER SAVE/REBOOT");
	else
		vederror("Cannot expand buffer, no replacements done");
}

/* Global replace. Save the buffer past the currrent position to
 * a scratch file and then read it back in 10K chuncks, do the 
 * replaces and continue. This is done to avoid a large number of
 * small inserts in a large buffer which take a long time to do.
 */

static void
global_replace(int startpos, int endpos)
{
	FILE *fp;
	u_char *s;
	int targsize, replsize, newpos, rcount, bsize, saved_size, chunk,
	    t, old_bend, old_bstart;;


	if ((fp = tmpfile()) == NULL)
		vederror("Can't create scratch file, %s", strerror(errno));

	dialog_adddata("Standby");

	old_bend = bf->block_end;
	old_bstart = bf->block_start;
	bf->block_end = -1;

	bsize = bf->bsize - startpos;
	saved_size = bsize;
	if (fwrite(&bf->buf[startpos], bsize, 1, fp) != 1) {
		fclose(fp);
		vederror("Error writing buffer during global replace, %s",
			 strerror(errno));
	}
	targsize = strlen(replace_target);
	replsize = strlen(replace_with);

	newpos = startpos;
	rcount = 0;

	bf->bsize = startpos;	/* Rudely change the buffer size */

	chunk = 10000;
	rewind(fp);

	while (1) {
		dialog_adddata(".");
		if (chunk > bsize)
			chunk = bsize;
		bsize -= chunk;
		t = bf->bsize;

		if (memory(chunk) != 0)
			global_restore_buffer(fp, saved_size);

		openbuff(chunk, t);
		if (fread(&bf->buf[t], 1, chunk, fp) != chunk) {
			fclose(fp);
			vederror("Serious read problem, buffer contents BAD");
		}
		while (1) {
			if ((newpos >= bf->bsize - 1) || (newpos >= endpos))
				break;

			s = find_pattern(&bf->buf[newpos], replace_target,
					 (endpos < bf->bsize ? endpos : bf->bsize) - newpos, 1);

			if (s == NULL)
				break;

			newpos = s - bf->buf;

			if ((replsize > targsize) && (memory(replsize - targsize) != 0))
				global_restore_buffer(fp, saved_size);

			replace_string(replace_with, newpos, replsize, targsize);

			rcount++;
			newpos += replsize;
			endpos -= (targsize - replsize);
			if (old_bend > -1)
				old_bend -= (targsize - replsize);

		}
		newpos = bf->bsize - targsize + 1;
		if (bsize <= 0)
			break;
	}
	fclose(fp);

	bf->block_end = old_bend;
	bf->block_start = old_bstart;

	cursor_change_refresh();

	dialog_addline("");
	dialog_msg("%d changes made!", rcount);

}

/* Global replace. This is used in next and prompt modes. 'All'
 * comes here, but forks to the global routine.
 */

static void
do_rplagain(void)
{
	int startpos, targsize, replsize, endpos, newpos, rcount;
	u_char *s;
	KEY k;

	if (replace_target == NULL || replace_with == NULL)
		return;


	startpos = bf->curpos;

	targsize = strlen(replace_target);
	replsize = strlen(replace_with);

	if (bf->block_end > bf->block_start && bf->block_start > -1) {
		dialog_addline("Replace limited to block");
		startpos = bf->block_start;
		endpos = bf->block_end;
		cursor_moveto(startpos);
	} else {
		endpos = bf->bsize;
	}

	if (replace_mode == 'a')
		return global_replace(startpos, endpos);


	newpos = startpos;
	rcount = 0;

	while (1) {
		/* find target, exit if not found */

		s = find_pattern(&bf->buf[newpos], replace_target, endpos - newpos, 1);
		if (s == NULL)
			break;

		newpos = s - bf->buf;

		if (replace_mode == 'p') {
			cursor_moveto(newpos);
			hilight(newpos, targsize);
			k = v_tolower(ved_getkey(bf->win));
			while (index("rynq", k) == NULL) {
				k = dialog_msg("Found target. Do you wish to Quit,"
				    " Replace or skip to Next (q/r/N):");
			}
			if (k == 'q')
				return;
			if (k == 'n') {
				newpos++;
				continue;
			}
		}
		/* do the replacement */

		if (replsize > targsize) {
			if (memory(replsize - targsize))
				vederror("Cannot expand buffer for replacement");
		}
		replace_string(replace_with, newpos, replsize, targsize);
		endpos -= (targsize - replsize);

		rcount++;


		if (replace_mode == 'p' || replace_mode == 'n') {
			wrap_buffer(bf->wrapwidth, bf->curline);
			bf->dirtyscreen = TRUE;
		}
		if (replace_mode == 'n') {
			cursor_moveto(newpos);
			hilight(newpos, replsize);
			goto EXIT;
		}
		newpos += replsize;
	}

	if (rcount) {
		redisplay();
		dialog_msg("%d changes made!", rcount);
	} else
		dialog_msg("No changes made!");

      EXIT:;
}


/* Entry points for replace() and replace_again().  */

void
replace(void)
{
	u_char *r, *t, *p;
	KEY k;

	if (bf->block_end > bf->block_start && bf->block_start > -1) {
		cursor_moveto(bf->block_start);
		dialog_addline("Replace limited to block");
	}
	dialog_clear();
	p = dialog_entry("Replace original:");
	if (p && *p)
		r = strdup(p);
	else
		return;

	p = dialog_entry("    Replace with:");
	if (p == NULL)
		return;
	t = strdup(p);


	k = dialog_msg("Next, All or Prompted (N/p/a):");

	if (k == KEYBOARD_ABORT)
		return;

	if (k != 'p' && k != 'a')
		k = 'n';	/* if not p/a then default to n  */

	replace_mode = k;

	free(replace_target);
	replace_target = r;

	free(replace_with);
	replace_with = t;

	do_rplagain();
}

void
replace_again(void)
{
	do_rplagain();
}


/* EOF */
