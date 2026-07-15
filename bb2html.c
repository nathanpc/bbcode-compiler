/**
 * bb2html.c
 * Converts the compiled BBCode intermediate language to HTML.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "utils.h"
#include "atom.h"

/**
 * Application's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Array of command line arguments.
 *
 * @return Return code.
 */
int main(int argc, char **argv) {
	char line[LINE_MAX];
	atom_t atom;
	int ret;

	atom.value = NULL;
	while (fgets(line, sizeof(line), stdin)) {
		char *buf;

		/* Removing newline from the end of the string. */
		buf = line + strlen(line) - 1;
		if (*buf == '\n')
			*buf = '\0';

		printf("GOT THIS '%s'\n", line);
		
		/* Are we parsing a new atom or continuing from a previous one? */
		if (ret == 0) {
			/* Parse atom from line. */
			ret = atom_parse_line(&atom, line);
		} else {
			/* Do we need to keep parsing the last line? */
			ret = atom_parse_continue(&atom, line);
		}

		/* Should we stop everything or continue parsing? */
		if (ret == -1) {
			break;
		} else if (ret >= 1) {
			continue;
		}

		atom_print(&atom);
	}

	return ret;
}
