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
char* buffer_paragraph_tag_group(char *buf, const atom_group_t *atoms);
char* buffer_link_tag_group(char *buf, const atom_group_t *atoms);
void print_link_tag_group(const atom_group_t *atoms);
void print_image_tag_group(const atom_group_t *atoms);
void print_header_tag_group(const atom_group_t *atoms);
void print_quote_tag_group(const atom_group_t *atoms);
void print_code_tag_group(const atom_group_t *atoms, int indent);
void print_list_tag_group(const atom_group_t *atoms);
void print_table_tag_group(const atom_group_t *atoms);
void print_table_row_divider(int cols, const int *colsizes, int header);
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
	atom_group_t *paragrp;
	atom_group_t *atomsgrp;
	atom_t *atom;
	int ret;
	FILE *stream;

	/* Take a memory snapshot for memory leak detection. */
	mem_snapshot_start();

	/* Initialize variables. */
	ret = 0;
	atom = NULL;
	paragrp = NULL;
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
			if ((atomsgrp == NULL) && (paragrp == NULL)) {
				fprintf(stderr, "error: atom argument found without group\n");
				goto finish;
			} else if (paragrp) {
				goto handle_atom;
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
				if (atom_is_type(first, ATOM_TAG_OPEN)) {
					atom_group_append(atomsgrp, atom);
					if (atom_eq(atom, ATOM_TAG_CLOSE, first->value)) {
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
			} else if (paragrp) {
				/* Grouping up atoms for a paragraph. */
				if (!atom_is_type(atom, ATOM_LINEBREAK)) {
					atom_group_append(paragrp, atom);
					atom = NULL;
				} else {
					/* It's time to print the paragraph. */
					size_t paralen;
					int i;

					/* Allocate a buffer long enough to hold the paragraph. */
					paralen = 1;
					for (i = 0; i < paragrp->len; i++)
						paralen += strlen(paragrp->group[i]->value);
					buf = malloc((size_t)(paralen * 1.3) * sizeof(char));
					*buf = '\0';

					/* Buffer the entire contents of the paragraph. */
					buffer_paragraph_tag_group(buf, paragrp);
					atom_group_free(paragrp);
					paragrp = NULL;

					/* Print paragraph text wrapped according to our margin. */
					print_text_wrap(buf, 80, NULL);

					/* Free the temporary buffer. */
					free(buf);
					buf = NULL;
				}
			} else if (!atom_is_type(atom, ATOM_LINEBREAK)) {
				/* We are about to start parsing a paragraph. */
				paragrp = atom_group_new();
				atom_group_append(paragrp, atom);
				atom = NULL;
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
	if (urls->len > 0) {
		putchar('\n');
		putchar('\n');
		for (ret = 0; ret < urls->len; ret++)
			printf("[%d]: %s\n", ret + 1, urls->group[ret]->value);
	}
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
	if (atom_is_type(atom, ATOM_RAW) || atom_is_type(atom, ATOM_PREFMT)) {
		/* Regular text and preformatted sections. */
		strcat(buf, atom->value);
	} else if (atom->type >= ATOM_TAG_OPEN) {
		/* Actual tag for us to deal with. */
		const char *v;

		if (atom_is_type(atom, ATOM_TAG_ARG)) {
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
	if (atom_is_type(atom, ATOM_LINEBREAK)) {
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

	if (!strcmp(v, "IMG")) {
		print_image_tag_group(atoms);
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
	} else if (!strcmp(v, "TABLE")) {
		print_table_tag_group(atoms);
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
	if (atom_is_value(atoms->group[0], "H1")) {
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
 * Appends an entire paragraph associated with an atom group, which can be
 * comprised of simple atoms and links, to a string buffer.
 *
 * @param buf   String buffer that will have the atoms appended to.
 * @param atoms Atom group object to be converted.
 *
 * @return Position of the NUL terminator after appending all the atoms.
 */
char* buffer_paragraph_tag_group(char *buf, const atom_group_t *atoms) {
	char *cursor;
	int i;

	/* Go through the atoms and buffer them out. */
	cursor = buf;
	for (i = 0; i < atoms->len; i++) {
		atom_t *atom = atoms->group[i];

		if (atom_is_type(atom, ATOM_TAG_OPEN)) {
			/* Handle complex tags. */
			if (atom_is_value(atom, "URL")) {
				atom_group_t *subgrp;

				/* Create a subgroup for this tag and populate it. */
				subgrp = atom_group_new();
				while (i < (atoms->len - 1)) {
					atom_group_append(subgrp, atoms->group[i]);

					/* Check if we've reached the end. */
					if (atom_is_type(atoms->group[i], ATOM_TAG_CLOSE) &&
							atom_is_value(atoms->group[i], atom->value)) {
						break;
					}

					/* Advance to the next atom. */
					i++;
				}

				/* Print subgroup and free while keeping original atoms. */
				cursor = buffer_link_tag_group(cursor, subgrp);
				atom_group_free_partial(subgrp);
			} else if (atom_is_value(atom, "COLOR")) {
				while (!atom_eq(atoms->group[i], ATOM_TAG_CLOSE, "COLOR")) {
					if (atoms->group[i]->type < ATOM_TAG_OPEN)
						cursor = buffer_simple_tag(cursor, atoms->group[i]);
					i++;
				}
			} else {
				cursor = buffer_simple_tag(cursor, atom);
			}
		} else {
			/* Handle simple tags. */
			cursor = buffer_simple_tag(cursor, atom);
		}
	}

	return cursor;
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
	cursor = NULL;
	for (i = 2; i < (atoms->len - 1); i++)
		cursor = buffer_simple_tag(buf, atoms->group[i]);

	/* Ensure that we error out if something bad happens. */
	if (cursor == NULL) {
		fprintf(stderr, "error: invalid link tag group, unable to buffer, "
			"ignoring.\n");
		return buf + strlen(buf);
	}

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
	*buf = '\0';

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
	atom_group_t *subgrp;
	const char *author;
	size_t buflen;
	char *buf;
	int i;

	/* Get the author of the quote if one exists. */
	author = NULL;
	if (atom_is_type(atoms->group[1], ATOM_TAG_ARG))
		author = atoms->group[1]->value;

	/* Subgroup the entire quote. */
	buflen = 1;
	subgrp = atom_group_new();
	for (i = author ? 2 : 1; i < (atoms->len - 1); i++) {
		atom_group_append(subgrp, atoms->group[i]);
		buflen += strlen(atoms->group[i]->value);
	}

	/* Allocate a string big enough to hold the entire quote. */
	buf = malloc((size_t)(buflen * 1.3) * sizeof(char));
	*buf = '\0';

	/* Buffer the content of the entire quote. */
	buffer_paragraph_tag_group(buf, subgrp);
	atom_group_free_partial(subgrp);
	subgrp = NULL;

	/* Print the quote text wrapped according to our margin and free it. */
	printf("> ");
	print_text_wrap(buf, 80, "> ");
	free(buf);
	buf = NULL;

	/* Attribute the quote. */
	if (author)
		printf("\n  -- %s", author);
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
	if (atom_is_type(atoms->group[1], ATOM_TAG_ARG))
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
	int i;
	int o;

	/* Go through the tag group. */
	for (i = 2, o = 1; i < (atoms->len - 1); i++) {
		atom_t *atom = atoms->group[i];

		/* Process an entire new list item all at once. */
		if (atom_is_type(atom, ATOM_TAG_OPEN) && (*atom->value == '*')) {
			atom_group_t *subgrp;
			char bullet[10];
			size_t buflen;
			char *buf;
			char *cursor;

			/* Create the bullet indentation. */
			*bullet = '\0';
			if (*atoms->group[0]->value == 'O') {
				/* Ordered list. */
				snprintf(bullet, 10, "  %d. ", o++);
			} else {
				/* Regular list. */
				strcpy(bullet, "  - ");
			}

			/* Print the bullet and create an indentation string. */
			printf("%s", bullet);
			cursor = bullet;
			while (*cursor != '\0') {
				*cursor = ' ';
				cursor++;
			}

			/* Subgroup the entire list item. */
			subgrp = atom_group_new();
			buflen = 1;
			i++;
			while (!atom_is_type(atoms->group[i], ATOM_LINEBREAK)) {
				atom_group_append(subgrp, atoms->group[i]);
				buflen += strlen(atoms->group[i]->value);
				i++;
			}

			/* Allocate a buffer that's large enough to hold the item. */
			buf = malloc((size_t)(buflen * 1.3) * sizeof(char));
			*buf = '\0';

			/* Buffer the contents of the list item. */
			buffer_paragraph_tag_group(buf, subgrp);
			atom_group_free_partial(subgrp);
			subgrp = NULL;

			/* Print list item text wrapped according to our margin. */
			print_text_wrap(buf, 80, bullet);

			/* Free the temporary buffer. */
			free(buf);
			buf = NULL;

			/* Break to the next line, but prevent double break at list end. */
			if (i < (atoms->len - 2))
				putchar('\n');
		}
	}
}

/**
 * Prints out a table associated with an atom group.
 *
 * @param atoms Atom group object to be converted.
 */
void print_table_tag_group(const atom_group_t *atoms) {
	char ***table;
	int *colsizes;
	char buf[512];
	int cols;
	int rows;
	int hashead;
	int r, c;
	int i, j;

	/* Determine the number of columns in the table from its first row. */
	cols = -1;
	hashead = 0;
	for (i = 0; i < atoms->len; i++) {
		/* Jump to the first table row. */
		if (cols == -1) {
			if (atom_eq(atoms->group[i], ATOM_TAG_OPEN, "TR"))
				cols = 0;
			continue;
		}

		/* Stop counting when we close the row. */
		if (atom_eq(atoms->group[i], ATOM_TAG_CLOSE, "TR"))
			break;

		/* Count the number of columns from table cell tags. */
		if (atom_is_type(atoms->group[i], ATOM_TAG_OPEN) &&
				(atom_is_value(atoms->group[i], "TH") ||
				atom_is_value(atoms->group[i], "TD"))) {
			cols++;

			/* Check if we have a header in this table. */
			if (atom_is_value(atoms->group[i], "TH"))
				hashead = 1;
		}
	}

	/* Determine the number of rows in the table. */
	rows = 0;
	for (i = 0; i < (atoms->len - 2); i++) {
		if (atom_eq(atoms->group[i], ATOM_TAG_OPEN, "TR"))
			rows++;
	}

	/* Allocate our temporary table variable. */
	table = malloc(rows * sizeof(char**));
	for (i = 0; i < rows; i++)
		table[i] = malloc(cols * sizeof(char*));
	colsizes = malloc(cols * sizeof(int));

	/* Initialize all table cells. */
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			table[i][j] = NULL;
			colsizes[j] = 0;
		}
	}

	/* Populate each table cell. */
	r = 0;
	c = 0;
	for (i = 1; i < (atoms->len - 2); i++) {
		if (atom_is_type(atoms->group[i], ATOM_TAG_OPEN) &&
				(atom_is_value(atoms->group[i], "TD") ||
				atom_is_value(atoms->group[i], "TH"))) {
			/* We've reached a cell. */
			atom_group_t *cellgrp;

			/* Skip to the inside of the cell. */
			i++;

			/* Subgroup the entire cell contents and buffer it. */
			cellgrp = atom_group_new();
			*buf = '\0';
			while (!atom_eq(atoms->group[i], ATOM_TAG_CLOSE, "TD") &&
					!atom_eq(atoms->group[i], ATOM_TAG_CLOSE, "TH")) {
				atom_group_append(cellgrp, atoms->group[i]);
				i++;
			}
			buffer_paragraph_tag_group(buf, cellgrp);
			atom_group_free_partial(cellgrp);
			cellgrp = NULL;

			/* Store the cell contents and move to the next one. */
			table[r][c] = strdup(buf);
			c++;
			i--;
		} else if (atom_eq(atoms->group[i], ATOM_TAG_CLOSE, "TR")) {
			/* Start populating the next row. */
			r++;
			c = 0;
		}
	}

	/* Determine the maximum size of each column. */
	for (c = 0; c < cols; c++) {
		for (r = 0; r < rows; r++) {
			int cz = (int)strlen(table[r][c]);
			if (colsizes[c] < cz)
				colsizes[c] = cz;
		}
	}

	/* Print out the table and its contents. */
	print_table_row_divider(cols, colsizes, 0);
	putchar('\n');
	for (r = 0; r < rows; r++) {
		/* Print an entire row. */
		putchar('|');
		for (c = 0; c < cols; c++) {
			int cz, left, right;

			/* Determine paddings for centering. */
			cz = colsizes[c] - (int)strlen(table[r][c]);
			left = cz / 2;
			right = cz - left;

			/* Left pad to center. */
			for (i = 0; i < left; i++)
				putchar(' ');

			/* Print the actual cell. */
			printf(" %s ", table[r][c]);

			/* Right pad and close the cell. */
			for (i = 0; i < right; i++)
				putchar(' ');
			putchar('|');
		}
		putchar('\n');

		/* Print row divider. */
		print_table_row_divider(cols, colsizes, (r == 0) && hashead);
		if (r < (rows - 1))
			putchar('\n');
	}

	/* Free resources. */
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			if (table[i][j])
				free(table[i][j]);
			table[i][j] = NULL;
		}
		free(table[i]);
		table[i] = NULL;
	}
	free(table);
	table = NULL;
}

void print_table_row_divider(int cols, const int *colsizes, int header) {
	int c, i;

	putchar('|');
	for (c = 0; c < cols; c++) {
		/* Horizontal divider. */
		for (i = 0; i < (colsizes[c] + 2); i++)
			putchar(header ? '=' : '-');

		/* Column divider. */
		putchar((c < (cols - 1)) ? '+' : '|');
	}
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

	/* Atoms to ignore since they are dealt with by buffering. */
	if (!strcmp(v, "URL") || !strcmp(v, "COLOR"))
		return 0;

	return atom_args_maybe(atom) || !strcmp(v, "H1") || !strcmp(v, "H2") ||
		!strcmp(v, "H3") || !strcmp(v, "H4") || !strcmp(v, "H5") ||
		!strcmp(v, "H6") || !strcmp(v, "LIST") || !strcmp(v, "OLIST") ||
		!strcmp(v, "PRE") || !strcmp(v, "TABLE");
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
