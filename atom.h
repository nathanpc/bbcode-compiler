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

atom_t* atom_new(atom_type_t type);
void atom_free(atom_t *atom);
int atom_parse_line(atom_t *atom, const char *line);
int atom_parse_continue(atom_t *atom, const char *line);
atom_type_t atom_type_fromstr(const char *str);
void atom_print(const atom_t *atom);

#endif /* _BBCODE_ATOM_H */
