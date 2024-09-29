/* spell.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "spell.h"
#include "attrs.h"

#define MAXPOS 1000		/* max number of possible replacement words */

enum {
	SPMODE_WORD, SPMODE_BLOCK, SPMODE_BUFFER
};

enum {
	RC_PER_DICT
};

static NAMETABLE spell_option_table[] =
{
	{"Personal_dict", RC_PER_DICT}
};


/* Look up strings for decoding the dictionary.
 * defined in spell.h
 */

static u_char const *chars1 = (u_char *) CHAR1;
static u_char const *chars2 = (u_char *) CHAR2;

/* Default names, defined by rc file */

char *spell_per_dict = NULL;
char *def_new_word_file = "ved_words";

/* Pointer for the in-memory word list loaded
 * from disk and some handy aliases.
 */

static Smod *spell_mod;

static int (*indexes)[KEYS2];	/* pointer to the indexes */
static int quickcount;		/* number of words in quick list */
static int per_count;		/* number of words in personal dict */
static int quicksize;		/* max size of a single word in quick list */
static u_char *quickwords;	/* ptr to first quick words */
static u_char *dictstart;	/* start of dict */
static u_char *dictend;		/* end of dict */


static u_char **per_dict_ptrs;	/* start of pointers to personal wrds */
static u_char *per_dict_data;	/* start of personal word data  */

/*************************************************************
 * This is the tree structure used to save new words. The 'word'
 * field is a single char marker. We allocate enuf memory for the
 * structure and the word to store in it. The word to be added
 * is copied to &wt->word. Note that this is also used to save the words
 * replaced. In this case 'word' contains the wrong and correct versions.
 */

typedef struct wtree {
	struct wtree *left, *right;
	FLAG _tag;
	FLAG _flag;		/* set to 1 if new word, 0 if just okay.. */
	u_char word;
} Wtree;

static Wtree *new_words, *changed_words;

/* Static prototypes */

static Wtree *search_new_word(char *, Wtree *);
static void sp_add_new(char *w, FLAG flag, Wtree ** rootp, int ssize);
static int sp_replace(char *, char *, KEY);


/*****************************************
 * Compare two pointers. Used by qsort
 * in various functions here.
 */

static int
ptr_strcmp(const void *p1, const void *p2)
{
	return strcmp(*(char *const *)p1, *(char *const *)p2);
}

/*********************************************
 * Wrapper for strcmp() for use by bsearch().
 * This is needed since bsearch() expects the compare
 * function to use void* and strcmp() expects
 * char*. This will cast (properly) the args
 */

static int
array_strcmp(const void *p1, const void *p2)
{
	return strcmp(p1, p2);
}

/*****************************************
 * Tree walking routines
 */

static void
descend_left(Wtree ** pres, Wtree ** prev)
{
	Wtree *next;

	while ((next = (*pres)->left) != NULL) {
		(*pres)->left = *prev;
		*prev = *pres;
		*pres = next;
	}
}


static int
descend_right(Wtree ** pres, Wtree ** prev)
{
	Wtree *next;

	if (!(next = (*pres)->right))
		return 0;

	(*pres)->_tag = 1;
	(*pres)->right = *prev;
	*prev = (*pres);
	*pres = next;
	return 1;
}

/*****************************************
 * Look up word(s) in the dictionary.
 * 
 * list - this is a pointer to an array of
 *		 pointers.
 * 
 * numtarg - this is the number of words to check.
 * 
 * NOTE that the list must be sorted.
 * 
 * The function will clear the 1st byte of any targets which are
 * not in the dictionaries. This means that the caller must
 * save the 1st char. if it's needed later...
 * 
 * THE LIST OF WORDS MUST BE ALL LOWERCASE!!!
 * 
 */

#define getnib() ((phase^=1) ? (*dictptr)>>4 : *(dictptr++) & 0xf)

