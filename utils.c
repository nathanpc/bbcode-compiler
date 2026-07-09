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
int strcatdelim(char *buf, const char *start, const char *end) {
	int len;
	char *cbuf;
	const char *cdelim;

	/* Skips the original string and position the cursor at NUL. */
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
int strcatdelim_alloc(char **buf, const char *start, const char *end) {
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
