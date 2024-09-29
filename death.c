/* death.c  */
/* Copyright 1999 Bob van der Poel. See COPYING for details */


#include "ved.h"
#include <signal.h>

static char *deaddir = NULL;
static char sendadvise = TRUE;
static char *mailer = NULL;
static char dead_save = TRUE;


void deathtrap(int sig);

enum {
	RC_SAVE, RC_DIR, RC_ADVISE, RC_MAILER
};

static NAMETABLE rc_dead[] =
{
	{"Save", RC_SAVE},
	{"Dir", RC_DIR},
	{"Advise", RC_ADVISE},
	{"Mailer", RC_MAILER},
	{}
};

/*********************************************************************
 * Create signal traps to save buffer on unexpected termination!
 * This is stolen in part from the Elvis code
 */

void
death_init(void)
{
#ifndef DEBUG

#ifdef SIGABRT
	signal(SIGABRT, deathtrap);
#endif
#ifdef SIGTERM
	signal(SIGTERM, deathtrap);
#endif
#ifdef SIGHUP
	signal(SIGHUP, deathtrap);
#endif
#ifndef DEBUG
#ifdef SIGILL
	signal(SIGILL, deathtrap);
#endif
#ifdef SIGBUS
	signal(SIGBUS, deathtrap);
#endif
#ifdef SIGSEGV
	signal(SIGSEGV, deathtrap);
#endif
#ifdef SIGSYS
	signal(SIGSYS, deathtrap);
#endif
#endif				/* !DEBUG */
#ifdef SIGPIPE
	signal(SIGPIPE, deathtrap);
#endif
#ifdef SIGTERM
	signal(SIGTERM, deathtrap);
#endif
#ifdef SIGUSR1
	signal(SIGUSR1, deathtrap);
#endif
#ifdef SIGUSR2
	signal(SIGUSR2, deathtrap);
#endif
#endif

}

/*********************************************************************
 * rc.c calls this to process the dead command.
 * 
 * Options: dead dir 'dead file directory'
 */

char *
death_set_rc(char *s)
{
	char *s1;

	s1 = strsep(&s, " \t");
	s = strsep(&s, " \t");

	if (s1 == NULL)
		return NULL;

	switch (lookup_value(s1, rc_dead)) {
		case RC_SAVE:
			dead_save = istrue(s);
			return NULL;

		case RC_DIR:
			free(deaddir);
			s = translate_delim_string(s);
			if (s == NULL )
				deaddir = NULL;
			else
				deaddir = strdup(s);
			return NULL;

		case RC_ADVISE:
			sendadvise = istrue(s);
			return NULL;

		case RC_MAILER:
			free(mailer);
			s = translate_delim_string(s);
			if (s == NULL )
				mailer = NULL;
			else
				mailer = strdup(s);
			return NULL;

		default:
			return "UNKNOWN COMMAND";
	}
}


/*********************************************************************
 * Print the current dead options to a new rc file
 */



void
rc_print_death(FILE * f, char *name)
{

#define L(x) lookup_name(x, rc_dead)

	fputs("# Unexpected (exit) death options\n"
	      "# \tsave - true/false enables saves on unexpected exits\n"
	      "# \tdir - directory in which saves are done\n"
	      "# \tadvise - true/false enables"
	      " mailing of message to user\n"
	      "# \tmailer - name of the mailer"
	      " to use (default==mail)\n\n", f);

	fprintf(f, "%s\t%s\t%s\n", name, L(RC_SAVE), truefalse(dead_save));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_DIR),
		make_delim_string(deaddir == NULL ? "." : deaddir));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_ADVISE), truefalse(sendadvise));
	fprintf(f, "%s\t%s\t%s\n", name, L(RC_MAILER),
		make_delim_string(mailer == NULL ? "mail" : mailer));
	putc('\n', f);
	putc('\n', f);

#undef L
}

void
deathtrap(int sig)
{
	char *why, *file, *user, *dir;
	FILE *f;
	int n, size;
	u_char *slash, *p;
	FLAG done_save;

	ved_reset();		/* reset the terminal to sanity */

	switch (sig) {

#ifdef SIGABRT
		case SIGABRT:
			why = "abort signal";
			break;
#endif

#ifdef SIGHUP
		case SIGHUP:
			why = "hangup signal";
			break;
#endif
#ifdef SIGILL
		case SIGILL:
			why = "illegal instruction";
			break;
#endif
#ifdef SIGBUS
		case SIGBUS:
			why = "bus error";
			break;
#endif
#if defined(SIGSEGV) && !defined(TOS)
		case SIGSEGV:
			why = "segmentation violation";
			break;
#endif
#ifdef SIGSYS
		case SIGSYS:
			why = "a munged system call";
			break;
#endif
#ifdef SIGPIPE
		case SIGPIPE:
			why = "the pipe reader died";
			break;
#endif
#ifdef SIGTERM
		case SIGTERM:
			why = "a terminate signal";
			break;
#endif
#if !MINIX
#ifdef SIGUSR1
		case SIGUSR1:
			why = "a SIGUSR1";
			break;
#endif
#ifdef SIGUSR2
		case SIGUSR2:
			why = "a SIGUSR2";
			break;
#endif
#endif
		default:
			why = "something went very wrong";
			break;
	}

	/* attempt save if there's a buffer with changes.... */

	if (dead_save) {

		file = NULL;
		done_save = FALSE;

		asprintf(&dir, "%s/DEAD-VED.%d.", (deaddir == NULL ? "." : deaddir),
			 getpid());

		for (n = 0; n < num_buffers; n++) {

			bf = &buffers[n];

			free(file);

			p = bf->filename;
			if (p == NULL)
				p = "NONAME";

			slash = rindex(p, '/');
			if (slash != NULL)
				p = slash + 1;

			asprintf(&file, "%s%s", dir, p);

			if ((bf->changed == TRUE) && (bf->buf != NULL) &&
			  (size = bf->bsize) && (f = fopen(file, "w"))) {

				fprintf(f, "*** SAVED VED BUFFER for %s\n", p);
				fprintf(f, "*** Ved died after a %s\n", why);
				fprintf(f, "*****\n");

				fwrite(bf->buf, size, sizeof(u_char), f);
				fclose(f);
				done_save = TRUE;
			}
		}

		if (sendadvise && done_save == TRUE && (user = getenv("USER"))) {
			char *cmd;

			asprintf(&cmd, "%s %s -s 'Dead Ved' ",
			       (mailer == NULL) ? "mail" : mailer, user);
			if ((f = popen(cmd, "w")) != NULL) {
				fprintf(f, "Ved suffered an unexpected death\n");
				fprintf(f, "due to a %s\n", why);
				fprintf(f, "We managed to save the buffer(s) in the ");
				fprintf(f, "file(s) %sxx\n", dir);
				pclose(f);
			}
		}
	}
	fprintf(stderr, "Unexpected death of ved after a %s\n", why);


	exit(sig);
}

/* EOF */
