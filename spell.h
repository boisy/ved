/* spell.h */
/* Copyright 1999 Bob van der Poel. See COPYING for details */


#define KEYS1	26
#define KEYS2	27

#define ALTCHAR 0x0e
#define EWORD	0x0f

#define CHAR1 "acdegilnorstuy"
#define CHAR2 "bfhjkmpqvwxz'-"

#define MAXWORDLEN 50

typedef struct {
	int _qcount;
	int _qoffset;
	int _qwidth;
	int _xoffset;
	int _doffset;
	int _dendoffset;
} Smod;
