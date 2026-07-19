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

/* Private functions. */
void print_tag(const atom_t *atom, int close);
void print_tag_arg(const atom_t *tag, const atom_t *arg);
void print_tag_group(const atom_group_t *atoms);

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

	/* Initialize variables. */
	ret = 0;
	atom = NULL;
	atomsgrp = NULL;

	/* Go through input line-by-line. */
	while (fgets(line, sizeof(line), stdin)) {
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
			if (!atom_args_maybe(atom))
				goto handle_atom;

			atomsgrp = atom_group_new();
			atom_group_append(atomsgrp, atom);
			atom = NULL;
			break;
		case ATOM_TAG_ARG:
			if (atomsgrp == NULL) {
				fprintf(stderr, "error: atom argument found without group\n");
				goto finish;
			}
			atom_group_append(atomsgrp, atom);
			atom = NULL;
			break;
handle_atom:
		default:
			/* Handle previous atom group first if one exists. */
			if (atomsgrp) {
				print_tag_group(atomsgrp);
				atom_group_free(atomsgrp);
				atomsgrp = NULL;
			}

			print_tag(atom, 1);
			atom_free(atom);
			atom = NULL;
			break;
		}
	}

finish:
	/* Free resources. */
	atom_group_free(atomsgrp);
	atomsgrp = NULL;
	atom_free(atom);
	atom = NULL;

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
		printf(atom->value);
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
			printf("pre><code");
		} else if (!strcmp(v, "QUOTE")) {
			printf("blockquote");
		} else if (!strcmp(v, "COLOR")) {
			printf("span");
		} else if (!strcmp(v, "TABLE")) {
			printf("table");
		} else if (!strcmp(v, "TR")) {
			printf("tr");
		} else if (!strcmp(v, "TH")) {
			printf("th");
		} else if (!strcmp(v, "TD")) {
			printf("td");
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
	}
}

/**
 * Prints out an HTML tag associated with an atom group.
 *
 * @param atoms Atom group object to be converted to HTML.
 */
void print_tag_group(const atom_group_t *atoms) {
	int i;

	/* Print the tag. */
	print_tag(atoms->group[0], 0);

	/* Print out any arguments. */
	for (i = 1; i < atoms->len; i++) {
		/* Check if the atom is actually an argument. */
		if (atoms->group[i]->type != ATOM_TAG_ARG) {
			fprintf(stderr, "error: item #%d in atom group not argument.\n", i);
			return;
		}

		/* Print the tag argument. */
		print_tag_arg(atoms->group[0], atoms->group[i]);
	}

	/* Close the tag. */
	putchar('>');
}
