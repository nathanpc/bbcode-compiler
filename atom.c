/**
 * atom.c
 * BBCode atom object. The output of the parser.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "atom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "utils.h"

/**
 * Maximum length of an atom type string buffer length.
*/
#define TYPE_STR_MAX 15

/* Private functions. */
int has_prefmt_suffix(const char *str);

/**
 * Allocates a brand new atom object.
 *
 * @param type Type of the new atom.
 *
 * @return Brand new, dynamically allocated, atom object.
 */
atom_t* atom_new(atom_type_t type) {
	atom_t *atom;

	/* Allocate a new atom object. */
	atom = malloc(sizeof(atom_t));
	atom->type = type;
	atom->value = NULL;

	return atom;
}

/**
 * Frees up any resources taken by an atom object.
 *
 * @param atom Atom object to be completely freed.
 */
void atom_free(atom_t *atom) {
	/* Do we even need to do anything? */
	if (atom == NULL)
		return;

	/* Invalidate type and free value string. */
	atom->type = -1;
	if (atom->value) {
		free(atom->value);
		atom->value = NULL;
	}

	/* Free the atom object itself. */
	free(atom);
}

/**
 * Parses an atom from a line of compiled BBCode intermediate language.
 *
 * @param atom Atom object to be populated.
 * @param line Line of compiled BBCode.
 *
 * @return 0 if the parsing was successful. 1 if we need to keep parsing on the
 *         next line. -1 if an error occurred.
 */
int atom_parse_line(atom_t *atom, const char *line) {
	char type[TYPE_STR_MAX];
	const char *cursor;

	/* Extract the atom type. */
	cursor = strcpyu(type, line, '\t');
	if (*cursor == '\0') {
		fprintf(stderr, "error: failed to extract atom type.\n");
		return -1;
	}
	atom->type = atom_type_fromstr(type);
	cursor++;  /* Move past the delimiter. */

	/* Skip double quote in case of a preformatted section. */
	if (atom->type == ATOM_PREFMT)
		cursor++;

	/* Allocate enough space for our atom value. */
	if (atom->value == NULL) {
		atom->value = malloc((strlen(cursor) + 1) * sizeof(char));
	} else {
		atom->value = realloc(atom->value, (strlen(cursor) + 1) * sizeof(char));
	}

	/* Copy our atom's value. */
	strcpy(atom->value, cursor);

	/* Ensure we don't have a single line preformatted section. */
	if (atom->type == ATOM_PREFMT) {
		size_t lenv;
		int lensfx;

		lenv = strlen(atom->value);
		lensfx = has_prefmt_suffix(atom->value);
		if (lensfx) {
			/* Single line preformatted section. */
			atom->value[lenv - lensfx] = '\0';
		} else {
			/* Preformatted section continues, thus add a NL to the value. */
			atom->value = realloc(atom->value, (lenv + 2) * sizeof(char));
			atom->value[lenv] = '\n';
			atom->value[lenv + 1] = '\0';
			return 1;
		}
	}

	return 0;
}

/**
 * Continue parsing an atom from a line of compiled BBCode intermediate
 * language.
 *
 * @param atom Atom object to be populated.
 * @param line Line of compiled BBCode.
 *
 * @return 0 if the parsing was successful and no longer need to continue. 1 if
 *         we need to keep parsing on the next line. -1 if an error occurred.
 */
int atom_parse_continue(atom_t *atom, const char *line) {
	if (atom->type == ATOM_PREFMT) {
		size_t lenv;
		size_t lenl;
		int lens;

		/* Allocate space for our string to grow. */
		lenv = strlen(atom->value);
		lenl = strlen(line);
		atom->value = realloc(atom->value, (lenv + lenl + 1) * sizeof(char));

		/* Append our line to the value. */
		strcat(atom->value, line);
		lens = has_prefmt_suffix(line);
		lenv += lenl;
		if (lens) {
			/* Preformatted section has ended here. */
			atom->value[lenv - lens] = '\0';
			return 0;
		} else {
			/* Preformatted section continues on the next line. */
			atom->value = realloc(atom->value, (lenv + 2) * sizeof(char));
			atom->value[lenv] = '\n';
			atom->value[lenv + 1] = '\0';
		}

		return 1;
	}

	/* I don't know what to do with this... */
	fprintf(stderr, "error: unknown atom type to keep parsing on next line.\n");
	return -1;
}

/**
 * Determines an atom type from its compiled string representation.
 *
 * @param str Atom type as a compiled BBCode string.
 *
 * @return Atom type.
 */
atom_type_t atom_type_fromstr(const char *str) {
	if (!strcmp(str, "TEXT")) {
		return ATOM_RAW;
	} else if (!strcmp(str, "PREFMT")) {
		return ATOM_PREFMT;
	} else if (!strcmp(str, "NEWLINE")) {
		return ATOM_LINEBREAK;
	} else if (!strcmp(str, "TAG_OPEN")) {
		return ATOM_TAG_OPEN;
	} else if (!strcmp(str, "TAG_ARG")) {
		return ATOM_TAG_ARG;
	} else if (!strcmp(str, "TAG_CLOSE")) {
		return ATOM_TAG_CLOSE;
	}

	return -1;
}

