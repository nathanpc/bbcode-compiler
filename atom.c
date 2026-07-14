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
 * Prints out the contents of an atom in the format of an intermediate output
 * language for formatters to use.
 *
 * @param atom Atom to be printed out.
 */
void atom_print(const atom_t *atom) {
	/* Type */
	switch (atom->type) {
	case ATOM_RAW:
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
#ifdef _DEBUG
	printf("%s", atom->value);
#else
	printf("%s", (atom->value == NULL) ? "" : atom->value);
#endif /* _DEBUG */
	if (atom->type == ATOM_PREFMT)
		printf("\"\tPREFMT");
	putchar('\n');
}
