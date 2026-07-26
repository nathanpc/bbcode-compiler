/**
 * bb2html.c
 * Converts the compiled BBCode intermediate language to HTML.
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
void print_tag(const atom_t *atom, int close);
void print_tag_arg(const atom_t *tag, const atom_t *arg);
void print_tag_group(const atom_group_t *atoms);
void print_image_tag_group(const atom_group_t *atoms);
int atom_maybe_paragraph(const atom_t *atom);


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
	int newlines;
	int inpara;
	FILE *stream;

	/* Take a memory snapshot for memory leak detection. */
	mem_snapshot_start();

	/* Initialize variables. */
	ret = 0;
	newlines = 1;
	inpara = 0;
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
		if (atom->type == ATOM_LINEBREAK) {
			/* Are we closing a paragraph? */
			if (inpara) {
				printf("</p>");
				inpara = 0;
			}

			newlines++;
			putchar('\n');
		} else {
			/* Are we starting a new paragraph? */
			if (newlines && atom_maybe_paragraph(atom)) {
				inpara = newlines > 1;
				if (inpara)
					printf("<p>");
			}

			newlines = 0;
		}

		/* Deal with tags that may span more than a single atom. */
		switch (atom->type) {
		case ATOM_TAG_OPEN:
			/* Check if this is a tag that doesn't need to be grouped. */
			if (atomsgrp || (!atom_args_maybe(atom) && (*atom->value != '*')))
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
				if (first->type == ATOM_TAG_OPEN) {
					/* Check if we are dealing with a list item. */
					if (*first->value == '*') {
						/* Add atoms to the group until we reach a linebreak. */
						if (atom->type != ATOM_LINEBREAK) {
							atom_group_append(atomsgrp, atom);
							atom = NULL;
							break;
						}

						/* Convert linebreak into tag close for list item. */
						atom->type = ATOM_TAG_CLOSE;
						free(atom->value);
						atom->value = strdup("*");
					} else if (!strcmp(first->value, "IMG")) {
						/* Append atoms until the image tag closes. */
						atom_group_append(atomsgrp, atom);
						if ((atom->type != ATOM_TAG_CLOSE) &&
								strcmp(atom->value, "IMG")) {
							atom = NULL;
							break;
						}

						/* Ensure that we ignore the last image tag close. */
						atom = NULL;
					}
				}

				/* Print out the entire tag group and move on. */
				print_tag_group(atomsgrp);
				atom_group_free(atomsgrp);
				atomsgrp = NULL;
			}

			/* Print the atom and move on. */
			if (atom) {
				if (atom->type != ATOM_LINEBREAK)
					print_tag(atom, 1);
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
 * Prints out an HTML tag associated with an atom.
 *
 * @param atom  Atom object to be converted to HTML.
 * @param close Should we put the final '>' of the tag?
 */
void print_tag(const atom_t *atom, int close) {
	if ((atom->type == ATOM_RAW) || (atom->type == ATOM_PREFMT)) {
		/* Regular text and preformatted sections. */
		printf("%s", atom->value);
	} else if (atom->type == ATOM_LINEBREAK) {
		/* Linebreak */
		printf("<br>\n");
	} else if (atom->type >= ATOM_TAG_OPEN) {
		/* Actual tag for us to deal with. */
		const char *v;

		if (atom->type == ATOM_TAG_ARG) {
			fprintf(stderr, "error: Trying to print a tag argument atom.\n");
			return;
		}

		/* Print the tag. */
		v = atom->value;
		putchar('<');
		if (atom->type == ATOM_TAG_CLOSE)
			putchar('/');
		if (!strcmp(v, "B")) {
			putchar('b');
		} else if (!strcmp(v, "I")) {
			putchar('i');
		} else if (!strcmp(v, "U")) {
			putchar('u');
		} else if (!strcmp(v, "S")) {
			putchar('s');
		} else if (!strcmp(v, "PRE")) {
			printf("pre");
		} else if (!strcmp(v, "H1")) {
			printf("h1");
		} else if (!strcmp(v, "H2")) {
			printf("h2");
		} else if (!strcmp(v, "H3")) {
			printf("h3");
		} else if (!strcmp(v, "H4")) {
			printf("h4");
		} else if (!strcmp(v, "H5")) {
			printf("h5");
		} else if (!strcmp(v, "H6")) {
			printf("h6");
		} else if (!strcmp(v, "URL")) {
			putchar('a');
		} else if (!strcmp(v, "IMG")) {
			printf("img");
		} else if (!strcmp(v, "CODE")) {
			if (atom->type == ATOM_TAG_OPEN) {
				printf("pre><code");
			} else {
				printf("code></pre");
			}
		} else if (!strcmp(v, "QUOTE")) {
			printf("blockquote");
		} else if (!strcmp(v, "COLOR")) {
			printf("span");
		} else if (!strcmp(v, "TABLE")) {
			printf("table");
		} else if (!strcmp(v, "*")) {
			printf("li");
		} else if (!strcmp(v, "LIST")) {
			printf("ul");
		} else if (!strcmp(v, "OLIST")) {
			printf("ol");
		} else if (!strcmp(v, "TR")) {
			printf("tr");
		} else if (!strcmp(v, "TH")) {
			printf("th");
		} else if (!strcmp(v, "TD")) {
			printf("td");
		} else {
			fprintf(stderr, "warning: unknown tag %s\n", v);
			printf("%s", v);
		}

		/* Close the tag brace. */
		if (close)
			putchar('>');
	}
}

/**
 * Prints an HTML tag's argument.
 *
 * @param tag Tag atom that the argument is part of.
 * @param arg Argument atom to be printed out.
 */
void print_tag_arg(const atom_t *tag, const atom_t *arg) {
	const char *v, *a;

	/* Shortcut tag and argument values. */
	v = tag->value;
	a = arg->value;

	/* Print out the argument. */
	putchar(' ');
	if (!strcmp(v, "URL")) {
		printf("href=\"%s\"", a);
	} else if (!strcmp(v, "IMG")) {
		printf("src=\"%s\"", a);
	} else if (!strcmp(v, "CODE")) {
		printf("class=\"lang-%s\"", a);
	} else if (!strcmp(v, "QUOTE")) {
		printf("author=\"%s\"", a);
	} else if (!strcmp(v, "COLOR")) {
		printf("style=\"color: %s;\"", a);
	} else {
		fprintf(stderr, "warning: unknown tag argument %s for tag %s.\n", a, v);
	}
}

/**
 * Prints out an HTML tag associated with an atom group.
 *
 * @param atoms Atom group object to be converted to HTML.
 */
void print_tag_group(const atom_group_t *atoms) {
	int i;

	/* Deal with the special case of image tags. */
	if (!strcmp(atoms->group[0]->value, "IMG")) {
		print_image_tag_group(atoms);
		return;
	}

	for (i = 0; i < atoms->len; i++) {
		/* Print tag and its possible argument. */
		if (atoms->group[i]->type == ATOM_TAG_OPEN) {
			int j = 0;
			print_tag(atoms->group[i], 0);
			while ((i + j + 1) < atoms->len) {
				if (atoms->group[i + j + 1]->type == ATOM_TAG_ARG) {
					print_tag_arg(atoms->group[i], atoms->group[i + j + 1]);
					j++;
				} else {
					/* Advance past arguments that have already been printed. */
					if (j > 0) {
						i += j + 1;
						j = 0;
					}
					break;
				}
			}
			putchar('>');

			/* Advance past single argument conditions. */
			if (j > 0)
				i += j;
		} else {
			print_tag(atoms->group[i], 1);
		}
	}
}

/**
 * Prints out an HTML img tag associated with an atom group.
 *
 * @param atoms Atom group object to be converted to an HTML img tag.
 */
void print_image_tag_group(const atom_group_t *atoms) {
	int i;

	/* Open the tag and add the source attribute. */
	print_tag(atoms->group[0], 0);
	print_tag_arg(atoms->group[0], atoms->group[1]);

	/* Print the alt text. */
	if (atoms->len > 3) {
		printf(" alt=\"");
		for (i = 2; i < (atoms->len - 1); i++)
			print_tag(atoms->group[i], 1);
		putchar('"');
	}

	/* Close the tag. */
	printf(" />");
}

/**
 * Checks if the atom may be the start of a paragraph.
 *
 * @param atom Atom to be checked.
 *
 * @return TRUE if the atom type or value may be a valid start of a paragraph.
 *         FALSE if it's an atom that may never be a start of a paragraph.
 */
int atom_maybe_paragraph(const atom_t *atom) {
	const char *v;

	/* If it's text, it's always a paragraph. */
	if (atom->type == ATOM_RAW) {
		return 1;
	} else if (atom->type != ATOM_TAG_OPEN) {
		return 0;
	}

	/* Check for specific tags that may be at the start of a paragraph. */
	v = atom->value;
  	return !strcmp(v, "B") || !strcmp(v, "I") || !strcmp(v, "U") ||
		!strcmp(v, "S") || !strcmp(v, "URL") || !strcmp(v, "COLOR");
}
