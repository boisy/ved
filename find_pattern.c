/* find_pattern.c  */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

static
u_char mtable[] =
{
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
  39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
  57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
  75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92,
 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183,
184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198,
199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228,
229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243,
	244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

/****************************************************
 * Set the case-translation table. This is done
 * on init and when the case-flag is toggled.
 */

void
set_findmatch_table(void)
{
	u_char a, b;

	/* do case translation */

	for (a = 'A', b = (opt_casematch == 0) ? 'a' : 'A'; a <= 'Z';)
		mtable[a++] = b++;
}

/*********************************************************************
 * Search text buffer for a target.
 * 
 * pos - char pointer to start of buffer to check
 * targ - a 0 terminated string???
 * count - size of memory to check in bytes
 * direction - forward/back search (-1==back, 1==fwd)
 * 
 * Returns NULL - no match found
 * 0... - location in buffer
 */

u_char *
find_pattern(u_char * pos, u_char * targ, int count, int direction)
{
	u_char *p, *p1;
	u_char *t1, a;
	u_char target[200];

	if (count <= 0)
		return NULL;

	strcpy(target, targ);
	if (opt_casematch == 0)
		lowstr(target);

	/* set tab to match space if no tabs in search string  */

	if (index(target, TAB) == NULL)
		mtable[TAB] = SPACE;
	else
		mtable[TAB] = TAB;

	for (p = pos;; p += direction) {	/* check each position for target */
		for (p1 = p, t1 = target;;) {
			if (!(a = *t1++))	/* a==0, reached end of string, ==MATCH  */
				return p;	/* end of target, match found  */

			if (a != mtable[*p1++] && a != wildcard)
				break;
		}
		if (!(--count))
			return NULL;	/* end of buffer, return 'not found'  */
	}
}

/* EOF */
