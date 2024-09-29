/* block.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

/**************************************
 * Verify that the block is properly
 * marked.
 */

static void
block_ok(void)
{
	dialog_clear();
	if (bf->block_start == -1 || bf->block_start == -1)
		vederror("No block defined");

	if (bf->block_start > bf->block_end)
		vederror("Internal block marker order error");
}

/********************************************
 * Block shortcut. This brings up the block menu
 * avoiding the top level menu call.
 */

void
block_cmd(void)
{
	menu_line_do(bf->mw, get_edit_menu_ptr(), 'b');
}

/**************************************************
 * Unmark block
 */

void
block_delmarks(void)
{
	bf->block_start = -1;
	bf->block_end = -1;
	redisplay();
}

/*********************************************
 * Mark a block.
 */


void
mark_block_start(void)
{
	sync_x();

	bf->block_start = bf->curpos;
	if (bf->block_end < bf->block_start)
		bf->block_end = bf->curpos;

	redisplay();
}

void
mark_block_end(void)
{

	sync_x();

	bf->block_end = bf->curpos;
	if ((bf->block_start == -1) || (bf->block_start > bf->block_end))
		bf->block_start = bf->curpos;

	redisplay();
}

/****************************************************
 * Copy a marked block to the current cursor pos
 */

void
block_copy(void)
{
	int size, pos;

	block_ok();

	sync_x();

	pos = bf->curpos;
	if (pos >= bf->block_start && pos <= bf->block_end)
		vederror("Can't copy block over itself");

	size = (bf->block_end - bf->block_start) + 1;

	if (memory(size))
		vederror("No memory to copy %d byte block", size);

	openbuff(size, pos);
	memmove(&bf->buf[pos], &bf->buf[bf->block_start], size);
	cursor_change_refresh();
}

/************************
 * Delete a marked block 
 */


void
block_delete(void)
{
	int start, end;

	block_ok();

	start = bf->block_start;
	end = bf->block_end;

	bf->block_start = -1;
	bf->block_end = -1;
	cursor_moveto(start);
	del_save(start, end);
}

/****************************************************
 * Move a marked block to the current cursor pos
 */

void
block_move(void)
{
	int size, pos, t;
	FILE *fp;

	block_ok();
	sync_x();

	pos = bf->curpos;
	if (pos >= bf->block_start && pos <= bf->block_end)
		vederror("Can't move block into itself");

	size = (bf->block_end - bf->block_start) + 1;

	/* The move is done by
	 *  1. Creating a file buffer for the data 
	 *  2. delete the old data
	 *  3. insert the buffered data
	 */

	if ((fp = tmpfile()) == NULL)
		vederror("Can't create scratch file, %s", strerror(errno));

	if (fwrite(&bf->buf[bf->block_start], size, 1, fp) != 1) {
		fclose(fp);
		vederror("Error writing data to scratch buffer, %s", strerror(errno));
	}
	closebuff(size, bf->block_start);

	if (memory(size)) {
		fclose(fp);
		vederror("No memory to copy %d byte block, DATA LOST!", size);
	}
	openbuff(size, bf->curpos);

	rewind(fp);
	t = fread(&bf->buf[bf->curpos], size, 1, fp);
	fclose(fp);

	if (t != 1)
		dialog_msg("Warning: error in reading copy data!");

	cursor_change_refresh();
}


/*******************************
 * Save a marked block to disk
 */

void
block_save(void)
{
	FILE *fp;
	char *f;
	int t;

	block_ok();

	f = dialog_entry("Block save:");
	if ((f == NULL) || (*f == '\0'))
		vederror(NULL);
	fp = open_outfile(f, PROMPT_APPEND);
	if (fp == NULL)
		vederror(NULL);
	t = fwrite(&bf->buf[bf->block_start], (bf->block_end - bf->block_start) + 1,
		   sizeof(u_char), fp);
	fclose(fp);
	if (t != 1)
		dialog_msg("WARNING: Error writing block, %s!", strerror(errno));

}


/*****************************************
 * Sort block
 */

static int
ptr_strcmp(const void *p1, const void *p2)
{
	return strcmp(*(char *const *)p1, *(char *const *)p2);
}

static int
ptr_strcasecmp(const void *p1, const void *p2)
{
	return strcasecmp(*(char *const *)p1, *(char *const *)p2);
}

