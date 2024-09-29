/* misc.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"


/*********************************************************************
 * Calculate the number of columns to the next tab stop. This is determined
 * by comparing the passed value with the tab field size.
 */

int
tabplus(int x)
{
	return opt_tabsize - (x % opt_tabsize);
}

/*********************************************************************
 * Convert strings to lower case
 */

void
lowstr(u_char * s)
{
	while (*s) {
		*s = v_tolower(*s);
		s++;
	}
}


/*********************************************************************
 * Return pointer to end of string
 */

u_char *
strend(u_char * s)
{
	while (*s++) ;
	return --s;
}

/********************************************
* Advance pointer past any leading whitespace
*/

u_char *
skip_white(u_char * s)
{
	if (s != NULL) {
		while (*s == ' ' || *s == '\t')
			s++;
	}
	return s;
}


/*********************************************************************
 * Return a ptr to a text string "true" or "false".
 */

char *
truefalse(int v)
{
	return v == TRUE ? "TRUE" : "FALSE";
}


/***************************************************
 * Locate the requested area
 * RETURN: index value
 */

int
lookup_value(char *s, NAMETABLE * tbl)
{
	char *p;
	int len;

	if (s == NULL || *s == 0)
		return -1;

	for (len = 0, p = s; *p > ' '; p++, len++) ;	/* length of first word in string */

	while (tbl->name) {
		if (strncasecmp(s, tbl->name, len) == 0)
			return tbl->value;
		tbl++;
	}
	return -1;
}

char *
lookup_name(int v, NAMETABLE * tbl)
{
	while (tbl->name) {
		if (tbl->value == v)
			return tbl->name;
		tbl++;
	}
	return NULL;
}

/**********************************************************
 * Create a  a string with delimiters
 * Returns a ptr to the string (static storage).
 * If error a pointer to a empty string (NOT NULL) is returned
 */

u_char *
make_delim_string(u_char * s)
{
	int delim;
	static char *buffer = NULL;

	free(buffer);
	buffer = NULL;

	if (s == NULL) {
		s = "";
	}
	for (delim = '\"'; delim < 0x80; delim++) {
		if (index(s, delim) == NULL) {
			asprintf(&buffer, "%c%s%c", delim, s, delim);
			break;
		}
	}

	return buffer;
}

/**************************************************
 * Convert a delimited string to non-delim. This
 * does change the string by converting the trailing
 * delimiter to a '0'.
 * The start of the string is returned.
 * IF THE STRING IS EMPTY or if there are NO DELIMITERS
 * a NULL ptr is returned!
 */

u_char *
translate_delim_string(u_char * s)
{
	int delim;
	u_char *p;

	if (s == NULL)
		return NULL;

	delim = *s++;
	
	if(delim == '\0')
		return NULL;
	
	p = s;
	while (*s != delim && *s != '\0')
		s++;
	*s = '\0';

	if(p == s)
		return NULL;
			
	return p;
}

/* EOF */
