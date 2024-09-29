/* tilde.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include <pwd.h>


/*********************************************************************
 * This function is called by most (all?) routines which open files.
 * It does tilde substitution on the first ~ in the filename and returns
 * a ptr to a static buffer. If you need to reuse the name, you must copy it.
 *
 * The buffer (fullname) is dynamically allocated by the
 * asprintf() functions. Since it declared static, it sticks
 * around after function finished. We start out each call by
 * free()ing the previous storage. It's very important that
 * the storage is initialized to NULL!
 */

char *
tilde_fname(char *name)
{
	static char *fullname = NULL;

	if( name == NULL || *name == '\0')
		return name;
	
	free(fullname);
	fullname = NULL;

	/* if the name is "~/..." or just "~" we
	 * try to convert the ~ to the home directory of
	 * the user. This is done by examining the HOME
	 * variable or, failing that, looking up the dir
	 * in the password file */

	if (name[0] == '~' && (name[1] == '/' || name[1] == '\0')) {

		char *home;

		home = getenv("HOME");
		if (home == NULL) {
			struct passwd *pw;
			pw = getpwuid(getuid());
			if (pw)
				home = pw->pw_dir;
		}
		if (home != NULL) {
			asprintf(&fullname, "%s%s", home, name + 1);
			name = fullname;
		}
		goto EXIT;
	}
	/* if the name is "~xxx" or "~xxx/..." owe try for
	 * the home directory of the user xxx. */

	if (name[0] == '~') {

		char *tmp;
		char *p;
		char *home;
		struct passwd *pw;

		if ((tmp = strdup(name + 1)) != NULL) {
			p = index(tmp, '/');
			if (p)
				*p = '\0';
			pw = getpwnam(tmp);
			free(tmp);

			if (pw) {
				home = pw->pw_dir;
				if ((p = index(name, '/')) != NULL)
					asprintf(&fullname, "%s%s", home, p);
				else
					asprintf(&fullname, "%s", home);
				name = fullname;
			}
		}
	}
	/* seems that we didn't have a tilde expansion to do...
	 * return the original name the caller passed to us. */

      EXIT:
	return name;
}

/* EOF */
