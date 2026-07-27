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
char* buffer_simple_tag(char *buf, const atom_t *atom);
void print_simple_tag(const atom_t *atom);
void print_tag_group(const atom_group_t *atoms);
char* buffer_link_tag_group(char *buf, const atom_group_t *atoms);
void print_link_tag_group(const atom_group_t *atoms);
void print_image_tag_group(const atom_group_t *atoms);
void print_header_tag_group(const atom_group_t *atoms);
void print_quote_tag_group(const atom_group_t *atoms);
void print_code_tag_group(const atom_group_t *atoms, int indent);
void print_list_tag_group(const atom_group_t *atoms);
int should_group_tag(const atom_t *atom);
int atom_just_whitespace(const atom_t *atom);

/* Global variables. */
static atom_group_t *urls;

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
	urls = atom_group_new();

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
					if ((atom->type == ATOM_TAG_CLOSE) &&
							!strcmp(atom->value, first->value)) {
						/* Ensure that we ignore the last tag close. */
						atom = NULL;
					} else {
						/* Move to the next atom of the group. */
						atom = NULL;
						break;
					}
				}

				/* Print out the entire tag group and move on. */
				print_tag_group(atomsgrp);
				atom_group_free(atomsgrp);
				atomsgrp = NULL;
			}

			/* Print the atom and move on. */
			if (atom) {
				print_simple_tag(atom);
				atom_free(atom);
				atom = NULL;
			}
			break;
		}
	}

	/* Print the links in our document. */
	putchar('\n');
	putchar('\n');
	for (ret = 0; ret < urls->len; ret++)
		printf("[%d]: %s\n", ret + 1, urls->group[ret]->value);
	ret = 0;

finish:
	/* Close the file stream if needed. */
	if (stream != stdin) {
		fclose(stream);
		stream = NULL;
	}

	/* Free resources. */
	atom_group_free(urls);
	urls = NULL;
	atom_group_free(atomsgrp);
	atomsgrp = NULL;
	atom_free(atom);
	atom = NULL;

	/* Check for memory leaks. */
	mem_snapshot_report();

	return ret;
}

/**
 * Appends a simple atom into a string so that it can later be broken at a set
 * margin cleanly.
 *
 * @param buf  String buffer to get the atom appended to.
 * @param atom Atom object to be converted.
 *
 * @return Position of the NUL terminator after appending the atom.
 */
char* buffer_simple_tag(char *buf, const atom_t *atom) {
	if ((atom->type == ATOM_RAW) || (atom->type == ATOM_PREFMT)) {
		/* Regular text and preformatted sections. */
		strcat(buf, atom->value);
	} else if (atom->type >= ATOM_TAG_OPEN) {
		/* Actual tag for us to deal with. */
		const char *v;

		if (atom->type == ATOM_TAG_ARG) {
			fprintf(stderr, "error: Trying to print a tag argument atom.\n");
			return buf + strlen(buf);
		}

		/* Print the tag. */
		v = atom->value;
		if (!strcmp(v, "B")) {
			strcat(buf, "**");
		} else if (!strcmp(v, "I")) {
			strcat(buf, "*");
		} else if (!strcmp(v, "U")) {
			strcat(buf, "_");
		} else if (!strcmp(v, "S")) {
			strcat(buf, "-");
		} else if (!strcmp(v, "TABLE")) {
 		} else if (!strcmp(v, "TR") || !strcmp(v, "TH") || !strcmp(v, "TD")) {
			strcat(buf, "|");
		} else {
			fprintf(stderr, "warning: unknown tag %s\n", v);
			strcat(buf, v);
		}
	}

	return buf + strlen(buf);
}


/**
 * Prints out a simple tag associated with an atom that can simply be
 * substituted directly without requiring any grouping.
 *
 * @param atom Atom object to be converted.
 */
