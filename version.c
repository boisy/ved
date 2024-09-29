/* version.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"

static char *revision =
#include "version.h"
;


void
about_ved(void)
{
	dialog_clear();

	dialog_addline("Ved (C) 1999-2004, Bob van der Poel");
	dialog_addline("Compiled %s %s", __DATE__, __TIME__);
	dialog_msg("Revision %s.", revision);
}
