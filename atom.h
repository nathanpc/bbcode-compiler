/**
 * atom.h
 * BBCode atom object. The output of the parser.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _BBCODE_ATOM_H
#define _BBCODE_ATOM_H

/**
 * Types of atoms.
 */
typedef enum atom_type_e {
	ATOM_RAW = 0,
	ATOM_PREFMT,
	ATOM_LINEBREAK,
	ATOM_ESCAPED,
	ATOM_TAG_OPEN,
	ATOM_TAG_ARG,
	ATOM_TAG_CLOSE
} atom_type_t;

/**
 * Atom object definition.
 */
typedef struct atom_s {
	atom_type_t type;
	char *value;
} atom_t;

/**
 * Structure that can hold multiple atoms that make a single output structure
 * when exported to a different markup language.
 */
typedef struct atom_group_s {
	atom_t **group;
	int len;
} atom_group_t;

/* Atom object functions. */
atom_t* atom_new(atom_type_t type);
void atom_free(atom_t *atom);
int atom_parse_line(atom_t *atom, const char *line);
int atom_parse_continue(atom_t *atom, const char *line);
atom_type_t atom_type_fromstr(const char *str);
int atom_args_maybe(const atom_t *atom);
void atom_print(const atom_t *atom);

/* Atom group object functions. */
atom_group_t* atom_group_new(void);
void atom_group_free(atom_group_t *atoms);
void atom_group_free_partial(atom_group_t *atoms);
int atom_group_append(atom_group_t *atoms, atom_t *atom);

#endif /* _BBCODE_ATOM_H */
