/**
 * bb2apt.c
 * Converts the compiled BBCode intermediate language to the amazing APT (Almost
 * Plain Text).
 *
 * More information about APT: https://github.com/nathanpc/almost-plain-text
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "atom.h"
#include "shims.h"

/* Private functions. */
void print_simple_tag(const atom_t *atom);
void print_tag_group(const atom_group_t *atoms);
void print_image_tag_group(const atom_group_t *atoms);
void print_header_tag_group(const atom_group_t *atoms);
int should_group_tag(const atom_t *atom);


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
	atom_group_t *atomsgrp;
	atom_t *atom;
	int ret;
	FILE *stream;

	/* Take a memory snapshot for memory leak detection. */
	mem_snapshot_start();

	/* Initialize variables. */
	ret = 0;
	atom = NULL;
	atomsgrp = NULL;
	stream = stdin;

	/* Check if we should read from a file. */
	if (argc > 1) {
		stream = fopen(argv[1], "r");
		if (stream == NULL) {
			fprintf(stderr, "error: failed to open file '%s' for reading.\n",
				argv[1]);
			return 1;
		}
	}

	/* Go through input line-by-line. */
	while (fgets(line, sizeof(line), stream)) {
		char *buf;

		/* Removing newline from the end of the string. */
		buf = line + strlen(line) - 1;
		if (*buf == '\n')
			*buf = '\0';

		/* Allocate a new atom if needed. */
		if (atom == NULL)
			atom = atom_new(-1);

		/* Are we parsing a new atom or continuing from a previous one? */
		if (ret == 0) {
			/* Parse atom from line. */
			ret = atom_parse_line(atom, line);
		} else {
			/* Do we need to keep parsing the last line? */
			ret = atom_parse_continue(atom, line);
		}

		/* Should we stop everything or continue parsing? */
		if (ret == -1) {
			break;
		} else if (ret >= 1) {
			continue;
		}

		/* Handle the creation of paragraphs. */
		if (atom->type == ATOM_LINEBREAK)
			putchar('\n');

		/* Deal with tags that may span more than a single atom. */
		switch (atom->type) {
		case ATOM_TAG_OPEN:
			/* Check if this is a tag that doesn't need to be grouped. */
			if (atomsgrp || !should_group_tag(atom))
				goto handle_atom;

			atomsgrp = atom_group_new();
			atom_group_append(atomsgrp, atom);
			atom = NULL;
			break;
		case ATOM_TAG_ARG:
			/* Check if we have a valid tag group to append this argument to. */
			if (atomsgrp == NULL) {
				fprintf(stderr, "error: atom argument found without group\n");
				goto finish;
			}

			/* Append the argument to the current tag group. */
			atom_group_append(atomsgrp, atom);
			atom = NULL;
			break;
		default:
handle_atom:
			/* Handle previous atom group first if one exists. */
			if (atomsgrp) {
				atom_t *first = *atomsgrp->group;

				/* Add atoms to group until the tag is closed. */
				if (first->type == ATOM_TAG_OPEN) {
					atom_group_append(atomsgrp, atom);
					if ((atom->type != ATOM_TAG_CLOSE) &&
							strcmp(atom->value, first->value)) {
						atom = NULL;
						break;
					}

					/* Ensure that we ignore the last tag close. */
					atom = NULL;
				}

				/* Print out the entire tag group and move on. */
				print_tag_group(atomsgrp);
				atom_group_free(atomsgrp);
				atomsgrp = NULL;
			}

			/* Print the atom and move on. */
			if (atom) {
				if (atom->type != ATOM_LINEBREAK)
					print_simple_tag(atom);
				atom_free(atom);
				atom = NULL;
			}
			break;
		}
	}

finish:
	/* Close the file stream if needed. */
	if (stream != stdin) {
		fclose(stream);
		stream = NULL;
	}

	/* Free resources. */
	atom_group_free(atomsgrp);
	atomsgrp = NULL;
	atom_free(atom);
	atom = NULL;

	/* Check for memory leaks. */
	mem_snapshot_report();

	return ret;
}

/**
 * Prints out a simple tag associated with an atom that can simply be
 * substituted directly without requiring any grouping.
 *
 * @param atom  Atom object to be converted.
 */