static void
check_dict(u_char ** list, int numtarg)
{
	FLAG phase;
	int t, dictpos;
	u_char c;
	u_char *dictptr;
	u_char *wordptr;
	u_char i1, i2;

	u_char word[MAXWORDLEN + 1];

	/* Determine the index into the main dictionary for the (first)
	 * word. Then set the pointer to the correct pos. in the main
	 * dictionary.
	 */

	i1 = ((i1 = list[0][0]) < 'a' || i1 > 'z') ? 0 : i1 - 'a';
	i2 = ((i2 = list[0][1]) < 'a' || i2 > 'z') ? 0 : i2 - 'a' + 1;

	/* skip over non-indexed */

	while ((dictpos = indexes[i1][i2]) == -1)
		i2++;
	dictptr = dictstart + (dictpos / 2);

	wordptr = word;		/* set 1st 2 chars of word (from index) */
	*wordptr++ = list[0][0];
	if (i2)
		*wordptr++ = list[0][1];
	*wordptr = '\0';	/* this is for 1st compare */

	phase = dictpos & 1;	/* sync to correct file pos */

	/* loop for all the words in the list */

	for (; numtarg--; list++) {

		/* First we see if the word is in the quicklist, personal
		 * dictionary or the new word list..
		 */

		if (bsearch(*list, quickwords, quickcount, quicksize, array_strcmp))
			continue;

		if (per_dict_ptrs && bsearch(list, per_dict_ptrs, per_count,
				   sizeof(per_dict_ptrs[0]), ptr_strcmp))
			continue;

		if (search_new_word(*list, new_words))
			continue;

		/* Check the dictionary... */

		while (1) {

			/* compare the current target to the current dict. word
			 * if target>dict_word then clear the 1st byte of target
			 */

			if ((t = strcmp(word, *list)) >= 0) {
				if (t > 0)
					**list = '\0';
				break;
			}
			while (1) {

				/* if at the end of the dictionary all remaining words
				 * are invalid. Clear the flags and exit
				 */

				if (dictptr >= dictend) {
					while (numtarg--)
						**(list++) = '\0';
					goto EXIT;
				}
				/* get the next char from the dictionary to build
				 * word.
				 */

				if ((c = getnib()) == ALTCHAR)
					*wordptr++ = chars2[getnib()];

				else if (c == EWORD) {
					*wordptr = 0;
					wordptr = word + getnib();
					break;	/* got a word, exit and check it */
				} else
					*wordptr++ = chars1[c];
			}
		}
	}
      EXIT:;
}


/**********************************************************************
 * This routine mungs the target word into possible 'correct'
 * versions. As the munging is done each new possibility is looked
 * up. If it is invalid it is removed from the possibility list.
 * 
 * If any valid possible changes remain, we ask the user if any changes
 * are valid...
 */

#define ISVOWEL(c) (c=='a' || c=='e' || c=='i' || c=='o' || c=='u')

#define ADDWORD(w)  if((pos_count < MAXPOS) && (possibles[pos_count] = \
		strdup(w))) pos_count++

