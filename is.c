/* is.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

/*********************************************************************
 * Character conversion routines. These are necessary since we're using
 * unsigned char and int (which the ctype.h stuff may or may not support.
 */

int
v_toupper(int c)
{
	return (c >= 'a' && c <= 'z') ? c - 0x20 : c;
}

int
v_tolower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + 0x20 : c;
}


int
v_isalpha(int c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int
v_isdigit(int c)
{
	return (c >= '0' && c <= '9');
}

int
v_isxdigit(int c)
{
	return (v_isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'Z'));
}

int
v_isalnum(int c)
{
	return (v_isalpha(c) || v_isdigit(c));
}

int
v_isblank(int c)
{
	return (c == SPACE || c == TAB);
}

int
v_islower(int c)
{
	return (c >= 'a' && c <= 'z');
}

int
v_isupper(int c)
{
	return (c >= 'A' && c <= 'Z');
}

/*********************************************************************
 * Test to see if the named file is a directory. Note that calls to this function
 * should have fixed the filename for tilde expansion, etc.
 * 
 * returns 0 - not a directory
 * 1 - accessable directory
 * -1 - not accessable directory
 */

int
isdir(u_char * f)
{
	struct stat sbuff;

	if (stat(f, &sbuff))
		return 0;	/* if fstat returns -1 it is really an error... */
	/* but returning 0 to the caller serves */

	return S_ISDIR(sbuff.st_mode);
}

/*********************************************************************
 * Return true/false from a string
 * 
 * return 1 -  If the text at the pointer is a numeric value >0 or
 * the text "true" or "yes" is at the pointer
 */

int
istrue(char *s)
{
	int result, err;
	
	if(s==NULL)
		return 0;

	get_exp(&result, s, &err);
	if (err == 0)
		return (result ? 1 : 0);

	if (strcasecmp(s, "true") == 0)
		return 1;

	if (strcasecmp(s, "yes") == 0)
		return 1;

	return 0;
}

/* EOF */