void
block_sort(void)
{
	FILE *fp;
	KEY ignore_case, k;
	u_char **array, *p, *bstart;
	int count, t, err, blocksize;

	block_ok();

	cursor_moveto(bf->block_start);

	ignore_case = dialog_msg("Sorting block, sure (yes/No/ignore_case):");
	if (ignore_case != 'y' && ignore_case != 'i')
		vederror(NULL);

	if (bf->buf[bf->block_end] != EOL) {
		t = bf->block_end;
		count = 0;
		while (bf->buf[t] != EOL) {
			count++;
			t++;
		}
		k = dialog_msg("Block end must be on a EOL. There "
		      "is one %d chars right. Okay to move there (y/N):",
			       count);
		if (k != 'y')
			vederror(NULL);
		bf->block_end = t;
	}
	blocksize = (bf->block_end - bf->block_start) + 1;
	bstart = &bf->buf[bf->block_start];

	for (count = 0, t = blocksize, p = bstart; t--;)
		if (*p++ == EOL)
			count++;


	/* create scratch file */

	if ((fp = tmpfile()) == NULL)
		vederror("Can't create scratch file, %s", strerror(errno));

	/* Get memory for an array of pointers. "no stack memory"
	 * error if malloc() fails.
	 */

	if ((array = (u_char **) malloc(count * sizeof(array))) == NULL) {
		fclose(fp);
		vederror("Out of stack memory");
	}
	/* Convert EOLs to NULLs */

	for (p = bstart, t = blocksize; t--;)
		if (*p++ == EOL)
			*(p - 1) = '\0';

	/* Create ptrs to strings */

	for (t = 0, p = bstart; t < count;) {
		array[t++] = p;
		p = (u_char *) strend(p) + 1;
	}

	qsort(array, count, sizeof(array[0]),
	      ignore_case == 'i' ? ptr_strcasecmp : ptr_strcmp);

	/* convert 0 terminators back to EOLs */

	for (t = 0, err = 0; t < count; t++)	// save sorted array in scratch file
	 {
		if (fprintf(fp, "%s\n", array[t]) < 0) {
			err++;
			break;
		}
	}

	if (err == 0) {
		rewind(fp);	// load sorted data

		if (fread(bstart, blocksize, 1, fp) != 1)
			err++;
	}
	fclose(fp);
	free(array);

	if (err != 0) {
		for (p = bstart, t = blocksize; t--;)
			if (*p++ == '\0')
				*(p - 1) = EOL;
	}
	cursor_change_refresh();

	if (err != 0)
		dialog_msg("*** Caution *** read/write error in block transfers!");

	bf->lastchange = bf->curpos;
	bf->changed = TRUE;
}

/****************************************************
 * Unformat block
 */

void
block_unformat(void)
{
	int t;
	u_char *p;

	block_ok();

	cursor_moveto(bf->block_start);

	if (dialog_msg("Unformatting block, sure (yes/No):") != 'y')
		vederror(NULL);

	for (t = bf->block_start, p = bf->buf; t <= bf->block_end; t++) {
		if ((p[t] == EOL) && (v_isalpha(p[t + 1]) == TRUE))
			p[t] = ' ';
	}
	bf->block_start = -1;
	bf->block_end = -1;
	cursor_change_refresh();
}

/****************************************************
 * Double a block.
 *   foo
 *   woof
 *   bar
 *
 * becomes:
 *
 *   foo foo
 *   woof woof
 *   bar bar
 */

void
block_double(void)
{
	int blocksize;
	KEY k;
	u_char delim, *p;
	int c;
	FILE *fp;
	long fpos;

	block_ok();

	if (bf->buf[bf->block_end] != EOL)
		vederror("Block must end in an EOL for duplicate function");

	cursor_moveto(bf->block_start);
	blocksize = bf->block_end - bf->block_start + 1;


	delim=' ';
	
	k = dialog_msg("Doubling block, sure (yes/No/tabseparate):");

	if(k == 't')
		delim = TAB;
	else if(k != 'y')
		vederror(NULL);
	

	if (memory(blocksize))
		vederror("No room to exand buffer");

	/* create scratch file */

	if ((fp = tmpfile()) == NULL)
		vederror("Can't create scratch file, %s", strerror(errno));

	if (fwrite(&bf->buf[bf->curpos], blocksize, 1, fp) != 1) {
		fclose(fp);
		vederror("Error writing scratch file, %s", strerror(errno));
	}
	rewind(fp);
	openbuff(blocksize, bf->curpos);

	for (blocksize *= 2, p = &bf->buf[bf->block_start]; blocksize > 0;) {
		fpos = ftell(fp);
		while (1) {
			c = fgetc(fp);
			if (c == EOF)
				c = EOL;
			*p++ = c;
			blocksize--;
			if (c == EOL) {
				*(p - 1) = delim;
				break;
			}
		}
		fseek(fp, fpos, SEEK_SET);
		while (1) {
			c = fgetc(fp);
			if (c == EOF)
				break;
			*p++ = c;
			blocksize--;
			if (c == EOL)
				break;
		}
	}

	fclose(fp);
	cursor_change_refresh();

	bf->lastchange = bf->curpos;
	bf->changed = TRUE;
}

void
block_start(void)
{
	if ((bf->block_start >= 0) && (bf->block_start < bf->bsize))
		cursor_moveto(bf->block_start);
}

void
block_end(void)
{
	if ((bf->block_end >= 0) && (bf->block_end < bf->bsize))
		cursor_moveto(bf->block_end);
}

void
block_to_buffer(void)
{
	int bsize;
	u_char *p;

	block_ok();

	bsize = bf->block_end - bf->block_start;
	p = &bf->buf[bf->block_start];

	windows_new();

	openbuff(bsize, bf->curpos);
	memcpy(&bf->buf[bf->curpos], p, bsize);

	cursor_change_refresh();
}

void
block_wordwrap(void)
{
	u_char *p;
	int t, width, err, startln, pos;
	
	block_ok();
	
	
	cursor_moveto(bf->block_start);
	
	dialog_clear();
	dialog_addline("This command will modify the block with real EOLs");	
	p=dialog_entry("Enter width: ");
	if(p==NULL || *p=='\0')
		return;
	
	get_exp(&width, p, &err);
	if(err !=0 ) 
		vederror("Parse error");
	if(width<10 || width>200)
		vederror("Width must be 10..200");
	
	wrap_buffer(width, 0);
	
	startln=get_line_number(bf->block_start);
	
	for(t=startln; ; ) {
		pos=bf->linestarts[t+1]-1;		
		if(bf->block_end < pos )
			break;
		if(bf->buf[pos]==' ') {
			bf->buf[pos]=EOL;
			bf->changed=TRUE;
			bf->lastchange=pos;
		}
		t++;
	}
	
	wrap_buffer(bf->wrapwidth, 0);

	cursor_change_refresh();
}

/* EOF */
