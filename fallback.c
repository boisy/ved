/* fallback.c */

/* This was donated by Thomas E. Dickey,  dickey@clark.net
 * It supplies some functions which are not present in
 * some other unixes, mainly solaris.
 */
 

#include "ved.h"

#include <ctype.h>

#ifdef NEED_ASPRINTF
/*
 * Replacement for sprintf, allocates buffer on the fly according to what's
 * needed for its arguments.  -T.Dickey
 */

typedef enum { Flags, Width, Prec, Type, Format } PRINTF;

#define VA_INTGR(type) ival = va_arg((ap), type)
#define VA_FLOAT(type) fval = va_arg((ap), type)
#define VA_POINT(type) pval = (void *)va_arg((ap), type)

#define NUM_WIDTH 10	/* allow for width substituted for "*" in "%*s" */
		/* also number of chars assumed to be needed in addition
		   to a given precision in floating point formats */

#define GROW_EXPR(n) (((n) * 3) / 2)
#define GROW_SIZE 256

int ved_asprintf (char **pptr, const char *fmt, ...)
{
    size_t tmp_len = GROW_SIZE;
    size_t have, need;
    char *tmp_ptr = 0;
    char *fmt_ptr;
    char *dst_ptr = 0;
    va_list ap;
    size_t dst_len = 0;

    if (fmt == 0 || *fmt == '\0')
	return 0;

    va_start(ap,fmt);

    need = strlen(fmt) + 1;
    if ((fmt_ptr = malloc(need*NUM_WIDTH)) == 0
    || (tmp_ptr = malloc(tmp_len)) == 0) {
	return 0;
    }

    if (dst_ptr == 0) {
	dst_ptr = malloc(have = GROW_SIZE + need);
    } else {
	have = strlen(dst_ptr) + 1;
	need += dst_len;
	if (have < need)
	    dst_ptr = realloc(dst_ptr, have = GROW_SIZE + need);
    }

    while (*fmt != '\0') {
	if (*fmt == '%') {
	    static char dummy[] = "";
	    PRINTF state = Flags;
	    char *pval   = dummy;	/* avoid const-cast */
	    double fval  = 0.0;
	    int done     = FALSE;
	    int ival     = 0;
	    int prec     = -1;
	    int type     = 0;
	    int used     = 0;
	    int width    = -1;
	    size_t f     = 0;

	    fmt_ptr[f++] = *fmt;
	    while (*++fmt != '\0' && !done) {
		fmt_ptr[f++] = *fmt;

		if (isdigit(*fmt)) {
		    int num = *fmt - '0';
		    if (state == Flags && num != 0)
			state = Width;
		    if (state == Width) {
			if (width < 0)
			    width = 0;
			width = (width * 10) + num;
		    } else if (state == Prec) {
			if (prec < 0)
			    prec = 0;
			prec = (prec * 10) + num;
		    }
		} else if (*fmt == '*') {
		    VA_INTGR(int);
		    if (state == Flags)
			    state = Width;
		    if (state == Width) {
			    width = ival;
		    } else if (state == Prec) {
			    prec = ival;
		    }
		    sprintf(&fmt_ptr[--f], "%d", ival);
		    f = strlen(fmt_ptr);
		} else if (isalpha(*fmt)) {
		    done = TRUE;
		    switch (*fmt) {
		    case 'Z': /* FALLTHRU */
		    case 'h': /* FALLTHRU */
		    case 'l': /* FALLTHRU */
		    case 'L': /* FALLTHRU */
			done = FALSE;
			type = *fmt;
			break;
		    case 'o': /* FALLTHRU */
		    case 'i': /* FALLTHRU */
		    case 'd': /* FALLTHRU */
		    case 'u': /* FALLTHRU */
		    case 'x': /* FALLTHRU */
		    case 'X': /* FALLTHRU */
		    if (type == 'l')
			VA_INTGR(long);
		    else if (type == 'Z')
			VA_INTGR(size_t);
		    else
			VA_INTGR(int);
			used = 'i';
			break;
		    case 'f': /* FALLTHRU */
		    case 'e': /* FALLTHRU */
		    case 'E': /* FALLTHRU */
		    case 'g': /* FALLTHRU */
		    case 'G': /* FALLTHRU */
			VA_FLOAT(double);
			used = 'f';
			break;
		    case 'c':
			VA_INTGR(int);
			used = 'c';
			break;
		    case 's':
			VA_POINT(char *);
			if (pval == 0)
			    pval = "";
			if (prec < 0)
			    prec = strlen(pval);
			used = 's';
			break;
		    case 'p':
			VA_POINT(void *);
			used = 'p';
			break;
		    case 'n':
			VA_POINT(int *);
			used = 0;
			break;
		    default:
			break;
		    }
		} else if (*fmt == '.') {
		    state = Prec;
		} else if (*fmt == '%') {
		    done = TRUE;
		    used = '%';
		}
	    }
	    fmt_ptr[f] = '\0';

	    if (prec > 0) {
		switch (used) {
		case 'f':
		    if (width < prec + NUM_WIDTH)
			width = prec + NUM_WIDTH;
		case 'i':
		case 'p':
		    if (width < prec + 2)
			width = prec + 2; /* leading sign/space/zero, "0x" */
		case 'c':
		case '%':
		    break;
		default:
		    if (width < prec)
			width = prec;
		}
	    }
	    if (width >= (int)tmp_len) {
		tmp_len = GROW_EXPR(tmp_len + width);
		tmp_ptr = realloc(tmp_ptr, tmp_len);
	    }

	    switch (used) {
	    case 'i':
	    case 'c':
		sprintf(tmp_ptr, fmt_ptr, ival);
		break;
	    case 'f':
		sprintf(tmp_ptr, fmt_ptr, fval);
		break;
	    default:
		sprintf(tmp_ptr, fmt_ptr, pval);
		break;
	    }
	    need = dst_len + strlen(tmp_ptr) + 1;
	    if (need >= have) {
		dst_ptr = realloc(dst_ptr, have = GROW_EXPR(need));
	    }
	    strcpy(dst_ptr + dst_len, tmp_ptr);
	    dst_len += strlen(tmp_ptr);
	} else {
	    if ((dst_len + 2) >= have) {
		dst_ptr = realloc(dst_ptr, (have += GROW_SIZE));
	    }
	    dst_ptr[dst_len++] = *fmt++;
	}
    }

    free(tmp_ptr);
    free(fmt_ptr);
    dst_ptr[dst_len] = '\0';
    *pptr = dst_ptr;
    va_end(ap);
    return strlen(dst_ptr);
}
#endif

#ifdef NEED_STRSEP
char *ved_strsep (char **pptr, const char *accept)
{
    char *first, *last;

    first = *pptr;
    if (! first || *first == 0)
	return 0;

    for (last = first, *pptr = 0; *last; last++) {
	if (strchr(accept, *last)) {
	    *last++ = 0;
	    *pptr = last;
	    break;
	}
    }

    return first;
}
#endif