static int
sub_word(char *org)
{
	char w[MAXWORDLEN];
	int t;
	char c, *i, v;
	KEY k;

	/* Array of ptrs for the possible alternate spellings.
	 * Memory for the array is malloc()ed.
	 */

	static u_char **possibles = NULL;
	static int pos_count;	/* number of possible replacement words */


	/* if 1st time we need to get memory for an array of ptrs
	 * to store the possible words. We get enuf for 1000 possibles.
	 * If more than that are generated we just ignore them...
	 */

	if (!possibles && (possibles = (u_char **) malloc(MAXPOS *
					   sizeof(possibles))) == NULL) {
		dialog_msg("No memory for lookup option!");
		return 0;
	}
	/* Zap out any existing word list... */

	for (t = 0; t < pos_count; t++)
		free(possibles[t]);
	pos_count = 0;

	/* Create a list of words based on the original with each
	 * possible combination of two letters transposed.
	 * For example, the word "test" would create "tset" and "tets".
	 */

	strcpy(w, org);
	for (i = w + 1; *(i + 1); i++) {
		c = *i;
		*i = *(i + 1);
		*(i + 1) = c;
		ADDWORD(w);
		*(i + 1) = *i;
		*i = c;
	}

	/* This creates a list of test words based on the target with
	 * assumed trailing letters filled in. The word "test" would
	 * generate "testa", "testb"..."testz" ... "tesat" .. "teszt"...
	 */

	strcpy(w, org);
	for (i = w + strlen(w) + 1, *i-- = '\0'; i > w; i--) {
		for (*i = 'a'; *i <= 'z'; (*i)++)
			ADDWORD(w);
		*i = *(i - 1);
	}

	/* Assume that a letter is wrong in the word. Create list of
	 * words with substitute letters. "Test" becomes "Tast" "tist"
	 * ... "tebt" .. "tezt" .. "tesb"... Note that vowels are
	 * substituted for vowels, constants for constants.
	 */

	strcpy(w, org);
	for (i = w + 1; *i; i++) {
		c = *i;
		v = ISVOWEL(c);
		for (*i = 'a'; *i <= 'z'; (*i)++)
			if (v == ISVOWEL(*i))
				ADDWORD(w);
		*i = c;
	}

	/* Assume that the misspelled word has an extra letter in it.
	 * Create a list of words with letters missing. "Test" becomes
	 * "tes" "tet" and "tst".
	 */

	strcpy(w, org);
	c = '\0';
	for (i = w + strlen(w) - 1; i > w; i--) {
		v = *i;
		*i = c;
		c = v;
		ADDWORD(w);
	}

	/* sort the possible replacements... */

	if (pos_count > 1)
		qsort(possibles, pos_count, sizeof(possibles[0]), ptr_strcmp);

	check_dict(possibles, pos_count);	/* zap out any bad words... */

	/* Check the list to see if any replacements have been found...
	 * if not, "sorry" and exit
	 */

	for (t = 0, c = 0; t < pos_count; t++) {
		if (possibles[t][0]) {
			c = 1;
			break;
		}
	}

	if (c == 0) {
		dialog_msg("No corrections found...sorry!");
		return 0;
	}
	/* go though replacements...and find correct one */

	for (t = 0; t < pos_count; t++) {
		if (possibles[t][0]) {
			k = dialog_msg("Change word to '%s' (yes/No/all):", possibles[t]);
			if (k == KEYBOARD_ABORT)	/* just like subing */
				return -1;

			if (k == 'y' || k == 'a')
				return sp_replace(org, possibles[t], k);
		}
	}
	return 0;
}

#undef ISVOWEL
#undef ADDWORD

/****************************************
 * Replace the word at the cursor with a new one.
 * RETURN: Longer of old/new word (used to skip forward)
 */

static int
sp_replace(char *orig, char *new, KEY mode)
{
	int newsize = strlen(new);
	int oldsize = strlen(orig);
	Wtree *newnode;
	char buff[200];

	replace_string(new, bf->curpos, newsize, oldsize);

	cursor_change_refresh();

	/* Now add the original and new word to the changed word list.
	 * This list is checked when mistakes are found and the user
	 * if first asked if he wants to make the same change...
	 */

	strcpy(buff, orig);
	lowstr(buff);
	strcpy(buff + oldsize + 1, new);
	lowstr(buff + oldsize + 1);

	if (search_new_word(buff, changed_words) == NULL) {
		sp_add_new(buff, 0, &changed_words, newsize + oldsize + 2);
		if ((newnode = search_new_word(buff, changed_words)) != NULL)
			newnode->_flag = mode;
	}
	return (newsize > oldsize) ? newsize : oldsize;
}





/*******************
 * FILE FUNCTIONS
 *******************/

/*****************************************
 * Load the dictionary and index into memory.
 * If not found, jump to the error handler...
 */

