/**
 * bbcode.c
 * Compiles BBCode into an intermediate language that can later be used by other
 * applications to convert it into other markup languages.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

/* Function prototypes. */
int lex(const char *str, const char **start, const char **end);

/**
 * Application's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Array of command line arguments.
 *
 * @return Return code.
 */
int main(int argc, char **argv) {
	char *code = "This is an example of [b]BBCode[/b]!\n\nIt's very simple, "
		"[url=https://nathancampos.me/assets/images/http-status/404.jpg]but it's "
		"honest work[/url].";
	const char *start;
	const char *end;
	int ret;

	start = code;
	while ((ret = lex(start, &start, &end)) > 0) {
		const char *tmp = start;

		putchar('"');
		while (tmp != end) {
			putchar(*tmp++);
		}
		printf("\" (%d)\n", ret);

		start = end;
	}

	return ret;
}

/**
 * A very simple lexer to find the beginning and the end of tokens in a string.
 *
 * @param str   String to be scanned for tokens.
 * @param start Pointer to the start of a token.
 * @param end   Pointer to the end of a token.
 *
 * @return The size of our token if successful. 0 if we've reached the end of
 *         the string. Less than 0 if an error occurred.
 */
int lex(const char *str, const char **start, const char **end) {
	const char *delim = "[]/=\n";

	/* Are we wasting our time? */
	if (str == NULL)
		return -1;

	/* Ignore CR characters. */
	str += strspn(str, "\r");

	/* Check if we've reached the end of the string. */
	if (*str == '\0') {
		*start = *end = NULL;
		return 0;
	}

	/* Read the delimiter or a string until we reach the next delimiter. */
	*start = str;
	if (chrchr(*str, delim)) {
		*end = str + 1;
	} else {
		*end = str + strcspn(str, delim);
	}

	return *end - *start;
}
