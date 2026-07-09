/**
 * utils.c
 * A collection of utility functions to help us be more productive in C.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "utils.h"

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