static void
load_dict(void)
{
	FILE *fp = NULL;
	int fsize, t;
	char *p, *pend;
	static char *nomem = "No memory for personal dictionary!";

	static char *dirs[] =
	{"~/", "~/.", "", "/usr/local/etc/", "/usr/etc/", NULL};

	for (t = 0; dirs[t]; t++) {
		asprintf(&p, "%s%s", dirs[t], "ved_dict.mod");
		if (p)
			fp = fopen(p, "r");
		free(p);
		if (fp != NULL)
			break;
	}
	if (fp == NULL)
		vederror("Can't access the spelling dictionary");

	fsize = get_filesize(fp);

	if ((spell_mod = (Smod *) malloc(fsize)) == NULL) {
		fclose(fp);
		vederror("No core for main dictionary");
	}
	t = fread(spell_mod, fsize, 1, fp);
	fclose(fp);
	if (t != 1) {
		free(spell_mod);
		vederror("Serious error reading dictionary into core");
	}
	/* set ptrs to start/end of dictionary from data module info */

	p = (u_char *) spell_mod;
	indexes = (int (*)[KEYS2])(p + spell_mod->_xoffset);
	dictstart = p + spell_mod->_doffset;
	dictend = p + spell_mod->_dendoffset;
	quickwords = p + spell_mod->_qoffset;
	quickcount = spell_mod->_qcount;
	quicksize = spell_mod->_qwidth;

	/* Load the users personel dictionary. Most errors
	 * at this point are silent and ignored
	 */

	if (spell_per_dict) {
		if ((fp = fopen(tilde_fname(spell_per_dict), "r")) == NULL)
			return (void)dialog_msg("WARNING, could not open personal"
				   " dictionary %s, %s!", spell_per_dict,
						strerror(errno));

		/* Get size of the personal file. If the file is empty, exit */

		if ((fsize = get_filesize(fp)) == 0) {
			fclose(fp);
			return;
		}
		/* get memory for file */

		if ((per_dict_data = (u_char *) malloc(fsize)) == NULL) {
			dialog_msg(nomem);
			fclose(fp);
			return;
		}
		fread(per_dict_data, fsize, 1, fp);
		fclose(fp);

		/* Translate file into 0 terminated words and count them */

		for (per_count = 0, p = per_dict_data, pend = p + fsize; p < pend;) {
			if (*p++ == '\n') {
				*(p - 1) = '\0';
				per_count++;
			}
		}

		/* Check that there are words in file */

		if (per_count == 0) {
			free(per_dict_data);
			per_dict_data = NULL;
			return;
		}
		/* Create array of pointers for the personal words */

		if ((per_dict_ptrs = (u_char **) malloc(per_count *
				       sizeof(per_dict_ptrs))) == NULL) {
			free(per_dict_data);
			per_dict_data = NULL;
			dialog_msg(nomem);
			return;
		}
		for (t = 0, p = per_dict_data; t < per_count; t++) {
			per_dict_ptrs[t] = p;
			p += strlen(p) + 1;
		}
		qsort(per_dict_ptrs, per_count, sizeof(per_dict_ptrs[0]), ptr_strcmp);
	}
}

/*****************************************
 * Write the new words into a file.
 */


void
speller_save_new_words(void)
{
	FILE *fp;
	Wtree *next;
	Wtree *prev, *pres;
	char *p;

	if (new_words == NULL)
		vederror("No new words to save");

	dialog_clear();
	p = dialog_entry("Save new words%s%s\nEnter filename:",
			 (def_new_word_file ? "; default: " : ""),
			 (def_new_word_file ? def_new_word_file : ""));

	if (p == NULL)
		return;

	if (*p == '\0') {
		if (def_new_word_file)
			p = def_new_word_file;
		else
			return;
	}
	if ((fp = open_outfile(p, PROMPT_APPEND)) == NULL)
		vederror(NULL);	/* open_outfile() reports error */

	/* This is a non-recursive tree traversal. For details see
	 * C Chest p. 190
	 */

	prev = NULL;
	pres = new_words;

	do {
		descend_left(&pres, &prev);
		if (pres->_flag)
			fprintf(fp, "%s\n", &pres->word);

	} while (descend_right(&pres, &prev));

	while (prev) {
		if (prev->_tag == 0) {
			next = prev->left;
			prev->left = pres;
			pres = prev;
			prev = next;

			while (1) {
				if (pres->_flag)
					fprintf(fp, "%s\n", &pres->word);

				if (!descend_right(&pres, &prev))
					break;
				descend_left(&pres, &prev);
			}
		} else {
			next = prev->right;
			prev->_tag = 0;
			prev->right = pres;
			pres = prev;
			prev = next;
		}
	}
	fclose(fp);
}


/*****************************************
 * Add a new word to the word list.
 * 
 * This is a non-recursive tree routine. For source see:
 * C Chest & Other Treasures, p 186.
 * 
 * If the word to be added already exists in the tree it is
 * just ignored. Nothing is returned. If no room for the word
 * a 'stack overflow' error is generated--this exits the speller.
 */

