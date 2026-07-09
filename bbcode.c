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
char** lexer(const char *code);

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
		"[url=https://nathancampos.me/assets/images/http-status/404.jpg]but "
		"it's honest work[/url].\n\nThis is something/somewhere, and 2+2=4.";
	char **tokens;
	char **cur;

	/* Pass the BBCode through the lexer and extract tokens. */
	tokens = lexer(code);

	/* Print all tokens. */
	cur = tokens;
	while (cur != NULL) {
		printf("\"%s\"\n", *cur);
		cur++;
	}

	/* Free all tokens. */
	cur = tokens;
	do {
		if (cur)
			free(cur);
	} while (cur++ != NULL);
	free(tokens);

	return 0;
}

/**
 * Lexes the entire code provided.
 *
 * @param code   BBCode string to be lexed.
 *
 * @return List of tokens lexed from the provided string.
 */
char** lexer(const char *code) {
	const char *start;
	const char *end;
	char **tokens;
	char *buf;
	int count;
	int ret;

	/* Allocate our initial token list. */
	count = 1;
	tokens = malloc(sizeof(char*));

	buf = NULL;
	start = code;
	while ((ret = lex(start, &start, &end)) > 0) {
		strcatdelim_alloc(&buf, start, end);
		tokens[count - 1] = buf;

		printf("LEXER \"%s\"\n", buf);

		/* Move to the next token in the string. */
		start = end;
		count++;

		/* Reset all buffers. */
		tokens = realloc(tokens, count * sizeof(char*));
		buf = tokens[count - 1] = NULL;
	}

	return tokens;
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