void print_simple_tag(const atom_t *atom) {
	if ((atom->type == ATOM_RAW) || (atom->type == ATOM_PREFMT)) {
		/* Regular text and preformatted sections. */
		printf("%s", atom->value);
	} else if (atom->type >= ATOM_TAG_OPEN) {
		/* Actual tag for us to deal with. */
		const char *v;

		if (atom->type == ATOM_TAG_ARG) {
			fprintf(stderr, "error: Trying to print a tag argument atom.\n");
			return;
		}

		/* Print the tag. */
		v = atom->value;
		if (!strcmp(v, "B")) {
			printf("**");
		} else if (!strcmp(v, "I")) {
			putchar('*');
		} else if (!strcmp(v, "U")) {
			putchar('_');
		} else if (!strcmp(v, "S")) {
			putchar('-');
		} else if (!strcmp(v, "PRE")) {
			printf("pre");
			printf("h6");
		} else if (!strcmp(v, "URL")) {
			putchar('a');
		} else if (!strcmp(v, "IMG")) {
			printf("img");
		} else if (!strcmp(v, "CODE")) {
		} else if (!strcmp(v, "QUOTE")) {
		} else if (!strcmp(v, "COLOR")) {
		} else if (!strcmp(v, "TABLE")) {
		} else if (!strcmp(v, "*")) {
		} else if (!strcmp(v, "LIST")) {
		} else if (!strcmp(v, "OLIST")) {
 		} else if (!strcmp(v, "TR") || !strcmp(v, "TH") || !strcmp(v, "TD")) {
			putchar('|');
		} else {
			fprintf(stderr, "warning: unknown tag %s\n", v);
			printf("%s", v);
		}
	}
}

/**
 * Prints out an atom group in APT format.
 *
 * @param atoms Atom group object to be converted.
 */
void print_tag_group(const atom_group_t *atoms) {
	const char *v = atoms->group[0]->value;
	int i;

	if (!strcmp(v, "H1") || !strcmp(v, "H2") || !strcmp(v, "H3") ||
			!strcmp(v, "H4") || !strcmp(v, "H5") || !strcmp(v, "H6")) {
		print_header_tag_group(atoms);
	} else if (!strcmp(v, "IMG")) {
		print_image_tag_group(atoms);
	} else if (!strcmp(v, "COLOR")) {
		for (i = 1; i < atoms->len; i++) {
			if (atoms->group[i]->type < ATOM_TAG_OPEN)
				printf("%s", atoms->group[i]->value);
		}
	} else {
		fprintf(stderr, "warning: unknown tag group %s to be printed.\n", v);
	}
}

/**
 * Prints out an header associated with an atom group.
 *
 * @param atoms Atom group object to be converted.
 */
void print_header_tag_group(const atom_group_t *atoms) {
	const char *c;
	char ch;

	/* Decide the character to use for the header underline. */
	if (!strcmp(atoms->group[0]->value, "H1")) {
		ch = '=';
	} else {
		ch = '-';
	}

	/* Print out the underlined characters. */
	printf("%s\n", atoms->group[1]->value);
	c = atoms->group[1]->value;
	while (*c != '\0') {
		putchar(ch);
		c++;
	}
}

/**
 * Prints out an image text associated with an atom group.
 *
 * @param atoms Atom group object to be converted.
 */
void print_image_tag_group(const atom_group_t *atoms) {
	/* Print the alt text. */
	if (atoms->len > 3) {
		int i;
		for (i = 2; i < (atoms->len - 1); i++)
			print_simple_tag(atoms->group[i]);
		printf(":\n");
	}

	/* Print the image URL. */
	printf("%s", atoms->group[1]->value);
}

/**
 * Checks if a tag should be grouped together for later being exported in APT.
 *
 * @param atom Opening tag atom to be checked.
 *
 * @return TRUE if we should group the atoms until the closing of the tag. FALSE
 *         if the atom should be directly printed out.
 */
int should_group_tag(const atom_t *atom) {
	const char *v = atom->value;
	return atom_args_maybe(atom) || !strcmp(v, "H1") || !strcmp(v, "H2") ||
		!strcmp(v, "H3") || !strcmp(v, "H4") || !strcmp(v, "H5") ||
		!strcmp(v, "H6") || !strcmp(v, "LIST") || !strcmp(v, "OLIST") ||
		!strcmp(v, "PRE");
}