void print_simple_tag(const atom_t *atom) {
	char *buf;

	/* We've encountered a deliberate line break. */
	if (atom->type == ATOM_LINEBREAK) {
		putchar('\n');
	}

	/* Allocate a string that's definitely big enough for anything. */
	buf = malloc((strlen(atom->value) + 20) * sizeof(char));
	*buf = '\0';

	/* Buffer the output onto the string we've created and print it out. */
	buffer_simple_tag(buf, atom);
	printf("%s", buf);

	/* Free resources. */
	free(buf);
	buf = NULL;
}

/**
 * Prints out an atom group in APT format.
 *
 * @param atoms Atom group object to be converted.
 */
void print_tag_group(const atom_group_t *atoms) {
	const char *v = atoms->group[0]->value;
	int i;

	if (!strcmp(v, "IMG")) {
		print_image_tag_group(atoms);
	} else if (!strcmp(v, "URL")) {
		print_link_tag_group(atoms);
	} else if (!strcmp(v, "H1") || !strcmp(v, "H2") || !strcmp(v, "H3") ||
			!strcmp(v, "H4") || !strcmp(v, "H5") || !strcmp(v, "H6")) {
		print_header_tag_group(atoms);
	} else if (!strcmp(v, "QUOTE")) {
		print_quote_tag_group(atoms);
	} else if (!strcmp(v, "CODE")) {
		print_code_tag_group(atoms, 1);
	} else if (!strcmp(v, "PRE")) {
		print_code_tag_group(atoms, 0);
	} else if (!strcmp(v, "LIST") || !strcmp(v, "OLIST")) {
		print_list_tag_group(atoms);
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
 * Appends a full link tag (excluding its link) associated with an atom group to
 * a string buffer.
 *
 * @param buf   String buffer that will have the atom appended to.
 * @param atoms Atom group object to be converted.
 *
 * @return Position of the NUL terminator after appending the atom.
 */
char* buffer_link_tag_group(char *buf, const atom_group_t *atoms) {
	char *cursor;
	atom_t *url;
	size_t len;
	int i;

	/* Buffer the associated text (body of the link). */
	for (i = 2; i < (atoms->len - 1); i++)
		cursor = buffer_simple_tag(buf, atoms->group[i]);

	/* Store the URL and buffer the reference tag. */
	url = atom_new(ATOM_RAW);
	url->value = strdup(atoms->group[1]->value);
	atom_group_append(urls, url);
	len = sprintf(cursor, " [%d]", urls->len);

	return cursor + len;
}

/**
 * Prints out a link associated with an atom group.
 *
 * @param atoms Atom group object to be converted.
 */
void print_link_tag_group(const atom_group_t *atoms) {
	char *buf;
	size_t len;
	int i;

	/* Make a buffer big enough to hold our entire tag group. */
	len = 0;
	for (i = 0; i < atoms->len; i++)
		len += strlen(atoms->group[i]->value);
	buf = malloc((len + 1) * sizeof(char));

	/* Buffer the link text and print it out. */
	buffer_link_tag_group(buf, atoms);
	printf("%s", buf);

	/* Free resources. */
	free(buf);
	buf = NULL;
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
 * Prints out quoted text associated with an atom group.
 *
 * @param atoms Atom group object to be converted.
 */
void print_quote_tag_group(const atom_group_t *atoms) {
	const char *author;
	char buf[LINE_MAX];
	const char *brk;
	char *cursor;
	int i;

	/* Get the author of the quote if one exists. */
	author = NULL;
	if (atoms->group[1]->type == ATOM_TAG_ARG)
		author = atoms->group[1]->value;

	/* Get the contents of the entire quote. */
	cursor = buf;
	*cursor = '\0';
	for (i = author ? 2 : 1; i < (atoms->len - 1); i++)
		cursor = buffer_simple_tag(cursor, atoms->group[i]);

	/* Break the quote out according to our margin. */
	cursor = buf;
	brk = cursor;
	while (*brk != '\0') {
		/* Go to the next line. */
		if (cursor != buf)
			putchar('\n');
		printf("> ");

		/* Break up any long lines and print them out. */
		brk = wrap_text(cursor, 78);
		print_until(cursor, brk, 0);

		/* Skip the character where we've broken the line. */
		cursor = (char*)(brk + 1);
	}

	/* Attribute the quote. */
	if (author)
		printf("\n  -- %s", author);

	*buf = '\0';
}

/**
 * Prints out preformatted text associated with an atom group.
 *
 * @param atoms  Atom group object to be converted.
 * @param indent Should we indent the section?
 */
void print_code_tag_group(const atom_group_t *atoms, int indent) {
	const char *lang;
	int i;

	/* Get the language code if one exists. */
	lang = NULL;
	if (atoms->group[1]->type == ATOM_TAG_ARG)
		lang = atoms->group[1]->value;

	/* Print out the preformatted section with indentation fixed. */
	for (i = lang ? 2 : 1; i < (atoms->len - 1); i++) {
		const char *buf = atoms->group[i]->value;

		if (indent)
			printf("    ");
		while (*buf != '\0') {
			if (*buf == '\n') {
				/* Go to a new line and auto-indent it. */
				putchar('\n');
				if (indent)
					printf("    ");
			} else if (*buf == '\t') {
				/* Convert tabs to spaces to ensure it looks consistent. */
				printf("    ");
			} else {
				/* Regular character. */
				putchar(*buf);
			}

			buf++;
		}
	}
}

/**
 * Prints out a list associated with an atom group.
 *
 * @param atoms Atom group object to be converted.
 */
void print_list_tag_group(const atom_group_t *atoms) {
	char *buf;
	const char *brk;
	char *cursor;
	int i;
	int o;

	/* Allocate a buffer that's definitely large enough to hold a line. */
	buf = malloc(LINE_MAX * sizeof(char));

	cursor = buf;
	*cursor = '\0';
	for (i = 2, o = 1; i < (atoms->len - 1); i++) {
		atom_t *atom = atoms->group[i];

		/* Check if we got a new list item. */
		if ((atom->type == ATOM_TAG_OPEN) && (*atom->value == '*')) {
			char bullet[4];

			/* Is this an ordered list? */
			if (*atoms->group[0]->value == 'O') {
				snprintf(bullet, 4, "%d.", o++);
			} else {
				strcpy(bullet, "-");
			}

			/* Indent list and print the bullet. */
			printf("  %s ", bullet);
		} else if (atom->type == ATOM_LINEBREAK) {
			/* Print and reset the buffered list item. */
			printf("%s\n", buf);

			/* Reset list item buffer for next item. */
			cursor = buf;
			*cursor = '\0';
		} else if (!atom_just_whitespace(atom)) {
			if (atom->type == ATOM_TAG_OPEN) {
				/* Handle complex tags. */
				if (!strcmp(atom->value, "URL")) {
					atom_group_t *subgrp;

					/* Create a subgroup for this tag and populate it. */
					subgrp = atom_group_new();
					while (i < (atoms->len - 1)) {
						atom_group_append(subgrp, atoms->group[i]);

						/* Check if we've reached the end. */
						if ((atoms->group[i]->type == ATOM_TAG_CLOSE) &&
								!strcmp(atoms->group[i]->value, atom->value)) {
							break;
						}

						/* Advance to the next atom. */
						i++;
					}

					/* Print subgroup and free while keeping original atoms. */
					cursor = buffer_link_tag_group(cursor, subgrp);
					atom_group_free_partial(subgrp);
				} else {
					cursor = buffer_simple_tag(cursor, atom);
				}
			} else {
				/* Handle simple tags. */
				cursor = buffer_simple_tag(cursor, atom);
			}
		}
	}

	/* Free resources. */
	free(buf);
	buf = NULL;
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

/**
 * Checks if an atom is simply whitespace. This function does not flag newlines
 * as whitespace.
 *
 * @param atom Atom object to be checkded.
 *
 * @return TRUE if the contents of the atom is just whitespace and nothing else,
 *         FALSE if it does contain some content.
 */
int atom_just_whitespace(const atom_t *atom) {
	size_t len = strspn(atom->value, " \t");
	return *(atom->value + len) == '\0';
}