/**
 * Checks if an atom is of a specific type.
 *
 * @param atom Atom object to be tested.
 * @param type Type of atom we are expecting.
 *
 * @return TRUE if the atom is of the specified type, FALSE otherwise.
 */
int atom_is_type(const atom_t *atom, atom_type_t type) {
	return atom->type == type;
}

/**
 * Checks if an atom is of a specific value.
 *
 * @param atom  Atom object to be tested.
 * @param value Specific value we are expecting.
 *
 * @return TRUE if the atom is of the specified value, FALSE otherwise.
 */
int atom_is_value(const atom_t *atom, const char *value) {
	return !strcmp(atom->value, value);
}

/**
 * Checks if an atom is of a specific type and of an specific value.
 *
 * @param atom  Atom object to be tested.
 * @param type  Type of atom we are expecting.
 * @param value Expected value of our atom.
 *
 * @return TRUE if both type and value are equal, FALSE otherwise.
 */
int atom_eq(const atom_t *atom, atom_type_t type, const char *value) {
	return (atom->type == type) && !strcmp(atom->value, value);
}

/**
 * Checks if a TAG_OPEN atom is of a type which may contain a subsequent
 * argument atom.
 *
 * @param atom Atom object of type TAG_OPEN to be checked.
 *
 * @return TRUE if the tag is one that may contain arguments. FALSE if it is a
 *         tag that never has arguments.
 */
int atom_args_maybe(const atom_t *atom) {
	const char *v;
	if ((atom->type != ATOM_TAG_OPEN) && (atom->type != ATOM_TAG_CLOSE))
		return 0;

	v = atom->value;
	return !strcmp(v, "URL") || !strcmp(v, "IMG") || !strcmp(v, "CODE") ||
		!strcmp(v, "QUOTE") || !strcmp(v, "COLOR");
}

/**
 * Prints out the contents of an atom in the format of an intermediate output
 * language for formatters to use.
 *
 * @param atom Atom to be printed out.
 */
void atom_print(const atom_t *atom) {
	/* Type */
	switch (atom->type) {
	case ATOM_RAW:
	case ATOM_ESCAPED:
		printf("TEXT");
		break;
	case ATOM_PREFMT:
		printf("PREFMT");
		break;
	case ATOM_LINEBREAK:
		printf("NEWLINE");
		break;
	case ATOM_TAG_OPEN:
		printf("TAG_OPEN");
		break;
	case ATOM_TAG_ARG:
		printf("TAG_ARG");
		break;
	case ATOM_TAG_CLOSE:
		printf("TAG_CLOSE");
		break;
	default:
		printf("UNKNOWN");
	}
	putchar('\t');

	/* Value */
	if (atom->type == ATOM_PREFMT)
		putchar('"');
	printf("%s", (atom->value == NULL) ? "" : atom->value);
	if (atom->type == ATOM_PREFMT)
		printf("\"\tPREFMT");
	putchar('\n');
}

/**
 * Checks if a string ends with the compiled BBCode preformatted section ending.
 *
 * @param String to be tested for the preformatted section ending.
 *
 * @return The number of bytes to trim from the string if the preformatted
 *         section ending. 0 if no valid suffix was found.
 */
int has_prefmt_suffix(const char *str) {
	if (!strcmpe(str, "\"\tPREFMT"))
		return 8;

	return 0;
}

/**
 * Allocates a brand new atom group object.
 *
 * @return Brand new atom group object that must be freed using atom_group_free.
 */
atom_group_t* atom_group_new(void) {
	atom_group_t *atoms = malloc(sizeof(atom_group_t));
	atoms->group = NULL;
	atoms->len = 0;
	return atoms;
}

/**
 * Frees up any resources created by an atom group object, including itself and
 * all atom objects inside it.
 *
 * @param atoms Atom group object to be freed.
 */
void atom_group_free(atom_group_t *atoms) {
	/* Do we even need to do anything? */
	if (atoms == NULL)
		return;

	/* Go through group freeing all atoms. */
	if (atoms->group) {
		int i;
		for (i = 0; i < atoms->len; i++) {
			atom_free(atoms->group[i]);
			atoms->group[i] = NULL;
		}
	}

	/* Free ourselves. */
	atom_group_free_partial(atoms);
}

/**
 * Frees up an atom group without touching any of the atoms it contains.
 *
 * @param atoms Atom group object to be freed.
 */
void atom_group_free_partial(atom_group_t *atoms) {
	/* Do we even need to do anything? */
	if (atoms == NULL)
		return;

	/* Free the atom group array. */
	if (atoms->group) {
		free(atoms->group);
		atoms->group = NULL;
	}

	/* Free ourselves. */
	atoms->len = 0;
	free(atoms);
}

/**
 * Appends a new atom to a group.
 *
 * @param atoms Atom group object to receive the new atom.
 * @param atom  Atom object to be added to the end of the group.
 *
 * @return Total number of atoms in the group.
 */
int atom_group_append(atom_group_t *atoms, atom_t *atom) {
	if (atoms->len == 0) {
		atoms->group = malloc(sizeof(atom_t*));
	} else {
		atoms->group = realloc(atoms->group, (atoms->len + 1) *
			sizeof(atom_t*));
	}

	atoms->group[atoms->len++] = atom;
	return atoms->len;
}
