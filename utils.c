/**
 * utils.c
 * A collection of utility functions to help us be more productive in C.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**
 * Checks if a character (ch) is contained in a list of characters (chars).
 *
 * @param ch    Character to be matched.
 * @param chars String with a list of characters to match.
 *
 * @return TRUE if the character ch is in chars, FALSE otherwise.
 */
int chrchr(char ch, const char *chars) {
	const char *buf = chars;
	while (*buf != '\0') {
		if (*buf == ch)
			return 1;
		buf++;
	}

	return 0;
}

/**
 * Concatenates a delimited string to the end of another string. Similar to
 * strcat, except the string to be appended has a start and an end point.
 *
 * @param buf   Buffer to append the delimited string to. Must be able to hold
 *              both strings and the NUL terminator.
 * @param start Beginning of the delimited string.
 * @param end   End of the delimited string (not included in buf).
 *
 * @return Final size of the string.
 */
size_t strcatdelim(char *buf, const char *start, const char *end) {
	size_t len;
	char *cbuf;
	const char *cdelim;

	/* Skips the original string and position the cursor at NUL. */
	len = 0;
	for (cbuf = buf; *cbuf != '\0'; cbuf++)
		len++;

	/* Append the delimited string. */
	for (cdelim = start; cdelim != end; cdelim++) {
		*cbuf = *cdelim;
		cbuf++;
		len++;
	}

	/* NUL terminate the buffer. */
	*cbuf = '\0';

	return len;
}

/**
 * Concatenates a delimited string to the end of another string and
 * automatically allocates or reallocates the buffer if needed.
 *
 * @param buf   Buffer to append the delimited string to.
 * @param start Beginning of the delimited string.
 * @param end   End of the delimited string (not included in buf).
 *
 * @return Final size of the string.
 */
size_t strcatdelim_alloc(char **buf, const char *start, const char *end) {
	size_t len;

	/* Get the length of the new buffer. */
	if (*buf == NULL) {
		len = 0;
	} else {
		len = strlen(*buf);
	}
	len += 1 + end - start;

	/* Ensure our buffer has enough space to hold our token. */
	if (*buf == NULL) {
		*buf = malloc(len * sizeof(char));
		*buf[0] = '\0';
	} else {
		*buf = realloc(*buf, len * sizeof(char));
	}

	/* Actually perform the concatenation and return. */
	return strcatdelim(*buf, start, end);
}

/**
 * Portable case insensitive string comparison.
 * Source: https://stackoverflow.com/a/5820991
 *
 * @param a String to be compared.
 * @param b String to be compared.
 *
 * @return Same as strcmp.
 */
int strcmpci(char const *a, char const *b) {
	for (;; a++, b++) {
		int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
		if (d != 0 || !*a)
			return d;
	}
}

/**
 * Copies a string into another until a specific character (non-inclusive), or
 * if the character is not found, copies the entire string.
 *
 * @param dest Destination string to write to.
 * @param src  Source string to copy characters from.
 * @param ch   Limiting character (non-inclusive).
 *
 * @return Pointer in src where the delimiter was found.
 */
const char* strcpyu(char *dest, const char *src, char ch) {
	const char *sc;
	char *dc;

	/* Copy characters over until the delimiter or the end of the string. */
	for (dc = dest, sc = src; (*sc != ch) && (*sc != '\0'); dc++, sc++)
		*dc = *sc;
	*dc = '\0';
	
	return sc;
}

/**
 * Compares the ends of two strings.
 * Source: https://stackoverflow.com/a/744822
 *
 * @param str    String to be compared.
 * @param suffix Suffix to find in string.
 *
 * @return Same as strcmp.
 */
int strcmpe(const char *str, const char *suffix) {
	size_t lenstr;
	size_t lensuffix;

	if (!str || !suffix)
		return -1;
	lenstr = strlen(str);
	lensuffix = strlen(suffix);
	if (lensuffix >  lenstr)
		return -1;

	return strncmp(str + lenstr - lensuffix, suffix, lensuffix);
}

/**
 * Tells you where to break a long line of text according to a line margin.
 *
 * @param text   Text to be broken up in smaller chunks.
 * @param margin Number of characters a line should have, excluding the break.
 *
 * @return Pointer to where in the string the break should happen or the
 *         position of the NUL terminator if the line doesn't exceed the margin.
 */
const char* wrap_text(const char *text, int margin) {
	const char *brkpoint;
	const char *buf;

	buf = text;
	brkpoint = NULL;
	while (*buf != '\0') {
		/* Store last place where we can break the text. */
		if ((*buf == ' ') || (*buf == '\n'))
			brkpoint = buf;

		/* Looks like we've found the margin limit. */
		if (((buf - text) >= margin) && brkpoint)
			return brkpoint;

		buf++;
	}

	return buf;
}

/**
 * Prints the contents of a text string (paragraph-format) wrapped to a specific
 * line margin.
 *
 * The prepended text won't be printed on the first line, the beginning of text,
 * only after a break.
 *
 * Last broken line, or the same line if text isn't long enough, won't get a
 * line break.
 *
 * @param text    Text to be printed and wrapped in paragraph format.
 * @param margin  Number of characters a line should have, excluding the line
 *                break character and including the prepend string.
 * @param prepend String to be prepended to each new line of the text. May be
 *                NULL or empty if nothing is to be prepended.
 */
void print_text_wrap(const char *text, int margin, const char *prepend) {
	const char *brk;
	const char *cursor;
	int pplen;

	/* Determine the size of the prepend string. */
	pplen = prepend ? (int)strlen(prepend) : 0;

	/* Print the text while wrapping it. */
	cursor = text;
	brk = cursor;
	while (*brk != '\0') {
		/* Go to the next line that *just wrapped* and indent it if needed. */
		if (cursor != text) {
			putchar('\n');
			if (pplen > 0)
				printf("%s", prepend);
		}

		/* Break up any long lines and print them out. */
		brk = wrap_text(cursor, margin - pplen);
		print_until(cursor, brk, 0);

		/* Skip the character where we've broken the line. */
		cursor = (char*)(brk + 1);
	}
}

/**
 * Prints a string until a definite point in itself.
 *
 * @param str       Begining of the string to be printed.
 * @param until     Point in the string to stop printing.
 * @param inclusive Point at until should also be printed out?
 */
void print_until(const char *str, const char *until, int inclusive) {
	const char *buf = str;

	/* Print the string character by character until the delimiter. */
	while (buf != until) {
		putchar(*buf);
		buf++;
	}

	/* Include the last character? */
	if (inclusive && (*buf != '\0'))
		putchar(*buf);
}