static void
sp_add_new(char *w, FLAG flag, Wtree ** rootp, int ssize)
{
	Wtree *root = *rootp;
	Wtree **insert_here = rootp;
	int rel;

	while (root) {
		if ((rel = strcmp(w, &root->word)) == 0)
			return;	/* duplicate, ignore */

		insert_here = (rel < 0) ? &root->left : &root->right;
		root = *insert_here;
	}
	if ((*insert_here = root = (Wtree *) malloc(sizeof(Wtree) +
						    ssize - 1)) != NULL) {
		root->right = root->left = NULL;
		root->_tag = 0;
		root->_flag = flag;
		memmove(&root->word, w, ssize);
	} else
		vederror("No stack to add save new word");
}

/*****************************************
 * Search the word list for a word. Return
 * NULL if word not in the
 * list; a ptr to the word if found.
 */

static Wtree *
search_new_word(char *w, Wtree * root)
{
	int comp;

	if (root) {
		while ((comp = strcmp(w, &root->word)) != 0) {
			if (comp < 0)
				root = root->left;
			else
				root = root->right;
			if (root == NULL)
				break;
		}
	}
	return root;		/* return NULL or ptr to package */
}

/*****************************************
 * Suck the word at the cursor into a static
 * buffer and return a pointer to the word.
 * We convert the word to lowercase and
 * stop at the end according to spelling rules.
 *
 * The current buffer pointer MUST be at a alpha character!!!!!
 */

static u_char *
getword(u_char * pos, u_char * end)
{
	u_char c, *p;
	static u_char wbuff[MAXWORDLEN + 3];

	/* copy word from text buffer into word buffer */

	for (p = wbuff;;) {
		if (pos >= end)
			break;
		c = *pos++;

		/* If char not-alpha we skip out. Exceptions are a
		 * '-' followed by a alpha (hyphenated word) or a
		 * ' followed by alpha (possesive or contraction).
		 * At this point pos points to the next char...
		 */

		if ((v_isalpha(c) == FALSE) &&
		    (((c != '-') && (c != '\'')) ||
		     (v_isalpha(*pos) == FALSE)))
			break;

		*p++ = v_tolower(c);

		if (p >= &wbuff[MAXWORDLEN - 2])
			break;
	}

	/* terminate and point p to end of word  */

	*p-- = '\0';

	/* check for possesive. If word ends in 's then strip that off */

	if (p > wbuff + 1 && *p == 's' && *(p - 1) == '\'')
		*(p - 1) = '\0';
	else if (p > wbuff && *p == '\'')
		*p = '\0';

	return wbuff;
}


/* Check the word at the current cursor pos
 * RETURN: -1 quit
 *        0   no word found
 *        nn  size of word found or replaced or verified ok
 */

static int
sp_main_one(void)
{
	u_char c, *target;
	int sz, t, retvalue;
	u_char buff[160], *wp, *p;
	Wtree *wordnode;
	u_char *list[2];

	/* get the word at the cursor */

	target = getword(&bf->buf[bf->curpos], &bf->buf[bf->bsize]);

	if (*target == '\0')
		return 0;

	sz = strlen(target);

	c = target[0];
	list[0] = target;
	check_dict(list, 1);	/* check the dicts */

	if (target[0] != '\0')
		return sz;

	bf->hilite_start = bf->curpos;
	bf->hilite_end = bf->curpos + sz - 1;
	redisplay();
	doupdate();

	target[0] = c;

	/* If this word has already been changed we ask if
	 * the user wants to do the same replace again
	 */

	if ((wordnode = search_new_word(target, changed_words)) != NULL) {
		wp = (u_char *) strend(&wordnode->word) + 1;
		if ((c = wordnode->_flag) != 'a') {
			c = wordnode->_flag =
			    dialog_msg("Change '%s' to '%s' (all/yes/No):",
				       target, wp);
		}
		if (c == 'a' || c == 'y') {
			retvalue = sp_replace(target, wp, 0);
			goto EXIT;
		}
	}
      CLOOP:
	switch (dialog_msg("Can't find '%s' in dictionary"
			   " (L,A,I,O,R,Q):", target)) {

			/* lookup */

		case 'l':
			if ((t = sub_word(target)) > 0) {
				retvalue = t;
				goto EXIT;
			}
			goto CLOOP;

			/* add to wordlist */

		case 'a':
			sp_add_new(target, 1, &new_words,
				   strlen(target) + 1);
			retvalue = sz;
			goto EXIT;

		case 'o':
			sp_add_new(target, 0, &new_words,
				   sz + 1);
			retvalue = sz;
			goto EXIT;

			/* ignore (skip to next) */

		case 'i':
			retvalue = sz;
			goto EXIT;

		case 'r':	/* retype word and replace */
			p = dialog_entry("New spelling:");
			if (p == NULL || *p == 0)
				goto CLOOP;

			strcpy(buff, p);
			lowstr(buff);
			list[0] = buff;
			c = buff[0];
			check_dict(list, 1);
			if (buff[0] == '\0') {
				buff[0] = c;
				if (dialog_msg("Can't find '%s' in dictionary,"
					       " okay to replace (Yes/no):", buff) == 'n')
					goto CLOOP;
				sp_add_new(buff, 1, &new_words, strlen(buff) + 1);
			}
			c = opt_casematch;
			opt_casematch = TRUE;
			if (c != TRUE)
				set_findmatch_table();
			t = sp_replace(target, buff, 0);
			opt_casematch = c;
			if (c != TRUE)
				set_findmatch_table();

			retvalue = t;
			goto EXIT;

			/* quit this nonsense, return to editor */

		case KEYBOARD_ABORT:
		case 'q':
			retvalue = -1;
			goto EXIT;

		default:
			dialog_addline("Lookup correction, Add to wordlist, "
				   "word is Okay, Ignore, Retype, Quit");
			goto CLOOP;
	}
      EXIT:

	bf->hilite_start = -1;
	bf->hilite_end = -1;
	redisplay();

	return retvalue;
}

