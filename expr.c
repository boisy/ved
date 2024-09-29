/* expr.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

/*********************************************************************
 * Parse a numeric expression.
 * 
 * This code has been adapted from a similiar routine in
 * 'C' The Complete Reference, page 555.
 * 
 * Mods have been made to the error routine,
 * Hex and ASC constants have been added,
 * Some variables used have been made private.
 * 
 * The following operators are supported:
 * +, -, *, /, %
 * -, + unary negate
 * ()
 * 
 * The following contants are supported:
 * nnn - decimal number
 * $xx - hexadecimal number
 * 'x' - ascii value of character 'x' (ascii escapes supported)
 * ^X  - ascii value of character X - 0x40.
 * 
 * This implementation does NOT support white space in
 * expressions--white space, commas, semi-commas, and
 * periods are treated as expression separators/terminators.
 */

#include "ved.h"
#include "limits.h"

static void arith(char, int *, int *), get_token(void), level2(int *),
    level3(int *), level5(int *), level6(int *), serror(int);


/* This sets the variable type to int. If float is used changes will
 * have to be made to the call to atoi() and the scan routine in
 * get_tok() which skips over the ascii representation of the number.
 */

enum {
	DELIMITER = 1, NUMBER, E_SYNTAX, E_PARENTH, E_NULL
};

static char *prog;		/* pointer to expression */
static char operator;		/* current token */
static char tok_type;		/* current token type */
static int *errcode;		/* error (0==success) */
static int tok_value;		/* value of current token if NUMBER */


/*********************************************************************
 * Entry point into the parser
 */

u_char *
get_exp(int *result, char *p, int *err)
				/* pointer to the result                  */
				/* pointer to the string to parse         */
				/* pointer to the error variable          */
{
	prog = p;
	errcode = err;
	*errcode = 0;

	get_token();
	if (tok_type == 0)
		serror(E_NULL);
	else
		level2(result);
	return prog;
}

static void
level2(int *result)
{				/* add or subtract two terms */
	int hold;
	char op;

	level3(result);
	while (((op = operator) == '+') || op == '-') {
		get_token();
		level3(&hold);
		arith(op, result, &hold);
	}
}

static void
level3(int *result)
{				/* mult or divide two factors */
	int hold;
	char op;

	level5(result);
	while (((op = operator) == '*') || op == '/' || op == '%') {
		get_token();
		level5(&hold);
		arith(op, result, &hold);
	}
}


static void
level5(int *result)
{				/* unary + or - */
	char op = 0;

	if (((tok_type == DELIMITER) && operator == '+') || (operator == '-')) {
		op = operator;
		get_token();
	}
	level6(result);
	if (op == '-')
		*result = -(*result);
}

static void
level6(int *result)
{				/* parenthesized expression */

	if (operator == '(' && tok_type == DELIMITER) {
		get_token();
		level2(result);
		if (operator != ')')
			serror(E_PARENTH);
		get_token();
	} else {
		*result = tok_value;
		get_token();
	}
}

static void
arith(char o, int *r, int *h)
{				/* perform arithmitic  */
	if (o == '-')
		*r -= *h;
	else if (o == '+')
		*r += *h;
	else if (o == '*')
		*r = (*r) * (*h);
	else if (o == '/')
		*r = (*r) / (*h);
	else if (o == '%')
		*r = (*r) % (*h);
}


static void
serror(int error)
{				/* display a syntax error */
	*errcode = error;
}

static void
get_token(void)
{
	tok_type = 0;
	operator = 0;

	/* whitespace, EOL, punct == end of the expression */

	if ((*prog == '\0') || (index(" \t,;:.=\n", *prog) != NULL)) {
		goto EXIT;
	}
	/* Operator */

	if (index("+-/*%()", *prog)) {
		tok_type = DELIMITER;
		operator = *prog++;
		goto EXIT;
	}
	/* decimal, 0xnn (hex) or 0nn (oct) */

	if (v_isdigit(*prog)) {
		tok_value = strtol(prog, &prog, 0);
		if ((tok_value == LONG_MIN) || (tok_value == LONG_MAX))
			serror(E_SYNTAX);
		tok_type = NUMBER;
		goto EXIT;
	}
	/* control value */

	if (*prog == '^') {
		prog++;
		if (*prog <= 0x40)
			serror(E_SYNTAX);
		tok_value = *prog++ - 0x40;
		tok_type = NUMBER;
		goto EXIT;
	}
	/* Alternate hex */

	if (*prog == '$') {
		tok_value = strtol(++prog, &prog, 16);
		if ((tok_value == LONG_MIN) || (tok_value == LONG_MAX))
			serror(E_SYNTAX);
		tok_type = NUMBER;
		goto EXIT;
	}
	/* Ascii constant */

	if (*prog == '\'') {
		prog++;
		if (*prog == '\\') {
			prog++;
			switch (*prog) {
				case '\\':
					tok_value = '\\';
					break;
				case 'a':
					tok_value = '\a';
					break;
				case 'b':
					tok_value = '\b';
					break;
				case 'f':
					tok_value = '\f';
					break;
				case 'n':
					tok_value = '\n';
					break;
				case 'r':
					tok_value = '\r';
					break;
				case 't':
					tok_value = '\t';
					break;
				case 'v':
					tok_value = '\v';
					break;

				default:
					serror(E_SYNTAX);
			}
		} else
			tok_value = *prog;
		tok_type = NUMBER;

		prog++;
		if (*prog++ == '\'')
			goto EXIT;

		/* not 'x' fall through to syntax error */
	}
	serror(E_SYNTAX);	/* unrecognized token */

      EXIT:;
}

/* EOF */