/*****************************************
 * Check the buffer from the start of the
 * current word to the position passed. This
 * will be the end of the block, buffer or
 * word.
 */


static void
sp_buffer(int mode)
{
	int sz;

	/* First time, get data. If we can't load the dict
	 * we error out.
	 */

	if (!dictstart)
		load_dict();

	dialog_clear();

	cursor_startword();

	while (1) {
		while ((v_isalpha(bf->buf[bf->curpos]) == FALSE) &&
		       (bf->curpos < bf->bsize - 1))
			cursor_right();


		if (bf->curpos >= bf->bsize - 2)
			break;

		sz = sp_main_one();

		/* exit if sp_main_one() aborted */

		if (sz == -1)
			break;

		if (sz == 0)
			sz++;

		/* Skip possesive endings on words ????????? */

		if (bf->buf[bf->curpos + sz] == '-' &&
		    v_tolower(bf->buf[bf->curpos + sz + 1]) == 's') {
			sz += 2;
		}
		cursor_moveto(bf->curpos + sz);

		switch (mode) {
			case SPMODE_WORD:
				return;

			case SPMODE_BLOCK:
				if (bf->curpos >= bf->block_end)
					return;

			default:
				break;
		}
	}
}




/* check the current word...move to start of word */

void
speller_file(void)
{
	sp_buffer(SPMODE_BUFFER);
}

void
speller_word(void)
{
	sp_buffer(SPMODE_WORD);
}

void
speller_block(void)
{
	if ((bf->block_start != -1) && (bf->block_end != -1)) {
		cursor_moveto(bf->block_start);
		sp_buffer(SPMODE_BLOCK);
	}
}

char *
spell_set(char *s)
{
	char *s1;

	s1 = strsep(&s, " \t");

	/* NOTE: The arg (s) is everything after
	 * the option name (minus Spaces). This
	 * permits args like "%X %a"
	 */


	if (s1 == NULL)
		return NULL;

	switch (lookup_value(s1, spell_option_table)) {
		case RC_PER_DICT:
			free(spell_per_dict);
			spell_per_dict = NULL;
			s = translate_delim_string(s);
			if ((s != NULL) && (*s != '\0'))
				spell_per_dict = strdup(s);
			return NULL;

		default:
			return "UNKNOWN OPTION";
	}
}


void
rc_spell_options(FILE * f, char *name)
{
#define L(x) lookup_name(x, spell_option_table)

	fprintf(f, "# Spell check options\n\n");

	fprintf(f, "%s\t%s\t%s\n", name, L(RC_PER_DICT),
	make_delim_string(spell_per_dict == NULL ? "" : spell_per_dict));

	putc('\n', f);

#undef L
}

/* EOF */
